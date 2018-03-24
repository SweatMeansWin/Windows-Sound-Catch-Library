#pragma once

#include "AudioBuffer.h"

#include <Audioclient.h>
#include <WinNT.h>
#include <Mmdeviceapi.h>

#include <string>

struct TRecordSettings
{
	std::wstring sFileName;
};

class CSoundCapturer
{
public:
	CSoundCapturer();
	~CSoundCapturer();

	bool Initialize();
	void DeInitialize();

	bool StartRecord(TRecordSettings& settings);
	bool StopRecord();

	HRESULT		GetHrError() { return m_hrError; }
	DWORD		GetDwError() { return m_dwError; }
private:
	HRESULT				m_hrError;
	DWORD				m_dwError;
	bool				m_bInit;
	bool				m_bWork;
	HANDLE				m_hSyncEvent;
	HANDLE				m_hThread;
	CRITICAL_SECTION	m_guard;

	//Audio params
	struct TCaptureObjects
	{
	public:
		TCaptureObjects();
		~TCaptureObjects();
		void Release();

		IMMDevice*				pDevice;
		IAudioClient*			pAudioClient;
		IAudioCaptureClient*	pCaptureClient;
		WAVEFORMATEX*			pWaveFmt;
		UINT32					bufferFrameCount;
		bool					Enabled;
	};

	TCaptureObjects			m_inputCapture;
	TCaptureObjects			m_outputCapture;
	CAudioBuffer*			m_inputBuffer;
	CAudioBuffer*			m_outputBuffer;

	bool initOutputAudio(IMMDeviceEnumerator* pEnumerator);
	bool initInputAudio(IMMDeviceEnumerator* pEnumerator);

	void releaseAudioObjects();
	DWORD WINAPI DoCaptureThread(LPVOID lParam);
	static DWORD WINAPI SoundCaptureThread(LPVOID lParam);
};