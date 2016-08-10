#include "mpidebuggerinternal.h"
#include "mpi.h"
#include <iostream>
#include <sstream>
#include "utils.h"

MPIDebugger* MPIDebugger::m_inst = nullptr;

void MPIDebugger::setRank(const int rank)
{
	m_inst->setRankInternal(rank);

	// If root set timer
	if (rank == 0)
	{
		Utils::startTimer();
	}

	MPI_Bcast(&Utils::m_sBaseTime, sizeof(Utils::m_sBaseTime), MPI_CHAR, 0, MPI_COMM_WORLD);

}

int MPID_Send(const void* buf, int count, int datatype, int dest, int tag, int comm, DebugBaseInfo&& baseDebugInfo)
{
	return MPIDebugger::getInst().Send(buf, count, datatype, dest, tag, comm);
}

int MPID_Recv(void* buf, int count, int datatype, int source, int tag, int comm, MPI_Status* status, DebugBaseInfo&& baseDebugInfo)
{
	return MPIDebugger::getInst().Recv(buf, count, datatype, source, tag, comm, status);
}

int MPID_ISend(const void* buf, int count, int datatype, int dest, int tag, int comm, MPI_Request* request, const DebugBaseInfo& info)
{
	MPIDebugger::getInst().StartCall(info);
	int res = MPIDebugger::getInst().ISend(buf, count, datatype, dest, tag, comm, request);
	MPIDebugger::getInst().EndCall();

	return res;
}

int MPID_IRecv(void* buf, int count, int datatype, int source, int tag, int comm,  MPI_Request* request, const DebugBaseInfo& info)
{
	MPIDebugger::getInst().StartCall(info);
	int res = MPIDebugger::getInst().IRecv(buf, count, datatype, source, tag, comm, request);
	MPIDebugger::getInst().EndCall();

	return res;
}

int MPID_Barrier(int comm, const DebugBaseInfo& info)
{
	MPIDebugger::getInst().StartCall(info);
	int res = MPIDebugger::getInst().Barrier(comm);
	MPIDebugger::getInst().EndCall();

	return res;
}

int MPID_Gather(const void *sendbuf, int sendcount, int sendtype, void *recvbuf, int recvcount, int recvtype, int root, int comm, const DebugBaseInfo& info)
{
	MPIDebugger::getInst().StartCall(info);
	const int res = MPIDebugger::getInst().Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
	MPIDebugger::getInst().EndCall();

	return res;
}

int MPID_Scatter(const void *sendbuf, int sendcount, int sendtype, void *recvbuf, int recvcount, int recvtype, int root, int comm, const DebugBaseInfo& info)
{
	MPIDebugger::getInst().StartCall(info);
	const int res = MPIDebugger::getInst().Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
	MPIDebugger::getInst().EndCall();

	return res;
}

int MPID_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[], const DebugBaseInfo& info)
{
	MPIDebugger::getInst().StartCall(info);
	int res = MPIDebugger::getInst().WaitAll(count, array_of_requests, array_of_statuses);
	MPIDebugger::getInst().EndCall();

	return res;
}

int MPIDebugger::Send(const void* buf, int count, int datatype, int dest, int tag, int comm)
{
	m_tempDebugInfo.mType = DET_SEND;
	m_tempDebugInfo.mDest = dest;
	m_tempDebugInfo.mTag = tag;
	OnRightBeforeCall();

	//printf("SEND - File %s line %d\n", fileName == nullptr ? "none" : fileName,  lineNum);
	return MPI_Send(buf, count, datatype,dest, tag, comm);
}

int MPIDebugger::Recv(void* buf, int count, int datatype, int dest, int tag, int comm, MPI_Status* status)
{
	m_tempDebugInfo.mType = DET_RECV;
	m_tempDebugInfo.mDest = dest;
	m_tempDebugInfo.mTag = tag;
	OnRightBeforeCall();

	//printf("RECV - File %s line %d\n", fileName == nullptr ? "none" : fileName,  lineNum);
	return MPI_Recv(buf, count, datatype,dest, tag, comm, status);
}

int MPIDebugger::ISend(const void* buf, int count, int datatype, int dest, int tag, int comm, MPI_Request* request)
{
	m_tempDebugInfo.mType = DET_ISEND;
	m_tempDebugInfo.mDest = dest;
	m_tempDebugInfo.mTag = tag;
	OnRightBeforeCall();

	//printf("SEND - File %s line %d\n", fileName == nullptr ? "none" : fileName,  lineNum);
	return MPI_Isend(buf, count, datatype,dest, tag, comm, request);
}

int MPIDebugger::IRecv(void* buf, int count, int datatype, int dest, int tag, int comm, MPI_Request* request)
{
	m_tempDebugInfo.mType = DET_IRECV;
	m_tempDebugInfo.mDest = dest;
	m_tempDebugInfo.mTag = tag;
	OnRightBeforeCall();

	//printf("RECV - File %s line %d\n", fileName == nullptr ? "none" : fileName,  lineNum);
	return MPI_Irecv(buf, count, datatype,dest, tag, comm, request);
}

