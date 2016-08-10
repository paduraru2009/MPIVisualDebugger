// This is for gathering the output from N processes and produce the sorted final output
#include <iostream>
#include "../../mpidebugger.h"
#include <assert.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include "MPISequenceOutputWriter.h"
#include <map>
#include <list>
#include <iterator>
#include <limits.h>
#include <stack>

using namespace std;

typedef list<DebugBaseInfo> DebugItemsList;
//typedef list<DebugBaseInfo>::iterator DebugItemsListIter;
//DebugItemsList gListOfProcessedItems;

MPIDebugger::DebugItemsArray gArrayOfItems;

vector<MPIDebugger::DebugItemsArray> itemsPerProcess;
vector<int>	itemsHeadPerProcess;
vector<vector<int>> loopIndicesPerProcess;

enum EventType
{
	IDLE,
	WAITING_CYCLE_START,
	WAITING_CYCLE_END,
	WAITING_BARRIER,
};

#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
ofstream combinedOutputFile;
#endif

MPISequenceOutputWriter umlWriter;

void outputEvent(const DebugBaseInfo& ev)
{
#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
	MPIDebugger::writeTextEvent(combinedOutputFile, ev);
#endif

	umlWriter.write(ev);
}

// Returns null when finished

struct EventState
{
	EventState()
		:	numStillWaitingBarrier(0)
		,	state(IDLE)
	{}

	void init(const EventType _state, int numProcs)
	{
		state = _state;
		isWaitingBarrier.resize(numProcs);
		waitingBarrierIndex.resize(numProcs);
		numStillWaitingBarrier =  0;
		//for (int i = 0; i < )

		std::fill(isWaitingBarrier.begin(), isWaitingBarrier.end(), false);
		std::fill(waitingBarrierIndex.begin(), waitingBarrierIndex.end(), -1);
	}

	EventType state;
	std::vector<bool> isWaitingBarrier;		// waittingBarrier[P] = true if this waits for another processor to get to barrier
	std::vector<int> waitingBarrierIndex;	// waitingBarrierINdex[P] - is the index where P has the barrier event in his stream
	int numStillWaitingBarrier;
};

stack<EventState> stackOfGlobalEvents;	// stack of events that apply for all processes (barriers, loops)
inline EventState* getCurrentEvent() { return stackOfGlobalEvents.empty() ? nullptr : &stackOfGlobalEvents.top(); }
inline EventType getCurrentEventState() { return stackOfGlobalEvents.empty() ? IDLE : stackOfGlobalEvents.top().state; }

vector<int> tempWaitingBarrierIndex;

void fillAdditionalDetails(DebugBaseInfo& out);

