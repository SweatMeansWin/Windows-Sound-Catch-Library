#include "SoundCapturer.h"

#include <ctime>

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

#pragma region DEFINITIONS

const unsigned int REFTIMES_PER_SEC = 10000000;
const unsigned int REFTIMES_PER_MILLISEC = 10000;
const unsigned int BUFFER_SIZE_SEC = 2;

const REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC * BUFFER_SIZE_SEC;

#pragma endregion


CSoundCapturer::CSoundCapturer()
	: m_bInit(false),
	m_bWork(false),
	m_dwError(ERROR_SUCCESS),
	m_hrError(S_OK),
	m_hSyncEvent(nullptr),
	m_hThread(nullptr)
{
	InitializeCriticalSection(&m_guard);
}

CSoundCapturer::~CSoundCapturer()
{
	DeInitialize();
	DeleteCriticalSection(&m_guard);
}

CSoundCapturer::TCaptureObjects::TCaptureObjects()
	: pDevice(nullptr),
	pAudioClient(nullptr),
	pCaptureClient(nullptr),
	pWaveFmt(nullptr),
	bufferFrameCount(0),
	Enabled(false)
{

}

CSoundCapturer::TCaptureObjects::~TCaptureObjects()
{
	Release();
}

void CSoundCapturer::TCaptureObjects::Release()
{
	if (pWaveFmt)
	{
		CoTaskMemFree(pWaveFmt);
		pWaveFmt = nullptr;
	}
	SAFE_RELEASE(pDevice);
	SAFE_RELEASE(pAudioClient);
	SAFE_RELEASE(pCaptureClient);
	Enabled = false;
}

bool CSoundCapturer::initOutputAudio(IMMDeviceEnumerator* pEnumerator)
{
	//Get IMMDevice
	HRESULT hr = pEnumerator->GetDefaultAudioEndpoint(
		eRender,
		eConsole,
		&m_outputCapture.pDevice);
	if (FAILED(hr))
		return false;

	//Get IAudioClient
	hr = m_outputCapture.pDevice->Activate(
		IID_IAudioClient,
		CLSCTX_ALL,
		NULL,
		(void**)&m_outputCapture.pAudioClient);
	if (FAILED(hr))
		return false;

	//Receive system waveformat
	hr = m_outputCapture.pAudioClient->GetMixFormat(&m_outputCapture.pWaveFmt);
	if (FAILED(hr))
		return false;

	//GET COMMON WAVEFORMATEX TO RECORD SAME AUDIO
	hr = m_outputCapture.pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		hnsRequestedDuration,
		0,
		m_outputCapture.pWaveFmt,
		NULL);
	if (FAILED(hr))
		return false;

	// Get the size of the allocated buffer (in frames)
	hr = m_outputCapture.pAudioClient->GetBufferSize(&m_outputCapture.bufferFrameCount);
	if (FAILED(hr))
		return false;

	hr = m_outputCapture.pAudioClient->GetService(
		IID_IAudioCaptureClient,
		(void**)&m_outputCapture.pCaptureClient);
	if (FAILED(hr))
		return false;

	m_outputCapture.Enabled = true;
	return m_outputCapture.Enabled;
}

bool CSoundCapturer::initInputAudio(IMMDeviceEnumerator* pEnumerator)
{
	//Get IMMDevice
	HRESULT hr = pEnumerator->GetDefaultAudioEndpoint(
		eCapture,
		eConsole,
		&m_inputCapture.pDevice);
	if (FAILED(hr))
	{
		m_inputCapture.Enabled = false;
		return true;
	}

	//Get IAudioClient
	hr = m_inputCapture.pDevice->Activate(
		IID_IAudioClient,
		CLSCTX_ALL,
		NULL,
		(void**)&m_inputCapture.pAudioClient);
	if (FAILED(hr))
		return false;

	//Receive system waveformat
	hr = m_inputCapture.pAudioClient->GetMixFormat(&m_inputCapture.pWaveFmt);
	if (FAILED(hr))
		return false;

	hr = m_inputCapture.pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		0,
		hnsRequestedDuration,
		0,
		m_inputCapture.pWaveFmt,
		NULL);
	if (FAILED(hr))
		return false;

	// Get the size of the allocated buffer (in frames)
	hr = m_inputCapture.pAudioClient->GetBufferSize(&m_inputCapture.bufferFrameCount);
	if (FAILED(hr))
		return false;

	hr = m_inputCapture.pAudioClient->GetService(
		IID_IAudioCaptureClient,
		(void**)&m_inputCapture.pCaptureClient);
	if (FAILED(hr))
		return false;

	m_inputCapture.Enabled = true;
	return m_inputCapture.Enabled;
}

