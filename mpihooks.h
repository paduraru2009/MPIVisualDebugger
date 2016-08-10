#pragma once

// This file contains different hooks for directing MPI calls to our debugger first

#define MPI_Send MPID_Send
#define MPI_Recv MPID_Recv

#define MPI_Isend MPID_ISend
#define MPI_Irecv MPID_IRecv
#define MPI_Barrier MPID_Barrier
#define MPI_Waitall MPID_Waitall
#define MPI_Gather MPID_Gather
#define MPI_Scatter MPID_Scatter

#define MPIDEBUGGER_LOOP_START						MPIDEBUGGER_LOOP(DET_CYCLE_MANUAL_START)
#define MPIDEBUGGER_LOOP_END						MPIDEBUGGER_LOOP(DET_CYCLE_MANUAL_END)

#define MPIDEBUGGER_LOOP(LOOP_TYPE)					\
	{												\
	DebugBaseInfo info;								\
	info.mFileName = Utils::getFileName(__FILE__);	\
	info.mLineNum = __LINE__;						\
	info.mType = LOOP_TYPE;							\
	info.mStartTime = info.mEndTime = Utils::getDebuggerTime(); \
	MPIDebugger::getInst().injectManualEvent(info);	\
	}


#define MPIDEBUGGER_NOTE(rnk, strArgs, ...) \
	{	if (rnk == MPIDebugger::getInst().getRank()) {	\
		char buff[512];	\
		sprintf_s(buff, 512, strArgs, ## __VA_ARGS__);	\
		const DebugBaseInfo info = MPIDebugger::getInst().buildNote(rnk, NOTE_CENTER, buff, Utils::getFileName(__FILE__), __LINE__, Utils::getDebuggerTime());  \
		MPIDebugger::getInst().injectManualEvent(info);	\
		}	\
	}

#define CUSTOM_TEXT(strArgs, ...)	\
		DebugBaseInfo(__FILE__, __LINE__, strArgs, __VA_ARGS__)

