#include "error.h"

namespace D4See {

	Error lastError = Error::OK;

	void SetError(Error err) {
		lastError = err;
	}

	Error GetLastError() {
		return lastError;
	}

}