bool CSoundCapturer::Initialize()
{
	if (m_bInit)
	{
		m_dwError = ERROR_ALREADY_INITIALIZED;
		return false;
	}

	IMMDeviceEnumerator *pEnumerator = nullptr;
	HRESULT hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator,
		NULL,
		CLSCTX_ALL,
		IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	if (FAILED(hr))
		goto EXIT_FAILED;

	if (!initInputAudio(pEnumerator))
		goto EXIT_FAILED;

	if (!initOutputAudio(pEnumerator))
		goto EXIT_FAILED;

	m_hSyncEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_inputBuffer = new CAudioBuffer();
	m_outputBuffer = new CAudioBuffer();

	m_hrError = hr;
	m_dwError = ERROR_SUCCESS;

	m_bInit = true;
	return m_bInit;

EXIT_FAILED:

	SAFE_RELEASE(pEnumerator);
	releaseAudioObjects();

	m_hrError = hr;
	m_dwError = ERROR_INSTALL_FAILED;
	return false;
}

void CSoundCapturer::releaseAudioObjects()
{
	m_outputCapture.Release();
	m_inputCapture.Release();
}

void CSoundCapturer::DeInitialize()
{
	if (!m_bInit)
		return;

	StopRecord();

	if (m_inputBuffer)
	{
		m_inputBuffer->ReleaseBuffer();
		delete m_inputBuffer;
		m_inputBuffer = nullptr;
	}
	if (m_outputBuffer)
	{
		m_outputBuffer->ReleaseBuffer();
		delete m_outputBuffer;
		m_outputBuffer = nullptr;
	}
	if (m_hSyncEvent)
	{
		CloseHandle(m_hSyncEvent);
		m_hSyncEvent = nullptr;
	}

	releaseAudioObjects();

	m_hrError = S_OK;
	m_dwError = ERROR_SUCCESS;
	m_bInit = false;
}

bool CSoundCapturer::StartRecord(TRecordSettings& settings)
{
	if (!m_bInit)
	{
		m_dwError = ERROR_NOT_READY;
		return false;
	}
	if (m_bWork)
	{
		m_dwError = ERROR_ALREADY_INITIALIZED;
		return false;
	}

	EnterCriticalSection(&m_guard);
	{
		std::wstring sOutputFilename = settings.sFileName + L"_o";

		m_outputBuffer->InitBuffer(m_outputCapture.pWaveFmt, sOutputFilename);
		m_outputBuffer->InitLame(sOutputFilename);

		if (m_inputCapture.Enabled)
		{
			std::wstring sInputFilename = settings.sFileName;
			sInputFilename += L"_i";

			m_inputBuffer->InitBuffer(m_inputCapture.pWaveFmt, sInputFilename);
			m_inputBuffer->InitLame(sInputFilename);
		}

		m_hThread = CreateThread(NULL, NULL, SoundCaptureThread, this, NULL, NULL);
		m_bWork = true;
	}
	LeaveCriticalSection(&m_guard);

	return m_bWork;
}

bool CSoundCapturer::StopRecord()
{
	EnterCriticalSection(&m_guard);
	{
		if (m_bWork)
		{
			SetEvent(m_hSyncEvent);

			if (WaitForSingleObject(m_hThread, 15000) != WAIT_OBJECT_0)
				TerminateThread(m_hThread, ERROR_SUCCESS);

			CloseHandle(m_hThread);
			m_hThread = nullptr;

			ResetEvent(m_hSyncEvent);
		}
		m_bWork = false;
	}
	LeaveCriticalSection(&m_guard);

	return true;
}

DWORD CSoundCapturer::SoundCaptureThread(LPVOID lParam)
{
	return static_cast<CSoundCapturer*>(lParam)->DoCaptureThread(nullptr);
}

