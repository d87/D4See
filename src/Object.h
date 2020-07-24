#pragma once
#include <vector>

namespace D4See {
	class IObject {
    public:
		std::vector<IObject> children;

		virtual void Draw();
		// virtual void SetParent();
	};
}