const DebugBaseInfo*  getNextEvent()
{
	const int numProc = itemsPerProcess.size();
	for (int i = 0; i < numProc; i++) 
		std::fill(tempWaitingBarrierIndex.begin(), tempWaitingBarrierIndex.end(), -1);

	// NOT an optimized version: TODO - use min heaps
	
	// Select the smallest from all - should take the min from heap here!		
	EventState* currentEventState = getCurrentEvent();

	long long smallestProcTime = 0;
	int selectedProcId = -1;
	for (int procId = 0; procId < numProc; procId++)
	{
		MPIDebugger::DebugItemsArray& arrItems = itemsPerProcess[procId];
		const unsigned int head = itemsHeadPerProcess[procId];
		const bool isProcRestrictedInCurrentState = getCurrentEventState() != IDLE && currentEventState->isWaitingBarrier[procId];
		if (head >= arrItems.size() || isProcRestrictedInCurrentState)
			continue;

		if (arrItems[head].mEndTime < smallestProcTime || selectedProcId == -1)
		{
			smallestProcTime = arrItems[head].mEndTime;
			selectedProcId = procId;
		}
	}
	
	if (selectedProcId != -1)
	{
		const DebugBaseInfo* res = &itemsPerProcess[selectedProcId][itemsHeadPerProcess[selectedProcId]];
		itemsHeadPerProcess[selectedProcId]++; // advance header
		
		const DebugEventType itemType = res->mType;
		if (itemType == DET_CYCLE_MANUAL_START || itemType == DET_BARRIER)
		{	
			// If proc that detected this is already in start/barrier event => there is an aggregated event, check and put it stack.
			currentEventState = getCurrentEvent();
			const EventType currentEventType = getCurrentEventState();
			const bool isNewEvent = currentEventType == IDLE || currentEventState->isWaitingBarrier[selectedProcId] || (currentEventType != WAITING_BARRIER && itemType == DET_BARRIER);
			
			if (isNewEvent)
			{				
				// Check if all have this loop
				tempWaitingBarrierIndex[selectedProcId] = itemsHeadPerProcess[selectedProcId] - 1; // Because is already incremented
				int numProcessesInThisLoop = 1;

				{
					int nextNode= res->mNext;
					int nextTag = res->mNextTag;
					while (nextNode != -1) 
					{ 
						numProcessesInThisLoop++; 
						tempWaitingBarrierIndex[nextNode] = nextTag;
						const DebugBaseInfo& nextP = itemsPerProcess[nextNode][nextTag];
						nextNode = nextP.mNext;
						nextTag = nextP.mNextTag;
					}
				}

				{			
					int prevNode = res->mPrev;
					int prevTag = res->mPrevTag;
					while (prevNode != -1) 
					{ 
						numProcessesInThisLoop++; 
						tempWaitingBarrierIndex[prevNode] = prevTag;
						DebugBaseInfo& prevP = itemsPerProcess[prevNode][prevTag];
						prevNode = prevP.mPrev;
						prevTag = prevP.mPrevTag;
					}
				}

				if (numProc == numProcessesInThisLoop)	// All in the same loop !
				{
					// Push into the stack this event since it's a global one
					EventState newState;
					stackOfGlobalEvents.push(newState);

					EventState& outState = stackOfGlobalEvents.top();
					const EventType eventType = (itemType == DET_CYCLE_MANUAL_START ? WAITING_CYCLE_START : WAITING_BARRIER);
					outState.init(eventType, numProc);
					outState.isWaitingBarrier[res->mProcessId] = true;
					outState.numStillWaitingBarrier = numProc;
					outState.waitingBarrierIndex = tempWaitingBarrierIndex;
				}
				else
				{
					DebugBaseInfo noteBegin = MPIDebugger::buildNote(res->mProcessId, NOTE_CENTER, (itemType == DET_CYCLE_MANUAL_START ? "Loop" : "Barrier"), res->mFileName, res->mLineNum, res->mEndTime);
					outputEvent(noteBegin);
				}
			}			
		}			
		
		currentEventState = getCurrentEvent();
		const EventType currentEventType = getCurrentEventState();
		const bool isCurrentStateInCycleOrBarrier =  (currentEventType == WAITING_CYCLE_START || currentEventType == WAITING_CYCLE_END || currentEventType == WAITING_BARRIER); 

		if (isCurrentStateInCycleOrBarrier)
		{
			// Set expected event and next state - if too many add a vectorized version
			DebugEventType expectedEvent = DET_NUM; 
			EventType nextState = IDLE;

			if (currentEventType == WAITING_CYCLE_START)
			{
				expectedEvent = DET_CYCLE_MANUAL_START;
				nextState = WAITING_CYCLE_END;
			}
			else if (currentEventType == WAITING_CYCLE_END)
			{
				expectedEvent = DET_CYCLE_MANUAL_END;
				nextState = IDLE;
			}
			else if (currentEventType == WAITING_BARRIER)
			{
				expectedEvent = DET_BARRIER;
				nextState = IDLE;
			}
			//---------

			// Check if all have arrived here
			if (res->mType == expectedEvent)
			{
				//assert(expectedEvent == DET_CYCLE_MANUAL_END || currentEventState->waitingBarrierIndex[selectedProcId] == itemsHeadPerProcess[selectedProcId]-1); // Not solved yet this case. TODO
				currentEventState->isWaitingBarrier[selectedProcId] = true;
				currentEventState->numStillWaitingBarrier--;

				if (currentEventState->numStillWaitingBarrier == 0)	// barrier in this context can mean start/end cycle too. it's a logical, not mpi barrier
				{
					DebugBaseInfo loopItem = (currentEventType == WAITING_BARRIER ? MPIDebugger::buildBarrier(*res) : 
																						  MPIDebugger::buildLoopForAll((currentEventType == WAITING_CYCLE_START ? LOOP_BEGIN : LOOP_END), *res));

					fillAdditionalDetails(loopItem);

					outputEvent(loopItem);

					if (nextState == IDLE)	// Final state, remove event !
					{
						stackOfGlobalEvents.pop();
					}
					else
					{
						// Move in another state
						currentEventState->state = nextState;
						std::fill(currentEventState->isWaitingBarrier.begin(), currentEventState->isWaitingBarrier.end(), false);
						currentEventState->numStillWaitingBarrier = numProc;
					}
				}				
			}
		}

		if (currentEventType == IDLE)	// Not in an event ?
		{
			if (itemType == DET_CYCLE_MANUAL_END)	// If a manual end is found note, write it only for that process
			{
				DebugBaseInfo noteEnd = MPIDebugger::buildNote(res->mProcessId, NOTE_CENTER, "End", res->mFileName, res->mLineNum, res->mEndTime);
				outputEvent(noteEnd);
			}
		}

		// If a barrier or loop it's treated separately so we need to get 
		return res;
	}	

	return nullptr;
}


