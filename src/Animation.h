#pragma once

#include <chrono>
#include <queue>

struct VelocityFrame {
	float dx;
	float dy;
	float dt;
};

class Animation {
protected:
	float vx = 0;
	float vy = 0;

	float vh = 0;

	float a = 0;
	float ax = 0;
	float ay = 0;
	std::chrono::steady_clock::time_point start_time;
	std::chrono::steady_clock::time_point last_batch_time;

	std::queue<VelocityFrame> vque;

public:
	void Start();
	void CountAverage();
	//void AddVelocity(float dx, float dy, float dt);
	void AddVelocity(float dx, float dy);
	int Interpolate(float& px, float& py, std::chrono::duration<float> elapsed);
};
