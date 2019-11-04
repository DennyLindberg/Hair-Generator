#pragma once

struct ApplicationClock
{
private:
	struct SDL2Time;
	SDL2Time* msTime;

public:
	double deltaTime = 0.0;
	double time = 0.0;

	ApplicationClock();
	~ApplicationClock();
	void Tick();
};
