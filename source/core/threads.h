#pragma once
#include <thread>

namespace Threads
{
	unsigned int Count();
	void Join();
}

struct ThreadInfo
{
	unsigned int id = 0;
	bool isDone = false;
};
