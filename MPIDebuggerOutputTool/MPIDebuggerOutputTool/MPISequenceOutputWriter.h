#pragma once

#include <fstream>

class DebugBaseInfo;

class MPISequenceOutputWriter 
{
public:
	struct Options
	{
		bool showFileNameAndNum;
		bool showTag;
		bool startEndTime;
		bool timeSpent;
		bool description;		

		Options() { Reset();}

		void Reset()
		{
			memset(this, 0, sizeof(*this));
		}
	};

	MPISequenceOutputWriter(){}

	void begin(const char* filename, const char* title);
	void end();

	void write(const DebugBaseInfo& ev);
	void writeSpace();

	void init(int numProcesses, Options opt = Options())
	{
		mOptions = opt; 
		mNumProcesses = numProcesses;
	}
	
	// TODO: add custom write event sync or something..., note ?!
	

private:
	const char* getNameFromId(const int proccessId);
	const char* getAttributesStr(const DebugBaseInfo& ev);
	const char* getDetailsStr(const DebugBaseInfo& ev);

	std::ofstream	mOutStream;	
	Options			mOptions;
	int				mNumProcesses;
};

