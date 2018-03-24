#include "LameWrapper.h"

#include <vector>

lame_t CLameWrapper::LameInitHeader(const char* strFileName)
{
	char szOutFilePath[255] = { 0 };
	sprintf_s(szOutFilePath, "xxx");

	const char* argv[4] = { "lame", "-b64 -V9", strFileName, szOutFilePath };

	lame_t gf = lame_init(); /* initialize libmp3lame */
	if (!gf)
	{
		printf("fatal error during initialization\n");
		return nullptr;
	}
	lame_main(gf, 4, (char**)argv);
	lame_read_sample_buffer = LameReadBuffer;

	return gf;
}

size_t CLameWrapper::LameReadBuffer(void * buffer, size_t size, size_t count, FILE * stream, char * pData, int dataLen)
{
	std::vector<unsigned char>* tempData = (std::vector<unsigned char>*)pData;

	if (tempData->size() == 0)
		return -1;

	size_t result = 0;
	size_t dataLength = size * count;

	if (tempData->size() >= dataLength)
		result = dataLength;
	else
		result = tempData->size();

	memcpy_s(buffer, dataLength, tempData->data(), result);
	tempData->erase(tempData->begin(), tempData->begin() + result);
	return result / size;
}

void CLameWrapper::LameFinalize(lame_t gf, unsigned char * mp3buffer, int mp3bufferLength)
{
	lame_encode_flush(gf, mp3buffer, mp3bufferLength);
	lame_close(gf);
	gf = nullptr;
}