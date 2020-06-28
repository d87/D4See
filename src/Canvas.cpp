#include "Canvas.h"

#include <algorithm>

using namespace D4See;


//void Canvas::LimitPanOffset() {
//    float x_limit = static_cast<float>(w_scaled - w_client);
//    x_poffset = std::min(x_poffset, x_limit);
//    x_poffset = std::max(x_poffset, 0.0f);
//
//    float y_limit = static_cast<float>(h_scaled - h_client);
//    y_poffset = std::min(y_poffset, y_limit);
//    y_poffset = std::max(y_poffset, 0.0f);
//}