#include "Canvas.h"

#include <algorithm>

using namespace D4See;

float Canvas::GetRotation() {
	return baseRotation + orientRotation;
}
void Canvas::SetImageBaseRotation(float rot) {
	baseRotation = rot;
}
void Canvas::SetImageBaseRotation(int rot) {
	baseRotation = static_cast<float>(rot);
}
void Canvas::SetOrientRotation(float rot){
	orientRotation = rot;
}
void Canvas::CycleRotation(int direction){
	int rot = orientRotation + ((direction > 0) ? 90 : -90);
	rot += 360; // avoid negative angle
	rot = rot % 360;
	orientRotation = static_cast<float>(rot);
}
//void Canvas::LimitPanOffset() {
//    float x_limit = static_cast<float>(w_scaled - w_client);
//    x_poffset = std::min(x_poffset, x_limit);
//    x_poffset = std::max(x_poffset, 0.0f);
//
//    float y_limit = static_cast<float>(h_scaled - h_client);
//    y_poffset = std::min(y_poffset, y_limit);
//    y_poffset = std::max(y_poffset, 0.0f);
//}