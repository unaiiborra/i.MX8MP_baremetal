#include "vmalloc.h"

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>

#include "../mm_info.h"
#include "containers/containers.h"
#include "kernel/io/term.h"
#include "lib/stdbool.h"

static v_uintptr reserve_from_fva_node(fva_node* cur, fva_node* prev, v_uintptr va, size_t bytes);
static void push_rva(size_t pages, v_uintptr start, const char* tag, vmalloc_cfg cfg);

static fva_node* first_fva_node;
static rva_node* first_rva_node;


static inline v_uintptr add_no_overflow(v_uintptr va, size_t size)
{
    v_uintptr v = va + size;

    return v >= va ? v : ~(v_uintptr)0;
}


void vmalloc_init()
{
    first_fva_node = vmalloc_init_containers();
    first_rva_node = NULL;
}


v_uintptr vmalloc_update_memblocks(const memblock* mblcks, size_t n)
{
    ASSERT(first_fva_node, "vmalloc: not initialized");
    ASSERT(first_fva_node->next == NULL, "vmalloc: not initialized");

    v_uintptr va = 0;
    memblock mb;

    fva_node* cur = first_fva_node;
    fva_node* prev = NULL;


    for (size_t i = 0; i < n; i++) {
        mb = mblcks[i];

        v_uintptr target = mm_kpa_to_kva(mb.addr);

        va = reserve_from_fva_node(cur, prev, target, mb.blocks * KPAGE_SIZE);

        ASSERT(ptrs_are_kmapped(pv_ptr_new(mb.addr, va)));

        push_rva(mb.blocks, va, mm_kpa_to_kva_ptr(mb.tag),
                 (vmalloc_cfg) {
                     .kmap =
                         {
                             .use_kmap = true,
                             .kmap_pa = mb.addr,
                         },
                     .assing_pa = true,
                     .device_mem = mb.device_memory,
                     .permanent = mb.permanent,
                 });
    }

    return va + (mb.blocks * KPAGE_SIZE);
}


typedef enum {
    POP_FVA_MIN_START, // reserve any address bigger or equal than v
    POP_FVA_EXACT_VA,  // reserve exactly the provided v address, if not free panic
} pop_fva_options;


static v_uintptr reserve_from_fva_node(fva_node* cur, fva_node* prev, v_uintptr va, size_t bytes)
{
    v_uintptr cur_start = cur->start;
    v_uintptr cur_end = cur->start + cur->size;


    DEBUG_ASSERT(cur_start <= va && va + bytes <= add_no_overflow(cur_start, cur->size));


    // get full node
    if (va == cur_start && bytes == cur->size) {
        if (prev)
            prev->next = cur->next;
        else
            first_fva_node = cur->next;

        free_fva_node(cur);
        return va;
    }

    // get from start
    if (va == cur_start) {
        cur->start += bytes;
        cur->size -= bytes;
        return va;
    }

    // get from end
    if (va + bytes == cur_end) {
        cur->size = va - cur_start;
        return va;
    }

    // split (is in the middle)
    fva_node* new = get_new_fva_node();

    new->start = va + bytes;
    new->size = cur_end - (va + bytes);
    new->next = cur->next;

    cur->size = va - cur_start;
    cur->next = new;

    return va;
}


static v_uintptr pop_fva(size_t pages, const pop_fva_options options, v_uintptr v)
{
    DEBUG_ASSERT(first_fva_node, "vmalloc: no available free va");

    size_t bytes = pages * KPAGE_SIZE;
    fva_node* cur = first_fva_node;
    fva_node* prev = NULL;

    while (cur) {
        v_uintptr cur_start = cur->start;
        v_uintptr cur_end = add_no_overflow(cur_start, cur->size);


        bool fits = false;
        v_uintptr va = 0;

        if (options == POP_FVA_EXACT_VA) {
            if (cur_start <= v && add_no_overflow(v, bytes) <= cur_end) {
                fits = true;
                va = v;
            }
        }
        /*  options == POP_FVA_MIN_START
         */
        else {
            v_uintptr attempt = (cur_start < v) ? v : cur_start;
            if (attempt + bytes <= cur_end) {
                fits = true;
                va = attempt;
            }
        }

        if (fits)
            return reserve_from_fva_node(cur, prev, va, bytes);

        prev = cur;
        cur = cur->next;
    }

    PANIC("vmalloc: no available free virtual address range found");
}

