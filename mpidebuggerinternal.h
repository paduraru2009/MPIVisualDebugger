#pragma once
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include "utils.h"
#include "mpidebuggercommons.h"

struct MPI_Status;

//#define WRITE_NUMBER_OF_RECORDS_IN_BINARY_FILE

class MPIDebugger
{
public:
	struct MPIDebuggerConfig
	{
		bool flushAfterEveryMsg;
		// TODO: add display options here
	};

	MPIDebugger(const MPIDebuggerConfig& config);
	virtual ~MPIDebugger();

	typedef std::vector<DebugBaseInfo> DebugItemsArray;
	typedef DebugItemsArray::iterator DebugItemsArrayIter;
	
	static MPIDebugger& getInst() 
	{ 
		assert((m_inst != nullptr)); //"call init first"; 
		return *m_inst; 
	}

	static void init(MPIDebuggerConfig& config = MPIDebuggerConfig())
	{
		assert(m_inst == nullptr);
		m_inst = new MPIDebugger(config);
	}

	static void destroy()
	{
		m_inst->destroyInternal();
	}
	
	void destroyInternal();

	int Send(const void* buf, int count, int datatype, int dest, int tag, int comm);
	int Recv(void* buf, int count, int datatype, int dest, int tag, int comm, MPI_Status* status);

	int ISend(const void* buf, int count, int datatype, int dest, int tag, int comm, MPI_Request* request);
	int IRecv(void* buf, int count, int datatype, int dest, int tag, int comm, MPI_Request* request);

	int Barrier(int comm);
	int Gather(const void *sendbuf, int sendcount, int sendtype, void *recvbuf, int recvcount, int recvtype, int root, int comm);
	int Scatter(const void *sendbuf, int sendcount, int sendtype, void *recvbuf, int recvcount, int recvtype, int root, int comm);
	int WaitAll(int count, MPI_Request* array_of_requests, MPI_Status* array_of_statuses);

	void StartCall(const DebugBaseInfo& info);
	void OnRightBeforeCall();
	void EndCall();
	void injectManualEvent(const DebugBaseInfo& info);

	// Adds a note
	static DebugBaseInfo buildNote(int rank, NoteAlignment alignment,  char* note, const char* fileName, const int lineNum, long long time);
	static DebugBaseInfo buildLoopForAll(const LoopType loopType, const DebugBaseInfo& info);
	static DebugBaseInfo buildBarrier(const DebugBaseInfo& info);

	static void terminate(){ m_inst->flushLastEvents(); delete m_inst; m_inst = nullptr; }
	void flushLastEvents();
	void modifyOnEndLastEvent();
	void setRankInternal(const int rank);
	static void setRank(const int rank);	
	int getRank() { return mRank; }

#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
	static void writeTextEvent(std::ofstream& fileRef, const DebugBaseInfo& ev);
#endif

	static void writeBinaryEventBlob(std::ofstream& fileRef, const DebugBaseInfo& ev);
	static bool readBinaryEventBlob(std::ifstream& fileRef, DebugBaseInfo& outEv);	// False if end of the file

private:
	static MPIDebugger* m_inst;

#ifdef MPIDEBUGGER_TEXT_OUTPUT_ENABLED
	std::ofstream mOutEventsFileText;
#endif

	std::ofstream mOutEventsFileBinary;

	DebugItemsArray mEntries;
	unsigned int mNextEventToFlush;
	bool mFlushAfterEachEvent;

	DebugBaseInfo m_tempDebugInfo; // Current temp buffer to fill
	MPIDebuggerConfig mConfig;

	/// Call state vars  
	bool m_isInCall;
	int mRank;
	//const DebugBaseInfo* m_currentDebugInfo;

	///
};

// TODO: need to use MPI base types !
//-------------------------------------------------
int  MPID_Send(const void* buf, int count, int datatype, int dest, int tag, int comm,  DebugBaseInfo&& baseDebugInfo = DebugBaseInfo ());

int MPID_Recv(void* buf, int count, int datatype, int source, int tag, int comm, MPI_Status* status, DebugBaseInfo&& baseDebugInfo = DebugBaseInfo ());

int MPID_ISend(const void* buf, int count, int datatype, int dest, int tag, int comm, MPI_Request* request, const DebugBaseInfo& info = DebugBaseInfo ());
int MPID_IRecv(void* buf, int count, int datatype, int source, int tag, int comm,  MPI_Request* request, const DebugBaseInfo& info = DebugBaseInfo());
int MPID_Barrier(int comm);
int MPID_Gather(const void *sendbuf, int sendcount, int sendtype, void *recvbuf, int recvcount, int recvtype, int root, int comm, const DebugBaseInfo& info = DebugBaseInfo());
int MPID_Scatter(const void *sendbuf, int sendcount, int sendtype, void *recvbuf, int recvcount, int recvtype, int root, int comm, const DebugBaseInfo& info = DebugBaseInfo());


int MPID_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[], const DebugBaseInfo& info = DebugBaseInfo());
int MPID_Barrier(int comm, const DebugBaseInfo& info = DebugBaseInfo() );

//-------------------------------------------------

