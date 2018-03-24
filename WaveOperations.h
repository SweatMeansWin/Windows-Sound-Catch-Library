#pragma once

#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE
#endif

inline HRESULT WriteWaveHeader(HMMIO hFile, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData)
{
	MMRESULT result;

	// make a RIFF/WAVE chunk
	pckRIFF->ckid = MAKEFOURCC('R', 'I', 'F', 'F');
	pckRIFF->fccType = MAKEFOURCC('W', 'A', 'V', 'E');

	result = mmioCreateChunk(hFile, pckRIFF, MMIO_CREATERIFF);
	if (MMSYSERR_NOERROR != result) {
		return E_FAIL;
	}

	// make a 'fmt ' chunk (within the RIFF/WAVE chunk)
	MMCKINFO chunk;
	chunk.ckid = MAKEFOURCC('f', 'm', 't', ' ');
	result = mmioCreateChunk(hFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		return E_FAIL;
	}

	// write the WAVEFORMATEX data to it
	LONG lBytesInWfx = sizeof(WAVEFORMATEX);
	if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		lBytesInWfx = sizeof(WAVEFORMATEX) + pwfx->cbSize;

	LONG lBytesWritten =
		mmioWrite(
			hFile,
			reinterpret_cast<PCHAR>(const_cast<LPWAVEFORMATEX>(pwfx)),
			lBytesInWfx
		);
	if (lBytesWritten != lBytesInWfx) {
		return E_FAIL;
	}

	// ascend from the 'fmt ' chunk
	result = mmioAscend(hFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		return E_FAIL;
	}

	// make a 'fact' chunk whose data is (DWORD)0
	chunk.ckid = MAKEFOURCC('f', 'a', 'c', 't');
	result = mmioCreateChunk(hFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		return E_FAIL;
	}

	// write (DWORD)0 to it
	// this is cleaned up later
	DWORD frames = 0;
	lBytesWritten = mmioWrite(hFile, reinterpret_cast<PCHAR>(&frames), sizeof(frames));
	if (lBytesWritten != sizeof(frames)) {
		return E_FAIL;
	}

	// ascend from the 'fact' chunk
	result = mmioAscend(hFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		return E_FAIL;
	}

	// make a 'data' chunk and leave the data pointer there
	pckData->ckid = MAKEFOURCC('d', 'a', 't', 'a');
	result = mmioCreateChunk(hFile, pckData, 0);
	if (MMSYSERR_NOERROR != result) {
		return E_FAIL;
	}

	return S_OK;
}

inline HRESULT FinishWaveFile(HMMIO hFile, MMCKINFO *pckRIFF, MMCKINFO *pckData)
{
	MMRESULT result;

	result = mmioAscend(hFile, pckData, 0);
	if (MMSYSERR_NOERROR != result) {
		return E_FAIL;
	}

	result = mmioAscend(hFile, pckRIFF, 0);
	if (MMSYSERR_NOERROR != result) {
		return E_FAIL;
	}

	return S_OK;
}