#pragma once

#include <stdint.h>

struct __attribute__((packed)) version_info {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
};

#define CURRENT_VERSION {0, 0, 1}
