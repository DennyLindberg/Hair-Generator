#include "clock.h"
#include "SDL2/SDL.h"
#undef main

struct ApplicationClock::SDL2Time
{
	Uint64 previous = 0;
	Uint64 current = 0;
	Uint64 delta()
	{
		return current - previous;
	}
};

ApplicationClock::ApplicationClock()
{
	msTime = new SDL2Time();
	Tick();
}

ApplicationClock::~ApplicationClock()
{
	delete msTime;
}

void ApplicationClock::Tick()
{
	msTime->previous = msTime->current;
	msTime->current = SDL_GetPerformanceCounter();
	deltaTime = (double)(msTime->delta() / (double)SDL_GetPerformanceFrequency());
	time = SDL_GetTicks() / 1000.0;
}
