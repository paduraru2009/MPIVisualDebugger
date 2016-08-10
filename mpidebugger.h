#pragma once


#include "mpidebuggerinternal.h"
#include "mpihooks.h"


#ifdef USING_MPI_DEBUGGER
	#define MDEBUGCALL(Func, Param) MPIDebugger::getInst().StartCall(Param); Func;  MPIDebugger::getInst().EndCall()
#else 
	#define MDEBUGCALL(Func, Param)  Func; 
#endif
