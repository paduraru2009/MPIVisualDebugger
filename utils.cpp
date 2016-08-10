#include "utils.h"
#include <string.h>
#include <math.h>

// TODO: bring funcs here from utils.h

long long Utils::m_sBaseTime = 0;

char* Utils::getFileName(const char* fullPath)
{
	//return const_cast<char*>(fullPath);

	if (fullPath == nullptr)
		return nullptr;

	const char* lastChar = fullPath;
	while(*lastChar != '\0') lastChar++;

	int numCharsInFileName = 0;	// end
	while(lastChar != fullPath)
	{
		const char chr = *lastChar;
		if (chr == '\\' || chr =='/')
		{
			lastChar++;
			break;
		}

		lastChar--;
		numCharsInFileName++;
	}

	char* newStr = new char[numCharsInFileName];
	strncpy_s(newStr, numCharsInFileName, lastChar, numCharsInFileName);
	return newStr;
}

long long Utils::convertWallDoubleTimeToLongInt(const double doubleWallTime)
{
	const double remainder = doubleWallTime - floor(doubleWallTime);
	return (((long long)doubleWallTime)*MPIDEBUGGER_TIME_PRECISION + (long long)(remainder*MPIDEBUGGER_TIME_PRECISION));
}

void Utils::startTimer()
{
	m_sBaseTime = convertWallDoubleTimeToLongInt(get_wall_time());
}

