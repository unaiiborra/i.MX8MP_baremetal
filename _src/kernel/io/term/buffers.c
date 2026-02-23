#include "buffers.h"

#include <kernel/mm.h>
#include <lib/lock/corelock.h>
#include <lib/math.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "kernel/io/term.h"


typedef struct {
    uint64 id;
    size_t size;
    char* a;
} term_buff;


typedef struct term_node {
    uint64 id;
    struct term_node* next;
    void* fn;
    bool enabled;
} term_node;


static term_out_id last_out_id;
static term_out_id last_in_id;

static term_node* output_enabled_list;
static term_node* output_disabled_list;

static term_node* input_enabled_list;
static term_node* input_disabled_list;





void term_buffers_init()
{
    output_enabled_list = NULL;
    output_disabled_list = NULL;

    input_enabled_list = NULL;
    input_disabled_list = NULL;

    last_out_id = 0;
    last_in_id = 0;
}


