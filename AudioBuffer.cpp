#include <algorithm>

#include "AudioBuffer.h"
#include "AudioResampler.h"
#include "WaveOperations.h"


struct TThreadParams
{
	CAudioBuffer* Instance;
	std::wstring FileName;
};

CAudioBuffer::CAudioBuffer() : m_bInit(false), m_pSoundBuffer(nullptr), m_sampleRate(44100), mp3BufferLength(LAME_MAXMP3BUFFER)
{
	mp3Buffer = new unsigned char[LAME_MAXMP3BUFFER];
}

CAudioBuffer::~CAudioBuffer()
{
	ReleaseBuffer();
}

bool CAudioBuffer::InitBuffer(WAVEFORMATEX * pFormat, const std::wstring& sFileName)
{
	if (m_bInit)
	{
		return false;
	}

	m_targetFileName = sFileName;
	m_format = *pFormat;
	m_pSoundBuffer = new TSoundBuffer();
	m_pSoundBuffer->sampleRate = pFormat->nSamplesPerSec;
	m_pSoundBuffer->numChannels = pFormat->nChannels;
	m_pSoundBuffer->sampleSize = pFormat->wBitsPerSample / 8;
	m_pSoundBuffer->blockAlign = pFormat->nBlockAlign;

	MMIOINFO mi = { 0 };
	HMMIO hFile = mmioOpen(const_cast<LPWSTR>(sFileName.c_str()), &mi, MMIO_WRITE | MMIO_CREATE);
	if (!hFile)
		return false;

	MMCKINFO ckRIFF = { 0 };
	MMCKINFO ckData = { 0 };

	HRESULT hr = WriteWaveHeader(hFile, pFormat, &ckRIFF, &ckData);
	hr = WriteWaveHeader(hFile, pFormat, &ckRIFF, &ckData);

	mmioClose(hFile, 0);

	TCHAR tempDirectory[MAX_PATH] = { 0 };
	GetTempPath(MAX_PATH, tempDirectory);
	TCHAR tempFileName[MAX_PATH] = { 0 };
	GetTempFileName(tempDirectory, L"wx", 0, tempFileName);

	m_tempFileName = tempFileName;

	m_bInit = true;
	return m_bInit;
}

void CAudioBuffer::ReleaseBuffer()
{
	if (m_pSoundBuffer)
	{
		delete m_pSoundBuffer;
		m_pSoundBuffer = nullptr;
	}
	if (mp3Buffer)
	{
		delete mp3Buffer;
		mp3Buffer = nullptr;
	}

	m_tempFileName.clear();
	m_targetFileName.clear();
	m_data.clear();

	m_bInit = false;
}

bool CAudioBuffer::InitLame(const std::wstring& sFileName)
{
	std::string s(sFileName.cbegin(), sFileName.cend());
	m_gf = CLameWrapper::LameInitHeader(s.c_str());

	DeleteFile(sFileName.c_str());

	return (m_gf != nullptr);
}

bool CAudioBuffer::FinalizeLame()
{
	CLameWrapper::LameFinalize(m_gf, mp3Buffer, mp3BufferLength);

	return true;
}

void CAudioBuffer::AddData(unsigned char* pData, size_t len)
{
	//if (m_pSoundBuffer->numChannels != 2)
	//	expandTo2ChannelSound(pData, len);

	int origLen = len;
	if (m_pSoundBuffer->sampleRate != m_sampleRate)
	{
		len = CAudioResampler::GetOutLen(len, m_pSoundBuffer->sampleRate, m_sampleRate);
		if (pData != NULL)
		{
			if (m_pSoundBuffer->resampleBuffLen < len)
			{
				if (m_pSoundBuffer->pResampleBuff)
					delete[] m_pSoundBuffer->pResampleBuff;
				m_pSoundBuffer->pResampleBuff = new BYTE[len];
				m_pSoundBuffer->resampleBuffLen = len;
			}

			BYTE* p = m_pSoundBuffer->pResampleBuff;
			CAudioResampler::ProcessBuffer(pData, origLen, m_pSoundBuffer->sampleRate, m_sampleRate, p, &len);
			pData = p;
		}
	}

	int extra = m_pSoundBuffer->position + len - m_data.size();

	if (extra > 0)
		m_data.insert(m_data.end(), extra, 0);

	if (pData != NULL)
	{
		BYTE* data = m_data.data() + m_pSoundBuffer->position;
		BYTE* p = pData;
		while (p < pData + len)*data++ += *p++;
	}

	m_pSoundBuffer->position += len;
}

