#pragma once
#include <lib/stdint.h>

#include "lib/mem.h"


typedef struct {
    uint32 test;
    v_uintptr virt;
    char* tag;
} mm_page_data;


#define UNINIT_PAGE \
    (mm_page_data)  \
    {               \
        .test = 0,  \
    }
