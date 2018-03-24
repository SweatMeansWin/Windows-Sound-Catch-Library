#pragma once

class CAudioResampler
{
public:
	static void ProcessBuffer(unsigned char* pData, int dataLen, int inRate, int outRate, unsigned char* out, size_t* outCount);
	static int  GetOutLen(int dataLen, int inRate, int outRate);
};