void gatherResults(const int numProcs)
{
	itemsPerProcess.reserve(numProcs);
	loopIndicesPerProcess.reserve(numProcs);
	itemsHeadPerProcess.reserve(numProcs);
	tempWaitingBarrierIndex.resize(numProcs);

	for (int i = 0 ; i < numProcs; i++)
	{
		itemsHeadPerProcess.push_back(0);
		vector<int> emptArr;
		loopIndicesPerProcess.push_back(emptArr);
	}

	for (int procId = 0; procId < numProcs; procId++)
	{
		MPIDebugger::DebugItemsArray arr;
		itemsPerProcess.push_back(arr);
		MPIDebugger::DebugItemsArray& itemsForThisProcess = itemsPerProcess.back();

		ostringstream outStr;
		Utils::fillFileNameBinaryForRank(outStr, procId);

		std::string str = outStr.str();
		ifstream inFile;
		inFile.open(str.c_str(), std::istream::binary);
		assert(inFile.is_open());

		// Go back to the end of the file and take numrecords first, then revert back
#ifdef WRITE_NUMBER_OF_RECORDS_IN_BINARY_FILE
		size_t numRecords = 0;
		const int numBytesToGoBack = sizeof(int);
		inFile.seekg (-numBytesToGoBack, inFile.end);
		inFile.read((char*)&numRecords, sizeof(numRecords));
		inFile.seekg(0, inFile.beg);
#endif

#if 0
		// Read and add in the array all the items
		int lowerLimit = 0; //gArrayOfItems.size();
		for (unsigned int r = lowerLimit-1, remainingToRead = numRecords; remainingToRead > 0; remainingToRead--)
		{
			DebugBaseInfo ev;	// TODO: delete mem from ev
			ev.mProcessId = procId;
			MPIDebugger::readBinaryEventBlob(inFile, ev);
			//gArrayOfItems.emplace_back(ev);
			itemsForThisProcess.emplace_back(ev);
			r++;

			// cache the loop index
			if (ev.mType == DET_CYCLE_MANUAL_START || ev.mType == DET_BARRIER)
			{
				loopIndicesPerProcess[procId].push_back(itemsForThisProcess.size() - 1);
			}


			// Check if the new added events creates a cycle
			// O(N^3) algorithm for finding cycle patterns
			int kLowerLimit = (lowerLimit + (r - lowerLimit + 1) / 2);
			for (int k = r; k >= kLowerLimit; k--)
			{
				const int len = r - k + 1;
				assert(len > 0);

				// Check if sequence[k,r]  = sequence[k-len,k-1]
				// First check if on pos (k-len-1) we have a cycle node that has length  != len
				const int presumedCycleNodePos = k-len-1;
				if (presumedCycleNodePos >= 0 && itemsForThisProcess[presumedCycleNodePos].mType == DET_CYCLE && itemsForThisProcess[presumedCycleNodePos].mProcessId != len)
				{
					continue;	// Can't be part of this cycle
				}


				bool isEqual = true;
				int seqAId = k; assert(seqAId > 0);
				int seqBId = k-len; assert(seqBId > 0);

				for (int iter = 0; iter < len; iter++, seqAId++, seqBId++)
				{
					if (!itemsForThisProcess[seqAId].compareInCycle(itemsForThisProcess[seqBId]))
					{
						isEqual = false;
						break;
					}
				}

				if (isEqual)
				{
					// Should we should create a new cycle event ? Remove items in cycle and modify indices

					// Check if there is already a cycle there. Should be located at [k-len-1]
					int locationOfCycleNode = k-len-1;
					if (locationOfCycleNode >= 0 && itemsForThisProcess[locationOfCycleNode].mType == DET_CYCLE)
					{
						// Increase number of iterations
						itemsForThisProcess[locationOfCycleNode].mTag++;	
						
						// Remove the last len items in the array and modify the indices
						itemsForThisProcess.erase(itemsForThisProcess.begin() + k, itemsForThisProcess.end() + r + 1); // second iterator is exclusive

						r -= len;
					}
					else	// New cycle, move all items one position to the right
					{						
						// No need to push_back, there are items to sustain this
						for (int copyIter = k; copyIter >= k - len + 1; copyIter--)
						{
							itemsForThisProcess[copyIter] = itemsForThisProcess[copyIter - 1];
						}

						// Create the new node on pos k-len
						int locationOfCycleNode = k-len;
						itemsForThisProcess[locationOfCycleNode].reset();
						itemsForThisProcess[locationOfCycleNode].mType = DET_CYCLE;
						itemsForThisProcess[locationOfCycleNode].mTag = 2;
						itemsForThisProcess[locationOfCycleNode].mProcessId = len;	// Len of the cycle
						itemsForThisProcess[locationOfCycleNode].mStartTime = itemsForThisProcess[locationOfCycleNode+1].mStartTime;	// Take the time of the first node, hoping that the sort is stable !
						itemsForThisProcess[locationOfCycleNode].mEndTime = itemsForThisProcess[locationOfCycleNode+1].mEndTime;					

						// Remove the last len items in the array and modify the indices
						itemsForThisProcess.erase(itemsForThisProcess.begin() + k + 1, itemsForThisProcess.end() + r + 1); // second iterator is exclusive
						r -= (len - 1);
					}					
				}				
			}
		}		
#endif

		struct LoopInfo { int beginIndex; LoopInfo():beginIndex(-1){}};
		stack<LoopInfo>  openedLoopsStack;
#ifdef WRITE_NUMBER_OF_RECORDS_IN_BINARY_FILE
		for (unsigned int r = 0, recordIndex = 0; r < numRecords;)
#else
		unsigned int r = 0;
		unsigned int recordIndex = 0;
		while(true)
#endif
		{
			DebugBaseInfo ev;	// TODO: delete mem from ev
			ev.mProcessId = procId;
			const bool res = MPIDebugger::readBinaryEventBlob(inFile, ev);
			if (res == false)
				break;

			itemsForThisProcess.emplace_back(ev);

			// cache the loop index
			if (ev.mType == DET_CYCLE_MANUAL_START || ev.mType == DET_BARRIER)
			{
				loopIndicesPerProcess[procId].push_back(itemsForThisProcess.size() - 1);
			}

			if (ev.mType == DET_CYCLE_MANUAL_START)
			{
				LoopInfo loop;
				loop.beginIndex = recordIndex;
				openedLoopsStack.push(loop);
			}
			else if (ev.mType == DET_CYCLE_MANUAL_END)
			{
				assert(!openedLoopsStack.empty());
				LoopInfo lastLoopInfo = openedLoopsStack.top();
				openedLoopsStack.pop();

				const int evIndex = itemsForThisProcess.size() - 1;

				// Is there another instance of the loop in the stream ?
				// Check the last END before START
				const int sizeOfNewLoop = evIndex - lastLoopInfo.beginIndex + 1;
				const int indexToCheckForPrevLoopEnd = lastLoopInfo.beginIndex - 1;

				// Is same as previous cycle ?
				if (indexToCheckForPrevLoopEnd > 0 &&
					itemsForThisProcess[indexToCheckForPrevLoopEnd].mType == DET_CYCLE_MANUAL_END && 
					itemsForThisProcess[indexToCheckForPrevLoopEnd].mDest == sizeOfNewLoop) // Same length ?
				{
					// Increment number of iterations and remove the last part from array
					const int beginIndex = itemsForThisProcess[indexToCheckForPrevLoopEnd].mPrev;
					itemsForThisProcess[beginIndex].mTag++;
					itemsForThisProcess[indexToCheckForPrevLoopEnd].mTag = itemsForThisProcess[beginIndex].mTag;

					// Copy the start / end times from latest iteration
					for (int sourceIndex = lastLoopInfo.beginIndex, targetIndex = beginIndex; targetIndex < beginIndex + sizeOfNewLoop; sourceIndex++, targetIndex++)
					{
						itemsForThisProcess[targetIndex].mStartTime = itemsForThisProcess[sourceIndex].mStartTime;
						itemsForThisProcess[targetIndex].mEndTime = itemsForThisProcess[sourceIndex].mEndTime;
					}

					itemsForThisProcess.erase(itemsForThisProcess.begin() + lastLoopInfo.beginIndex, itemsForThisProcess.end());
					recordIndex -= sizeOfNewLoop;

					// Delete loop indices caches if they in our removed area 
					// TODO: being ordered we can binary search and find the highest index where value[index] < lastLoopInfo.beginIndex

					while(! loopIndicesPerProcess[procId].empty() &&
							loopIndicesPerProcess[procId].back() >= lastLoopInfo.beginIndex)
						loopIndicesPerProcess[procId].pop_back();					
				}
				else // New loop
				{
					// Leave the items as they are, update the tag and dest meaning number of iterations and size respectively					
					itemsForThisProcess[lastLoopInfo.beginIndex].mTag = 1;
					itemsForThisProcess[evIndex].mTag = 1;
					itemsForThisProcess[evIndex].mDest = sizeOfNewLoop;
					itemsForThisProcess[evIndex].mPrev = lastLoopInfo.beginIndex;	// link to the beginning
				}
			}

			r++;
			recordIndex++;
		}

		std::copy(itemsForThisProcess.begin(), itemsForThisProcess.end(), std::back_inserter(gArrayOfItems));
	}
	
	// Construct the links between while nodes	
	for (int i = 0; i < numProcs; i++)
	{
		MPIDebugger::DebugItemsArray& itemsForThisProcess = itemsPerProcess[i];
		const vector<int>& loopIndicesForThisProcess = loopIndicesPerProcess[i];
		
		for (unsigned int loopIndex = 0; loopIndex < loopIndicesForThisProcess.size(); loopIndex++)
		{
			const int itemIndex = loopIndicesForThisProcess[loopIndex];
			DebugBaseInfo& item = itemsForThisProcess[itemIndex];
			assert(item.mType == DET_CYCLE_MANUAL_START || item.mType == DET_CYCLE_MANUAL_END || item.mType == DET_BARRIER);
			
			bool found = false;
			
			for (int j = i + 1; j < numProcs; j++)
			{
				// Check if we can match item to one of the items in other process
				
				MPIDebugger::DebugItemsArray& itemsForOtherProcess = itemsPerProcess[j];
				const vector<int>& loopIndicesForOtherProcess = loopIndicesPerProcess[j];
				
				for (unsigned int loopIndexOther = 0; loopIndexOther < loopIndicesForOtherProcess.size(); loopIndexOther++)
				{
					const int itemIndexOther = loopIndicesForOtherProcess[loopIndexOther];
					DebugBaseInfo& itemOther = itemsForOtherProcess[itemIndexOther];
					assert(itemOther.mType == DET_CYCLE_MANUAL_START || itemOther.mType == DET_BARRIER);
										
					const bool isLinkBetweenCycleStart = itemOther.mType == DET_CYCLE_MANUAL_START && item.compareManualCycle(itemOther);
					const bool isLinkBetweenBarriers = itemOther.mType == DET_BARRIER && item.compareBarrier(itemOther);
					if (isLinkBetweenBarriers || isLinkBetweenCycleStart)
					{
						item.mNext = j;
						item.mNextTag = itemIndexOther;
						itemOther.mPrev = i;
						itemOther.mPrevTag = itemIndex;
					
						found = true;
						break;
					}
				}
				
				if (found == true)
					break;
			}
			
			//if (found == true)
			//	break;
		}
	}
}

