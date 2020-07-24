#pragma once

namespace D4See {
	enum class Error {
		NONE,
		OK,
		DirectoryNotExists,
		FileNotExists,
		FileNotSupported,
		EmptyPlaylist
	};

	void SetError(Error err);

	Error GetLastError();
}