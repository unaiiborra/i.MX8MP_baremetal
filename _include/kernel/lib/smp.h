#pragma once
#include <lib/stdint.h>


uint64
get_core_id();

bool
wake_core(uint64 core_id, uintptr entry_addr, uint64 context);