void sortResults()
{
	auto sortFunc = [](const DebugBaseInfo& a, const DebugBaseInfo& b) -> bool
	{
		const long long timeA = (a.mEndTime != DEBUGGER_INVALID_TIME ? a.mEndTime : a.mStartTime);
		const long long timeB = (b.mEndTime != DEBUGGER_INVALID_TIME ? b.mEndTime : b.mStartTime);

		return (timeA < timeB);
	};

	std::sort(gArrayOfItems.begin(), gArrayOfItems.end(), sortFunc);

	//gListOfProcessedItems.clear();
	//std::copy(gArrayOfItems.begin(), gArrayOfItems.end(), std::back_inserter(gListOfProcessedItems));
}

void outputResults(const int numProcesses, const MPISequenceOutputWriter::Options& options)
{
#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
	ostringstream strOut;
	strOut << "CombinedOutput.txt";
	combinedOutputFile.open(strOut.str().c_str(), ofstream::binary);
#endif

	umlWriter.init(numProcesses, options);
	umlWriter.begin("output.txt", "Flow");

	const DebugBaseInfo* nextEvent = nullptr;
	while((nextEvent = getNextEvent()))
	{
		// If manual outputed, don't output here again...TODO: improve this
		const bool isLoopOrBarrierNode = (nextEvent->mType == DET_CYCLE_MANUAL_START || nextEvent->mType == DET_CYCLE_MANUAL_END || nextEvent->mType == DET_BARRIER);
		if (isLoopOrBarrierNode)
			continue;

		outputEvent(*nextEvent);
	}

	umlWriter.end();

#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
	combinedOutputFile.close();
#endif
}

