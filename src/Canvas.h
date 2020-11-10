#pragma once

namespace D4See {
	class Canvas {
    public:
        int w_client; // client area, everything inside the window frame
        int h_client;

        float scale_manual = 1.0f; // 
        float scale_effective = 1.0f; // Actual current scale
        float baseRotation = 0.0f; // Inherent image rotation determined by EXIF metadata
        float orientRotation = 0.0f; // 90 degree rotation by user
        bool flipHorizontal = false;
        bool flipVertical = false;

        float x_poffset = 0; // Panning offset
        float y_poffset = 0;

        int w_scaled; // Dimensions after applying scale
        int h_scaled;

        float GetRotation();
        void SetImageBaseRotation(float rot);
        void SetImageBaseRotation(int rot);
        void SetOrientRotation(float rot);
        void CycleRotation(int direction);
	};
}