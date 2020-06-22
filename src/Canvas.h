#pragma once

namespace D4See {
	class Canvas {
    public:
        float scale_manual = 1.0f; // 
        float scale_effective = 1.0f; // Actual current scale

        float x_poffset = 0; // Panning offset
        float y_poffset = 0;

        int w_scaled; // Dimensions after applying scale
        int h_scaled;
	};
}