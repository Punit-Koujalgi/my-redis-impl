#ifndef UTILITY_H
#define UTILITY_H


#include <string>
#include <algorithm>
#include <exception>
#include <fstream>
#include <condition_variable>

inline const std::string toLower(const std::string &str)
{
	std::string res{str};
	std::transform(res.begin(), res.end(), res.begin(), ::tolower);

	return res;
}

inline const void createFileWithData(const std::string &file, const std::string &data)
{
	std::ofstream outfile(file);
	outfile << data;
	outfile.close();
}

class EventWaiter
{
private:
	std::condition_variable cv;
	std::mutex mtx;
	bool eventSet = false;

public:
	// Wait for event with timeout, returns true if event was set, false if timeout
	bool waitForEvent(std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(mtx);
		return cv.wait_for(lock, timeout, [this]
						   { return eventSet; });
	}

	// Wait indefinitely until event is set
	void waitForEvent()
	{
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this]
				{ return eventSet; });
	}

	void setEvent()
	{
		{
			std::lock_guard<std::mutex> lock(mtx);
			eventSet = true;
		}
		cv.notify_all();
	}

	void resetEvent()
	{
		std::lock_guard<std::mutex> lock(mtx);
		eventSet = false;
	}
};

#endif // UTILITY_H