void CAudioBuffer::SaveData()
{
	std::wstring sMp3FileName = m_targetFileName;
	sMp3FileName += L".mp3";

	FILE* mp3File = NULL;
	if (_wfopen_s(&mp3File, sMp3FileName.c_str(), L"wb") == 0)
	{
		FILE* tempFile;
		if (_wfopen_s(&tempFile, m_tempFileName.c_str(), L"rb") == 0)
		{
			BYTE buffer[4960];

			while (true)
			{
				int c = fread(buffer, 1, sizeof(buffer), tempFile);
				if (c > 0)
					fwrite(buffer, 1, c, mp3File);
				else
					break;
			}
			fclose(tempFile);
		}
		fclose(mp3File);
	}

	DeleteFile(m_tempFileName.c_str());
}

bool CAudioBuffer::ProcessBuffer()
{
	if (m_data.empty())
		return false;

	m_pSoundBuffer->position = 0;

	FILE* file = nullptr;
	if (_wfopen_s(&file, m_tempFileName.c_str(), L"a+b") == 0)
	{
		int buffer[2][1152];

		int iread = 0;
		do {
			memset(buffer[0], 0, sizeof(buffer[0]));
			memset(buffer[1], 0, sizeof(buffer[1]));

			iread = get_audio(m_gf, buffer, (char*)&m_data, m_data.size());
			if (iread > 0)
			{
				int imp3 = lame_encode_buffer_int(m_gf, buffer[0], buffer[1], iread, mp3Buffer, mp3BufferLength);
				size_t owrite = fwrite(mp3Buffer, 1, imp3, file);
			}
		} while (iread > 0);

		fclose(file);
	}

	m_data.clear();

	return true;
}

void CAudioBuffer::expandTo2ChannelSound(unsigned char* pData, size_t& len)
{
	unsigned int newLen = (len / m_pSoundBuffer->blockAlign) * 2 * m_pSoundBuffer->sampleSize;
	if (pData != NULL)
	{
		if (m_pSoundBuffer->channelsBuffLen < newLen)
		{
			if (m_pSoundBuffer->pChannelsBuff)
				delete[] m_pSoundBuffer->pChannelsBuff;
			m_pSoundBuffer->pChannelsBuff = new BYTE[newLen];
			m_pSoundBuffer->channelsBuffLen = newLen;
		}

		BYTE* pNewData = m_pSoundBuffer->pChannelsBuff;

		BYTE* pDataEnd = pData + len;
		BYTE* p = pData; BYTE* pNew = pNewData;

		if (m_pSoundBuffer->numChannels > 2)
		{
			while (p != pDataEnd)
			{
				memcpy(pNew, p, m_pSoundBuffer->sampleSize * 2);

				pNew += m_pSoundBuffer->sampleSize * 2;
				p += m_pSoundBuffer->blockAlign;
			}
		}
		if (m_pSoundBuffer->numChannels == 1)
		{
			while (p != pDataEnd)
			{
				memcpy(pNew, p, m_pSoundBuffer->sampleSize);
				pNew += m_pSoundBuffer->sampleSize;
				memcpy(pNew, p, m_pSoundBuffer->sampleSize);
				pNew += m_pSoundBuffer->sampleSize;

				p += m_pSoundBuffer->blockAlign;
			}
		}
		pData = pNewData;
	}
	len = newLen;
}

CAudioBuffer::TSoundBuffer::TSoundBuffer()
	: sampleSize(0),
	sampleRate(0),
	numChannels(0),
	blockAlign(0),
	position(0),
	channelsBuffLen(0),
	resampleBuffLen(0),
	pChannelsBuff(nullptr),
	pResampleBuff(nullptr)
{ }