static void push_fva(v_uintptr va, size_t pages)
{
    size_t bytes = pages * KPAGE_SIZE;
    fva_node* cur = first_fva_node;
    fva_node* prev = NULL;


    while (cur && cur->start < va) {
        prev = cur;
        cur = cur->next;
    }

#ifdef DEBUG
    if (prev)
        ASSERT(prev->start + prev->size <= va);

    if (cur)
        ASSERT(va + bytes <= cur->start);
#endif

    // if it can be joined with the prev node
    if (prev && prev->start + prev->size == va) {
        prev->size += bytes;

        // watch if it can now be joined with the next node
        if (cur && va + bytes == cur->start) {
            prev->size += cur->size;
            prev->next = cur->next;
            free_fva_node(cur);
        }

        return;
    }

    // if it can be joined with the next one
    if (cur && va + bytes == cur->start) {
        cur->start = va;
        cur->size += bytes;
        return;
    }


    // cannot be joined, alloc a new node
    fva_node* node = get_new_fva_node();

    *node = (fva_node) {
        .next = cur,
        .start = va,
        .size = bytes,
    };

    if (prev)
        prev->next = node;
    else
        first_fva_node = node;
}


// returns the size in bytes of the reserved va
static size_t pop_rva(v_uintptr start)
{
    rva_node* cur = first_rva_node;

    while (cur) {
        if (cur->start == start) {
            if (cur == first_rva_node)
                first_rva_node = cur->next;

            ASSERT(!cur->info.permanent, "vmalloc: cannot free a permanent va");

            size_t size = cur->size;

            free_rva_node(cur);

            return size;
        }

        cur = cur->next;
    }

    PANIC("vmalloc: attempted to free an unreserved virtual address");
}


static void push_rva(size_t pages, v_uintptr start, const char* tag, vmalloc_cfg cfg)
{
    size_t bytes = pages * KPAGE_SIZE;

    rva_node* cur = first_rva_node;
    rva_node* prev = NULL;

    while (cur && cur->start < start) {
        prev = cur;
        cur = cur->next;
    }

#ifdef DEBUG
    if (prev)
        DEBUG_ASSERT(prev->start + prev->size <= start);

    if (cur)
        DEBUG_ASSERT(start + bytes <= cur->start);
#endif

    rva_node* node = get_new_rva_node();

    *node = (rva_node) {
        .next = cur,
        .start = start,
        .size = bytes,
        .info =
            {
                .tag = tag,
                .kmapped = cfg.kmap.use_kmap,
                .pa_assigned = cfg.assing_pa,
                .device_mem = cfg.device_mem,
                .permanent = cfg.permanent,
            },
    };

    if (prev)
        prev->next = node;
    else
        first_rva_node = node;
}


v_uintptr vmalloc(size_t pages, const char* tag, vmalloc_cfg cfg)
{
    pop_fva_options opt;
    v_uintptr v;

    if (cfg.kmap.use_kmap) {
        ASSERT(is_pow2(pages), "vmalloc: only pow2 n pages can be requested for kmapping");
        ASSERT(cfg.assing_pa, "vmalloc: kmap can only be configured with assign_pa = true");

        opt = POP_FVA_EXACT_VA;
        v = mm_kpa_to_kva(cfg.kmap.kmap_pa);
    }
    else {
        opt = POP_FVA_MIN_START;
        v = mm_kpa_to_kva(mm_info_ddr_end());
    }


    v_uintptr start = pop_fva(pages, opt, v);

#ifdef DEBUG
    if (cfg.kmap.use_kmap)
        DEBUG_ASSERT(start == v);
    else
        DEBUG_ASSERT(start >= v);
#endif

    push_rva(pages, start, tag, cfg);


    return start;
}


void vfree(v_uintptr va)
{
    size_t size = pop_rva(va);
    DEBUG_ASSERT(size % KPAGE_SIZE == 0);
    push_fva(va, size / KPAGE_SIZE);
}


