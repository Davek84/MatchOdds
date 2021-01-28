#include "Timer.h"

void time_measuring::BaseTimer::SetTimeout(const std::chrono::milliseconds & ms, std::function<void (void)> callBack)
{
	timeoutThread = std::thread([this, ms, callBack]()
	{
		std::cv_status retStatus;
		do {
			std::unique_lock<std::mutex> lock(timerMutex);
			retStatus = condTimer.wait_for(lock, ms);
			callBack();
			if (retStatus == std::cv_status::no_timeout) {
				printf("What?!\n");
			}
		} while (retStatus == std::cv_status::timeout);
	});
}

void time_measuring::BaseTimer::StopTimer()
{
	condTimer.notify_one();
	timeoutThread.join();
}
