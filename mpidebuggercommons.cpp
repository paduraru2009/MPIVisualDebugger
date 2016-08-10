#include "mpidebuggercommons.h"
#include <assert.h>
#include "utils.h"
#include <stdarg.h>

char* debugEventsStr[] =
{
	"SEND",
	"RECV",
	"ISEND",
	"IRECV",
	"BARRIER",
	"WAITALL",
	"GATHER",
	"SCATTER",
	"NOTE",
	"CYCLE",
	"CYCLE_MANUAL_START",
	"CYCLE_MANUAL_END",
};

COMPILE_TIME_ASSERT((sizeof(debugEventsStr)/sizeof(debugEventsStr[0]) == DET_NUM));
const char* getDebugEventTypeString(DebugEventType type) { return debugEventsStr[type]; }


bool DebugBaseInfo::compareInCycle(const DebugBaseInfo& other) const
{
	if (mType != other.mType)
		return false;

	if (_stricmp(mFileName, other.mFileName) !=0 || mLineNum != other.mLineNum)
		return false;		

	return true;
}

bool DebugBaseInfo::compareManualCycle(const DebugBaseInfo& other) const
{
	//assert(mType == DET_CYCLE_MANUAL_START && other.mType == DET_CYCLE_MANUAL_START);
	if (_stricmp(mFileName, other.mFileName) !=0 || mLineNum != other.mLineNum)
		return false;		
	
	return true;
}

bool DebugBaseInfo::compareBarrier(const DebugBaseInfo& other) const
{
	if (_stricmp(mFileName, other.mFileName) !=0 || mLineNum != other.mLineNum)
		return false;		

	return true;
}

DebugBaseInfo::DebugBaseInfo(char* filename, const int lineNum, char* shortDescription, ...)
{
	static char buff[512];
	buff[0] = 0;
	va_list args;
	va_start(args, shortDescription);
	vsnprintf_s(buff, 512, 512, shortDescription, args);
	va_end(args);


	mFileName = nullptr; mShortDescription = nullptr;
	reset();
	mFileName = Utils::getFileName(filename);
	mLineNum = lineNum;
	mActivated = true;


	mShortDescription = strdup(buff);
	mStartTime = DEBUGGER_INVALID_TIME;
	mEndTime = DEBUGGER_INVALID_TIME;
}

DebugBaseInfo::DebugBaseInfo(char* filename, const int lineNum, char* shortDescription)
{
	mFileName = nullptr; mShortDescription = nullptr;
	reset();
	mFileName =  Utils::getFileName(filename);
	mLineNum = lineNum;
	mActivated = true;
	mShortDescription = shortDescription;
	mStartTime = DEBUGGER_INVALID_TIME;
	mEndTime = DEBUGGER_INVALID_TIME;
}