int MPIDebugger::Barrier(int comm)
{	
	m_tempDebugInfo.mType = DET_BARRIER;
	OnRightBeforeCall();

	//printf("RECV - File %s line %d\n", fileName == nullptr ? "none" : fileName,  lineNum);
	return MPI_Barrier(comm);
}

int MPIDebugger::Gather(const void *sendbuf, int sendcount, int sendtype, void *recvbuf, int recvcount, int recvtype, int root, int comm)
{
	m_tempDebugInfo.mType = DET_GATHER;
	m_tempDebugInfo.mDest = root;
	OnRightBeforeCall();

	return MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int MPIDebugger::Scatter(const void *sendbuf, int sendcount, int sendtype, void *recvbuf, int recvcount, int recvtype, int root, int comm)
{
	m_tempDebugInfo.mType = DET_SCATTER;
	m_tempDebugInfo.mDest = root;
	OnRightBeforeCall();

	return MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int MPIDebugger::WaitAll(int count, MPI_Request* array_of_requests, MPI_Status* array_of_statuses)
{
	m_tempDebugInfo.mType = DET_WAITALL;
	OnRightBeforeCall();

	return MPI_Waitall(count, array_of_requests, array_of_statuses);
}

void MPIDebugger::StartCall(const DebugBaseInfo& info)
{
	assert(m_isInCall == false && "already in cal???");
	m_isInCall = true;		
	m_tempDebugInfo.reset();
	m_tempDebugInfo = info;
	m_tempDebugInfo.mStartTime = Utils::getDebuggerTime();	
}

// Called before MPI call
void MPIDebugger::OnRightBeforeCall()
{
	mEntries.push_back(m_tempDebugInfo);
	if (mFlushAfterEachEvent)
	{
		flushLastEvents();
	}
}

void MPIDebugger::injectManualEvent(const DebugBaseInfo& info)
{
	mEntries.push_back(info);
	if (mFlushAfterEachEvent)
	{
		flushLastEvents();
	}
}

void MPIDebugger::EndCall()
{
	m_isInCall = false;

	//if (mEntries.back().mType == DET_BARRIER)
	//	mEntries.back().mEndTime = mEntries.back().mStartTime;
	//else
	mEntries.back().mEndTime = Utils::getDebuggerTime();

	if (mFlushAfterEachEvent)
	{
		modifyOnEndLastEvent();
		mOutEventsFileBinary.flush();
	}
}

MPIDebugger::MPIDebugger(const MPIDebuggerConfig& config)
	: mNextEventToFlush(0) 
	, mConfig(config)
	, m_isInCall(false)
	, mRank(-1)
	
{
	m_tempDebugInfo.reset();	
	mFlushAfterEachEvent = config.flushAfterEveryMsg;
}

void MPIDebugger::setRankInternal(const int rank)
{
	mRank = rank;

#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
	std::ostringstream strText;
	Utils::fillFileNameTextForRank(strText, rank);
	mOutEventsFileText.open(strText.str().c_str(), std::ofstream::out);
	if (!mOutEventsFileText.is_open())
	{
		std::cout<<"ERROR: COULDN'T CREATE OUTPUT FILE ! check user rights";
		assert(false);
	}
#endif
	
	std::ostringstream strBinary;
	Utils::fillFileNameBinaryForRank(strBinary, rank);
	mOutEventsFileBinary.open(strBinary.str(), std::ios::binary | std::ios::out);
	if (!mOutEventsFileBinary.is_open())
	{
		std::cout<<"ERROR: COULDN'T CREATE OUTPUT FILE ! check user rights";
		assert(false);
	}
}

MPIDebugger::~MPIDebugger()
{
#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
	mOutEventsFileText.close();
#endif
	
	mOutEventsFileBinary.close();
}


void MPIDebugger::flushLastEvents()
{
	while (mNextEventToFlush < mEntries.size())
	{
		const DebugBaseInfo& ev = mEntries[mNextEventToFlush];


#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
		writeTextEvent(mOutEventsFileText, ev);
		mOutEventsFileText.flush();
#endif

		writeBinaryEventBlob(mOutEventsFileBinary, ev);	
		mOutEventsFileBinary.flush();

		mNextEventToFlush++;
	}
}


#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
void MPIDebugger::writeTextEvent(std::ofstream& fileRef, const DebugBaseInfo& ev)
{
	const char* str = getDebugEventTypeString(ev.mType);
	fileRef << "P"<< ev.mProcessId <<" Type: " << str << " Time:["
		<< ev.mStartTime <<","<<ev.mEndTime<< " (File: " << (ev.mFileName ? ev.mFileName : "none") << ", " << "Line: " << ev.mLineNum << ") "
		<< "(Dest: " << ev.mDest << ", tag " << ev.mTag << ") " << "desc: " << (ev.mShortDescription ? ev.mShortDescription : "none") << "\n";
}
#endif

void MPIDebugger::writeBinaryEventBlob(std::ofstream& fileRef, const DebugBaseInfo& ev)
{
	size_t fileNameLen = ev.mFileName ? strlen(ev.mFileName) + 1 : 0;
	size_t descriptionLen = (ev.mShortDescription ? strlen(ev.mShortDescription) + 1 : 0);

	fileRef.write((const char*)&ev.mType, sizeof(ev.mType));				// type of event
	fileRef.write((const char*)&fileNameLen, sizeof(fileNameLen));			// Filename
	if (fileNameLen > 0) fileRef.write(ev.mFileName, fileNameLen);								

	fileRef.write((const char*)&ev.mLineNum, sizeof(ev.mLineNum));			// Line num
	fileRef.write((const char*)&ev.mDest, sizeof(ev.mDest));				// Destination and tag
	fileRef.write((const char*)&ev.mTag, sizeof(ev.mTag));		
	
	fileRef.write((const char*)&descriptionLen, sizeof(descriptionLen));
	if (descriptionLen) fileRef.write(ev.mShortDescription, descriptionLen);					// Description
	fileRef.write((const char*)&ev.mStartTime, sizeof(ev.mStartTime));		// Start and end time
	fileRef.write((const char*)&ev.mEndTime, sizeof(ev.mEndTime));
}

bool MPIDebugger::readBinaryEventBlob(std::ifstream& fileRef, DebugBaseInfo& outEv)
{
	if (fileRef.eof())
		return false;

	size_t fileNameLen = 0, descriptionLen = 0;

	fileRef.read((char*)&outEv.mType, sizeof(outEv.mType));
	fileRef.read((char*)&fileNameLen, sizeof(fileNameLen));		// Filename.
	if (fileNameLen > 0)
	{
		outEv.mFileName = new char[fileNameLen];
		fileRef.read (outEv.mFileName, fileNameLen);									
	}
	else
	{
		outEv.mFileName = nullptr;
	}

	fileRef.read((char*)&outEv.mLineNum, sizeof(outEv.mLineNum));					// Line num
	fileRef.read((char*)&outEv.mDest, sizeof(outEv.mDest));							// Destination and tag
	fileRef.read((char*)&outEv.mTag, sizeof(outEv.mTag));							

	fileRef.read((char*)&descriptionLen, sizeof(descriptionLen));
	if (descriptionLen > 0)
	{
		outEv.mShortDescription = new char[descriptionLen];
		fileRef.read(outEv.mShortDescription, descriptionLen);		// Description
	}
	else
	{
		outEv.mShortDescription = nullptr;
	}

	fileRef.read((char*)&outEv.mStartTime, sizeof(outEv.mStartTime));				// Start and end time
	fileRef.read((char*)&outEv.mEndTime, sizeof(outEv.mEndTime));										

	return true;
}

void MPIDebugger::modifyOnEndLastEvent()
{
	//TODO: seek and modify in the data blob the event time

	const DebugBaseInfo& ev = mEntries[mNextEventToFlush-1];
	const std::streamoff filePos = mOutEventsFileBinary.tellp();

	mOutEventsFileBinary.seekp(filePos - sizeof(long long));// go back and write updated endtime
	mOutEventsFileBinary.write((const char*)&ev.mEndTime, sizeof(long long));
}

void MPIDebugger::destroyInternal()
{
	flushLastEvents();

	assert(mOutEventsFileBinary.is_open());

#ifdef WRITE_NUMBER_OF_RECORDS_IN_BINARY_FILE
	const size_t numEntries = mEntries.size();
	mOutEventsFileBinary.write((const char*)&numEntries, sizeof(numEntries)); // Write the number of entries at the end of the file !
#endif

	for (auto it = mEntries.begin(); it != mEntries.end(); it++)
	{
		it->destroy();
	}
	
#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
	mOutEventsFileText.close();
#endif

	
	mOutEventsFileBinary.close();
}

DebugBaseInfo MPIDebugger::buildNote(int rank, NoteAlignment alignment,  char* note, const char* fileName, const int lineNum, long long time)
{
	DebugBaseInfo info;
	info.mTag = alignment;
	info.mDest = rank;
	info.mProcessId = rank;
	info.mStartTime = time;
	info.mEndTime = time;
	info.mType = DET_NOTE;
	info.mShortDescription = note;
	info.mFileName = const_cast<char*>(fileName);
	info.mLineNum = lineNum;

	return info;
}

DebugBaseInfo MPIDebugger::buildLoopForAll(const LoopType loopType, const DebugBaseInfo& sourceInfo)
{
	DebugBaseInfo info = sourceInfo;
	info.mType = loopType == LOOP_BEGIN ? DET_CYCLE_MANUAL_START : DET_CYCLE_MANUAL_END;
	return info;
}

DebugBaseInfo MPIDebugger::buildBarrier(const DebugBaseInfo& info)
{
	DebugBaseInfo out = info;
	out.mType = DET_BARRIER;
	return out;
}