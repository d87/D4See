#include "Animation.h"
#include "D4See.h"

void Animation::Start() {
	start_time = std::chrono::steady_clock::now();
	last_batch_time = start_time;
	vx = 0;
	vy = 0;
	vque.empty();
}

void Animation::CountAverage() {
	float tdx = 0.0; // combined dx
	float tdy = 0.0;
	float tdt = 0.0;
	while (!vque.empty()) {
		VelocityFrame vf = vque.front();
		tdx += vf.dx;
		tdy += vf.dy;
		tdt += vf.dt;
		vque.pop();
	}
	vx = tdx / tdt;
	vy = tdy / tdt;

	//vh = std::sqrt(vx * vx + vy * vy);
	ax = -vx * 0.015;
	ay = -vy * 0.015;
}

void Animation::AddVelocity(float dx, float dy) {
	auto now = std::chrono::steady_clock::now();
	auto dt_ = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_batch_time);
	last_batch_time = now;
	float dt = static_cast<float> (dt_.count()) / 1000;


	VelocityFrame vf;
	vf.dx = dx;
	vf.dy = dy;
	vf.dt = dt;

	LOG_DEBUG("{0} {1} {2}", dx, dy, dt);

	vque.push(vf);

	if (vque.size() > 10) {
		vque.pop();
	}
}

int Animation::Interpolate(float& px, float& py, std::chrono::duration<float> elapsed) {
	if (vx == 0.0f && vy == 0.0f) return 0;

	float dx = vx * elapsed.count();
	float dy = vy * elapsed.count();

	px -= dx;
	py -= dy;

	float vx0 = vx;
	float vy0 = vy;

	if (vx != 0.0) vx += ax;
	if (vy != 0.0) vy += ay;

	//LOG("--------------");
	//LOG("{0} {1}", vx, vx * vx0);
	//LOG("{0} {1}", vy, vy * vx0);
	if ((vx*vx0) < 0) vx = 0.0; // if vx goes from negative to positive or the other way around
	if ((vy*vy0) < 0) vy = 0.0;
	return 1;
}