DWORD CSoundCapturer::DoCaptureThread(LPVOID lParam)
{
	// Calculate the actual duration of the allocated buffer.
	REFERENCE_TIME hnsOutActualDuration =
		REFTIMES_PER_SEC * (m_outputCapture.bufferFrameCount / m_outputCapture.pWaveFmt->nSamplesPerSec);

	REFERENCE_TIME hnsInActualDuration = 0;
	if (m_inputCapture.Enabled)
	{
		hnsInActualDuration =
			REFTIMES_PER_SEC * (m_inputCapture.bufferFrameCount / m_inputCapture.pWaveFmt->nSamplesPerSec);

		m_inputCapture.pAudioClient->Start();
	}
	HRESULT hr = m_outputCapture.pAudioClient->Start(); // Start recording output.

	printf("%s\n", "StartRecording");

	time_t startTime = std::time(NULL);

	if (SUCCEEDED(hr))
	{
		BYTE *pInData = nullptr;
		DWORD inFlags = 0;
		UINT32 inPacketLength = 0;
		UINT32 inNumFramesAvailable = 0;
		DWORD inFrameSize = 0;
		if (m_inputCapture.Enabled)
			inFrameSize = m_inputCapture.pWaveFmt->nChannels * m_inputCapture.pWaveFmt->wBitsPerSample / 8;

		BYTE *pOutData = nullptr;
		DWORD outFlags = 0;
		UINT32 outPacketLength = 0;
		UINT32 outNumFramesAvailable = 0;
		const DWORD outFrameSize = m_outputCapture.pWaveFmt->nChannels * m_outputCapture.pWaveFmt->wBitsPerSample / 8;

		const DWORD WaitTime = static_cast<DWORD>(hnsOutActualDuration / 2 / REFTIMES_PER_MILLISEC);

		// Each loop fills about half of the shared buffer.
		while (true)
		{
			printf("%s\n", "GetData. . . . . .");

			// Sleep for half the buffer duration.
			Sleep(WaitTime);

			if (m_inputCapture.Enabled)
				m_inputCapture.pCaptureClient->GetNextPacketSize(&inPacketLength);
			m_outputCapture.pCaptureClient->GetNextPacketSize(&outPacketLength);

			while (outPacketLength != 0 || inPacketLength != 0)
			{
				// Get the available data in the shared buffer.
				hr = m_outputCapture.pCaptureClient->GetBuffer(
					&pOutData,
					&outNumFramesAvailable,
					&outFlags, NULL, NULL);

				if (m_inputCapture.Enabled)
				{
					hr = m_inputCapture.pCaptureClient->GetBuffer(
						&pInData,
						&inNumFramesAvailable,
						&inFlags, NULL, NULL);
					if (FAILED(hr))
						break;
				}

				if (outFlags & AUDCLNT_BUFFERFLAGS_SILENT)
				{
					//Write silence
					pOutData = nullptr;
				}
				if (inFlags & AUDCLNT_BUFFERFLAGS_SILENT)
				{
					//Write silence
					pInData = nullptr;
				}

				m_outputBuffer->AddData(pOutData, outNumFramesAvailable * outFrameSize);

				if (m_inputCapture.Enabled)
					m_inputBuffer->AddData(pInData, inNumFramesAvailable * inFrameSize);

				m_outputBuffer->ProcessBuffer();
				m_inputBuffer->ProcessBuffer();

				m_outputCapture.pCaptureClient->ReleaseBuffer(outNumFramesAvailable);
				m_outputCapture.pCaptureClient->GetNextPacketSize(&outPacketLength);

				if (m_inputCapture.Enabled)
				{
					m_inputCapture.pCaptureClient->ReleaseBuffer(inNumFramesAvailable);
					m_inputCapture.pCaptureClient->GetNextPacketSize(&inPacketLength);
				}
			}

			if (WaitForSingleObject(m_hSyncEvent, 0) == WAIT_OBJECT_0)
				break;
		}

		m_outputCapture.pAudioClient->Stop();
		if (m_inputCapture.Enabled)
			m_inputCapture.pAudioClient->Stop();

		ResetEvent(m_hSyncEvent);
	}
	else
	{
		m_hrError = hr;
		m_dwError = ::GetLastError();
		return false;
	}

	time_t finishTime = std::time(NULL);

	printf("%s\n", "StopRecording");

	m_outputBuffer->ProcessBuffer();
	m_inputBuffer->ProcessBuffer();

	m_outputBuffer->FinalizeLame();
	if (m_inputCapture.Enabled)
		m_inputBuffer->FinalizeLame();

	unsigned int duration = static_cast<unsigned int>(std::difftime(finishTime, startTime));
	if (duration > 2)
	{
		m_outputBuffer->SaveData();
		m_inputBuffer->SaveData();
	}

	m_outputBuffer->ReleaseBuffer();
	m_inputBuffer->ReleaseBuffer();

	return ERROR_SUCCESS;
}
