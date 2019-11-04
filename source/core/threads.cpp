#include "threads.h"

#ifndef USE_MULTITHREADING
#define USE_MULTITHREADING false
#endif

unsigned int numThreads = std::thread::hardware_concurrency();
std::vector<std::thread> threads(numThreads);
std::vector<ThreadInfo> threadInfos(numThreads);

namespace Threads
{
	unsigned int Count()
	{
		return numThreads;
	}
	
	void Join()
	{
		for (auto& t : threads)
		{
			t.join();
		}
	}

	bool AreDone()
	{
		if constexpr (USE_MULTITHREADING)
		{
			for (unsigned int i = 0; i < threadInfos.size(); i++)
			{
				if (!threadInfos[i].isDone)
				{
					return false;
				}
			}
			return true;
		}
		else
		{
			return threadInfos[0].isDone;
		}
	}
}