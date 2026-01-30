#pragma once
#include <lib/stdint.h>

#include "lib/mem.h"


typedef struct {
    v_uintptr virt;
    const char* tag;
    // TODO: use a bitfield
    bool device_mem;
    bool permanent;
} mm_page_data;


#define UNINIT_PAGE (mm_page_data) {0}
