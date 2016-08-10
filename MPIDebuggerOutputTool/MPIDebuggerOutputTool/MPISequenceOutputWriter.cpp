#include "MPISequenceOutputWriter.h"
#include "../../mpidebuggercommons.h"
#include <assert.h>
#include <iostream>
#include <sstream>

#include <mpi.h> // For 1:1 type matching only

using namespace std;

void MPISequenceOutputWriter::begin(const char* filename, const char* title)
{
	mOutStream.open(filename, ios::out);

	mOutStream <<"@startuml"<<endl;
	mOutStream <<"hide footbox"<<endl;
	mOutStream <<"title " << title <<endl;

	/*mOutStream<< "note over "; 
	for (int i = 0; i < mNumProcesses; i++)
	{
		mOutStream<<"P"<<i;
		if (i != mNumProcesses - 1) mOutStream<<",";
	}
	mOutStream<<" : MPI_Init"<<endl;

	*/

	//mOutStream<<"note over " <<"P0" <<" : MPI_Init" << endl;


}

void MPISequenceOutputWriter::end()
{
/*	for (int i = 0; i < mNumProcesses; i++)
	{
		mOutStream <<"P"<<i;
		if (i != mNumProcesses - 1)
			mOutStream<<",";
	}
	*/
	//mOutStream<< "note over P0"<<": MPI_Finalize"<<endl;
	mOutStream <<"@enduml"<<endl;
	mOutStream.close();
}

void MPISequenceOutputWriter::writeSpace()
{
	mOutStream << "|||"<<endl;
}

void MPISequenceOutputWriter::write(const DebugBaseInfo& ev)
{
	assert(mOutStream.is_open());

	switch(ev.mType)
	{

	case DET_SEND:
	case DET_ISEND:
		{
			//const char* sourceStr = getNameFromId(ev.mProcessId);
			const char* destStr = getNameFromId(ev.mDest);
			const char* attributesStr = getAttributesStr(ev);
			const char* descriptionStr = getDetailsStr(ev);

			mOutStream << "P"<<ev.mProcessId <<" -" << attributesStr << ">" << destStr << " : " << getDebugEventTypeString(ev.mType)<< " " << descriptionStr<< endl;
		}
		break;
	case DET_RECV:
	case DET_IRECV:
		{
			//const char* sourceStr = getNameFromId(ev.mProcessId);
			const char* destStr = getNameFromId(ev.mDest);
			const char* attributesStr = getAttributesStr(ev);
			const char* descriptionStr = getDetailsStr(ev);

			mOutStream << "P" << ev.mProcessId <<" -" << attributesStr << ">"<< destStr <<" : " << getDebugEventTypeString(ev.mType)<< " " << descriptionStr <<endl;
		}
		break;
	case DET_WAITALL:
		{
			// TODO:
			//mOutStream << "P" << ev.mProcessId <<" -->]: MPI_Waitall"<< getDetailsStr(ev) <<endl;

			mOutStream << "P" << ev.mProcessId <<" -> " << "P" << ev.mProcessId << ": MPI_Waitall"<< getDetailsStr(ev) <<endl;
		}
		break;
	case DET_GATHER:
		{
			if (ev.mProcessId == ev.mDest)
			{	
				// TODO:
				mOutStream << "== GATHER " << getDetailsStr(ev) << " ==" << endl;
			}
		}
		break;
	case DET_SCATTER:
		{
			if (ev.mProcessId == ev.mDest)
			{	
				mOutStream <<"== SCATTER " << getDetailsStr(ev) <<" =="<<endl;
			}
		}
		break;
	case DET_BARRIER:
		{
			//if (ev.mProcessId == 0) // Improve this !
			{	
				mOutStream <<"== BARRIER " << getDetailsStr(ev) << " ==" << endl;
			}
		}
		break;
	case DET_NOTE:
		{
			mOutStream<<"note ";
			switch(ev.mTag)
			{
			case NOTE_CENTER:
				mOutStream<<"over ";
				break;
			case NOTE_LEFT:
				mOutStream<<"left of ";
				break;
			case NOTE_RIGHT:
				mOutStream<<"right of ";
				break;
			default:
				assert(false);
			}

			mOutStream<<"P"<<ev.mDest<<" : " << ev.mShortDescription<<endl;
		}
		break;
	case DET_CYCLE_MANUAL_START:
		{
			mOutStream<<"loop iterations: " << ev.mTag<<endl;
		}
		break;
	case DET_CYCLE_MANUAL_END:
		{
			mOutStream<<"end"<<endl;
		}
		break;
	default:
		assert(false && "not supported yet");
	}
}

