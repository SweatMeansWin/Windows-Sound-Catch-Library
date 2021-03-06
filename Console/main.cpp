// SweatMeansWin
// Testing SoundRecorder library

#include <Windows.h>

#include <iostream>

#include "SoundCapturer.h"

#define CHECK(OPERATION) if (!OPERATION) { std::cout << capturer.GetDwError() << capturer.GetHrError() << std::endl; throw std::exception("failed"); }

int main()
{
	CoInitialize(NULL);

	try
	{
		CSoundCapturer capturer = CSoundCapturer();
		CHECK(capturer.Initialize());

		TRecordSettings settings = TRecordSettings();
		settings.sFileName = L"test.output";
		CHECK(capturer.StartRecord(settings));
		Sleep(5 * 1000);
		CHECK(capturer.StopRecord());
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
	}
	CoUninitialize();

    return 0;
}