size_t vmalloc_get_free_bytes()
{
    size_t s = 0;
    fva_node* cur = first_fva_node;

    while (cur) {
        s += cur->size;
        cur = cur->next;
    }

    return s;
}


size_t vmalloc_get_reserved_bytes()
{
    size_t s = 0;
    rva_node* cur = first_rva_node;

    while (cur) {
        s += cur->size;
        cur = cur->next;
    }

    return s;
}


vmalloc_va_info vmalloc_get_addr_info(void* addr)
{
    v_uintptr a = (v_uintptr)addr;

    fva_node* fcur = first_fva_node;
    while (fcur) {
        v_uintptr start = fcur->start;
        v_uintptr end = add_no_overflow(start, fcur->size);

        if (start > a)
            break;

        if (end > a) {
            return (vmalloc_va_info) {.state = VMALLOC_VA_INFO_FREE,
                                      .state_info = {.free = {
                                                         .free_start = start,
                                                         .free_size = fcur->size,
                                                     }}};
        }

        fcur = fcur->next;
    }

    rva_node* rcur = first_rva_node;
    while (rcur) {
        v_uintptr start = rcur->start;
        v_uintptr end = add_no_overflow(start, rcur->size);

        if (start > a)
            break;

        if (end > a) {
            return (vmalloc_va_info) {.state = VMALLOC_VA_INFO_RESERVED,
                                      .state_info = {.reserved = {
                                                         .reserved_start = start,
                                                         .reserved_size = rcur->size,
                                                         .info = rcur->info,
                                                     }}};
        }

        rcur = rcur->next;
    }

    return (vmalloc_va_info) {.state = VMALLOC_VA_UNREGISTERED};
}


void vmalloc_debug_free()
{
    term_printf("\n\r");
    term_printf("=============================================\n\r");
    term_printf("           VMALLOC - FREE REGIONS            \n\r");
    term_printf("=============================================\n\r");
    term_printf("   START              END                SIZE (bytes / pages)\n\r");
    term_printf("-------------------------------------------------------------\n\r");

    size_t total = 0;
    fva_node* cur = first_fva_node;

    while (cur) {
        v_uintptr start = cur->start;
        v_uintptr end = add_no_overflow(start, cur->size);
        size_t pages = cur->size / KPAGE_SIZE;

        term_printf("   [%p - %p)   %p B  (%p pages)\n\r", start, end, cur->size, pages);

        total += cur->size;
        cur = cur->next;
    }

    term_printf("-------------------------------------------------------------\n\r");
    term_printf("   TOTAL FREE: %p bytes (%p pages)\n\r", total, total / KPAGE_SIZE);
    term_printf("=============================================\n\r");
}


void vmalloc_debug_reserved()
{
    term_printf("\n\r");
    term_printf("=============================================\n\r");
    term_printf("         VMALLOC - RESERVED REGIONS\n\r");
    term_printf("=============================================\n\r");
    term_printf("   START              END                SIZE    FLAGS\n\r");
    term_printf("-------------------------------------------------------------\n\r");

    size_t total = 0;
    rva_node* cur = first_rva_node;

    while (cur) {
        v_uintptr start = cur->start;
        v_uintptr end = add_no_overflow(start, cur->size);
        size_t pages = cur->size / KPAGE_SIZE;

        term_printf("   [%p - %p)   %p B (%d p)   ", start, end, cur->size, pages);

        term_printf("[");
        term_printf("%c", cur->info.kmapped ? 'K' : '-');
        term_printf("%c", cur->info.pa_assigned ? 'P' : '-');
        term_printf("%c", cur->info.device_mem ? 'D' : '-');
        term_printf("%c", cur->info.permanent ? '!' : '-');
        term_printf("]");

        term_printf("\t%s\n\r", cur->info.tag);

        total += cur->size;
        cur = cur->next;
    }

    term_printf("-------------------------------------------------------------\n\r");
    term_printf("   TOTAL RESERVED: %p bytes (%p pages)\n\r", total, total / KPAGE_SIZE);
    term_printf("=============================================\n\r");
}