const char* MPISequenceOutputWriter::getNameFromId(const int processId)
{
	static char sbuff[1024];
	if (processId == MPI_ANY_SOURCE)
		sprintf_s(sbuff, 1024, "PANY");
	else
		sprintf_s(sbuff, 1024, "P%d", processId);
	return sbuff;
}

const char* MPISequenceOutputWriter::getAttributesStr(const DebugBaseInfo& ev)
{
	const int nm = 1024;
	static char sbuff[nm];
	sbuff[0] = 0;

	// Fill color only for now...
	switch(ev.mType)
	{
	case DET_SEND:
	case DET_ISEND:
		{
			sprintf_s(sbuff, nm, "[#green]");
		}
		break;
	case DET_RECV:
	case DET_IRECV:
		{
			sprintf_s(sbuff, nm, "[#red]");
		}
		break;
	case DET_GATHER:
	case DET_BARRIER:
	case DET_WAITALL:
	case DET_CYCLE_MANUAL_START:
	case DET_CYCLE_MANUAL_END:
		{

		}
		break;
	default:
		assert(false && "not supported yet");
	}

	return sbuff;
}

const char* MPISequenceOutputWriter::getDetailsStr(const DebugBaseInfo& ev)
{
	// Iterate over options and fill a buffer
	const int nm = 4096;
	static char sbuff[nm];
	sbuff[0] = 0;

	std::ostringstream outStr;

	// Filename and linenum 
	if (mOptions.showFileNameAndNum)
	{
		if (ev.mFileName) outStr << " [" << ev.mFileName;
		outStr << "-" << ev.mLineNum << "] ";
	}
	
	switch(ev.mType)
	{
	case DET_SEND:
	case DET_RECV:
	case DET_ISEND:
	case DET_IRECV:
		{		
			if (mOptions.showTag)
				outStr << " Tag: "<< ev.mTag;		
		}
		break;
	
	case DET_BARRIER:
		{
			if (mOptions.timeSpent)
			{
				outStr<<"\\nLongest wait: P" << ev.mProcessId<< "-" << (ev.mEndTime - ev.mStartTime);
			}
		}
		break;
	case DET_WAITALL:
		{
			
		}
		break;

	case DET_GATHER:
	case DET_SCATTER:
		{
			if (mOptions.showTag)
				outStr<<"Root: P" << ev.mTag;
		}
		break;
	default:
		assert(false);
	}

	//if (ev.mType != DET_BARRIER && ev.mType != DET_WAITALL)
	{
		//if (mOptions.startEndTime || mOptions.timeSpent)
		//	outStr <<"\\n";

		if (mOptions.startEndTime)
			outStr <<" Time[" << ev.mStartTime <<":"<<ev.mEndTime<<"] ";
	
		if (ev.mType == DET_WAITALL) 
			outStr <<"\\n";

		if (mOptions.timeSpent)
			outStr <<" Duration:" << (ev.mEndTime - ev.mStartTime);
	}

	if (mOptions.description && ev.mShortDescription)
	{
		if (ev.mType != DET_BARRIER && ev.mType != DET_SCATTER && ev.mType != DET_GATHER)
			outStr<<"\\n";
		else 
			outStr <<" ";

		outStr << ev.mShortDescription;	// TODO: can this be set as a note 
	}

	strcpy_s(sbuff, nm, outStr.str().c_str());
	return sbuff;
}