bool is_number(const std::string& s)
{
	return !s.empty() && find_if(s.begin(), s.end(), [](char c) {return !isdigit(c);}) == s.end();
}

void fillAdditionalDetails(DebugBaseInfo& out)
{
	const int numProcs = itemsHeadPerProcess.size();
	// If barrier, fill the processor with the longest waiting time
	if (out.mType == DET_BARRIER)
	{
		for (int i = 0; i < numProcs; i++)
		{
			const int headOfBarrier = itemsHeadPerProcess[i] - 1; // head already advanced that's why -1 !
			assert(headOfBarrier >= 0); 

			DebugBaseInfo& barrierItem = itemsPerProcess[i][headOfBarrier];
			assert(barrierItem.mType == DET_BARRIER);

			const long long thisBarrierDuration = barrierItem.mEndTime - barrierItem.mStartTime;
			const long long bestBarrierDuration = out.mEndTime - out.mStartTime;
			if (thisBarrierDuration > bestBarrierDuration)
			{
				out.mProcessId = i;
				out.mStartTime = barrierItem.mStartTime;
				out.mEndTime = barrierItem.mEndTime;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	MPISequenceOutputWriter::Options options;

	// TOOD: read options
	if (argc < 2)
	{
		assert(false);
	}
	
	// Read options
	//-------------------------
	assert(is_number(argv[1]));
	const int numProcesses = atoi(argv[1]);

	typedef map<string, bool*> MapStrToOpt ;
	typedef MapStrToOpt::iterator MapStrToOptIter;
	MapStrToOpt mapToOptions;
	mapToOptions.insert(make_pair("fileLine", &options.showFileNameAndNum));
	mapToOptions.insert(make_pair("timeSpent", &options.timeSpent));
	mapToOptions.insert(make_pair("tag", &options.showTag));
	mapToOptions.insert(make_pair("timeInterval", &options.startEndTime));
	mapToOptions.insert(make_pair("desc", &options.description));

	for (int i = 2; i < argc; i++)
	{
		const char* opt = argv[i];
		if (_stricmp(opt, "all") == 0)
		{
			for (MapStrToOptIter it = mapToOptions.begin(); it != mapToOptions.end(); it++)
				*(it->second) = true;
		}
		else
		{
			auto it = mapToOptions.find(opt);
			if (it == mapToOptions.end())
			{
				cout<<"Param not found " << it->first << endl;
				assert(false);
				return -1;
			}
			else
			{
				*(it->second) = true;
			}
		}
	}
	//-------------------------


	// Gather in the global arrays all outputted entries
	gatherResults(numProcesses);

	// Sort all the items
	sortResults();

	// Output 
	outputResults(numProcesses, options);


	return 0;
}
