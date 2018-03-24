#pragma once
#define NOMINMAX
#include <Windows.h>
#include <Mmsystem.h>

#include <vector>

#include "LameWrapper.h"

class CAudioBuffer
{
public:
	CAudioBuffer();
	~CAudioBuffer();

	bool InitBuffer(WAVEFORMATEX* pFormat, const std::wstring& sFileName);
	void ReleaseBuffer();

	bool InitLame(const std::wstring& sFileName);
	bool FinalizeLame();

	void AddData(unsigned char* pData, size_t len);
	void SaveData();

	bool ProcessBuffer();
private:
	struct TSoundBuffer
	{
		size_t position;
		unsigned int sampleRate, numChannels, sampleSize, blockAlign;

		unsigned char* pChannelsBuff;
		size_t channelsBuffLen;

		unsigned char* pResampleBuff;
		size_t resampleBuffLen;

		TSoundBuffer();
	};

	bool				m_bInit;
	std::wstring		m_targetFileName;
	std::wstring		m_tempFileName;
	unsigned int		m_sampleRate;
	lame_t				m_gf;
	TSoundBuffer*		m_pSoundBuffer;
	WAVEFORMATEX		m_format;

	void expandTo2ChannelSound(unsigned char* pData, size_t& len);
	//data for internal processing
	std::vector<unsigned char>	m_data;
	unsigned char				*mp3Buffer;
	unsigned long				mp3BufferLength;
};

