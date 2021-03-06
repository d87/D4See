#pragma once

#include "Canvas.h"
#include "D4See.h"

#include <chrono>
#include <queue>
#include <functional>

class Animation {

public:
	virtual int Animate(D4See::Canvas& canvas, std::chrono::duration<float> elapsed) { return 0;  };
};

struct VelocityFrame {
	float dx;
	float dy;
	float dt;
};

class MomentumAnimation : public virtual Animation {
protected:
	float vx = 0;
	float vy = 0;

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
	virtual int Animate(D4See::Canvas& canvas, std::chrono::duration<float> elapsed) override;
};

class TranslateAnimation: public virtual Animation {
protected:
	float vx = 0;
	float vy = 0;

public:
	TranslateAnimation(float vx, float vy);
	virtual int Animate(D4See::Canvas& canvas, std::chrono::duration<float> elapsed) override;
};
