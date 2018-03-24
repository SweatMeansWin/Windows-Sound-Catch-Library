#pragma once
#include "lame\lame.h"
#include "lame\applame.h"

class CLameWrapper
{
public:
	static size_t LameReadBuffer(void *buffer, size_t size, size_t count, FILE *stream, char* pData, int dataLen);
	static lame_t LameInitHeader(const char* strFileName);
	static void LameFinalize(lame_t  gf, unsigned char* mp3buffer, int mp3bufferLength);
};

