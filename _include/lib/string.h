#pragma once

#include <lib/stdint.h>

/// Copies a string into another buffer until the max_size or until finding a
/// \0. It allways appends a \0 either at max_size -1 or where it finds it in
/// the source
void strcopy(char *dst, char *src, uint64 max_size);