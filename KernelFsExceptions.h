#pragma once

#include <exception>

#define Verify(condition) \
	if (!(condition)) { \
		throw std::exception(); \
	}