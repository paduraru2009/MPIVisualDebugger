#pragma once

#define MPIDEBUGGER_TEXT_OUTPUT_ENABLED
#define DEBUGGER_INVALID_TIME -1

enum DebugEventType
{
	DET_SEND = 0,
	DET_RECV,
	DET_ISEND,
	DET_IRECV,
	DET_BARRIER,
	DET_WAITALL,
	DET_GATHER,
	DET_SCATTER,
	DET_NOTE,
	DET_CYCLE,
	DET_CYCLE_MANUAL_START,
	DET_CYCLE_MANUAL_END,
	DET_NUM
};

enum NoteAlignment
{
	NOTE_LEFT,
	NOTE_RIGHT,
	NOTE_CENTER,
};

enum LoopType
{
	LOOP_BEGIN,
	LOOP_END,
};

const char* getDebugEventTypeString(DebugEventType type);

class DebugBaseInfo
{
public:
	DebugBaseInfo() {mFileName = nullptr; mShortDescription = nullptr; reset();}
	DebugBaseInfo(char* filename, const int lineNum, char* shortDescription);
	DebugBaseInfo(char* filename, const int lineNum, char* shortDescription, ...);

	/*DebugBaseInfo& operator=(const DebugBaseInfo& other)
	{
		reset();
		mFileName = other.mFileName; // ? Utils::getFileName(other.mFileName) : nullptr;
		mShortDescription = other.mShortDescription; // ? _strdup(other.mShortDescription) : nullptr;

		mLineNum = other.mLineNum;
		return *this;
	}
	*/

	void reset()
	{
		
		mFileName = nullptr;
		mLineNum = 0;
		
		mTag = mDest = 0;
		mStartTime = 0;
		mProcessId = -1;
		mPrev = -1;
		mPrevTag = -1;
		mNext = -1;
		mNextTag = -1;
	}

	void destroy()
	{
		//if (mFileName) { delete [] mFileName;  mFileName = nullptr; }
		//if (mShortDescription) { delete [] mShortDescription; mShortDescription = nullptr; }
	}

	~DebugBaseInfo() { reset(); }

	bool compareInCycle(const DebugBaseInfo& other) const;
	bool compareManualCycle(const DebugBaseInfo& other) const;
	bool compareBarrier(const DebugBaseInfo& other) const;

	char* mFileName;
	unsigned int mLineNum;
	char* mShortDescription;
	DebugEventType mType;

//private:
	// Private variables will be filled internally by MPIDebugger. Public one will be copied from client code
	bool mActivated;
	long long mStartTime;
	long long mEndTime;
	int mDest;  // also used as loop size // TODO: add an union !
	int mTag;  // also used as counter for loops
	int mProcessId; 
	int mPrev;
	int mPrevTag;
	int mNext;
	int mNextTag;
};

