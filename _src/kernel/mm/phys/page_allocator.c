#include "page_allocator.h"

#include <lib/lock/spinlock_irq.h>

#include "../init/early_kalloc.h"
#include "../mm_info.h"
#include "arm/mmu/mmu.h"
#include "boot/panic.h"
#include "lib/align.h"
#include "lib/lock/_lock_types.h"
#include "lib/math.h"
#include "lib/mem.h"
#include "lib/stdint.h"
#include "lib/stdmacros.h"
#include "lib/string.h"
#include "page.h"
#include "tests.h"

#define MM_PAGE_BYTES MMU_GRANULARITY_4KB

#define NONE (~(size_t)0)
#define IS_NONE(v) ((v) == NONE)

typedef struct {
    mm_page_data page;
    size_t next;
    bool free;
    uint8 order;
} page_node;

typedef struct {
    spinlock_t lock;
    size_t* free_list;
    page_node* pages;
    size_t N; // number of nodes
    size_t max_order;
} page_allocator_state;


static page_allocator_state* s;


static inline size_t buddy_of(size_t i, size_t o)
{
    return i ^ (1 << o);
}


static inline size_t get_parent(size_t i, size_t parent_o)
{
    return i & ~((1UL << (parent_o)) - 1);
}

static inline bool is_in_free_list(size_t i, uint8 o)
{
    size_t cur = s->free_list[o];

    while (cur != NONE) {
        if (cur == i)
            return true;

        cur = s->pages[cur].next;
    }

    return false;
}


#ifdef DEBUG
static inline void ASSERT_ORDER(size_t i, size_t o)
{
    DEBUG_ASSERT(o <= s->max_order);
    size_t mask = (1UL << o) - 1;
    DEBUG_ASSERT((i & mask) == 0, "index does not correlate with the provided order");
}


#else // !DEBUG
#    define ASSERT_ORDER(...)
#endif


static inline void list_push(size_t i)
{
    size_t o = s->pages[i].order;
    DEBUG_ASSERT(!IS_NONE(i));
    ASSERT_ORDER(i, o);

    size_t last = s->free_list[o];

    s->free_list[o] = i;
    s->pages[i].next = last;
    s->pages[i].free = true;
}


static inline size_t list_pop(size_t o)
{
    size_t i = s->free_list[o];

    if (IS_NONE(i))
        return i;

    ASSERT_ORDER(i, o);

    s->free_list[o] = s->pages[i].next;
    s->pages[i].next = NONE;
    s->pages[i].free = false;

    return i;
}


static void list_remove(size_t i)
{
    DEBUG_ASSERT(!IS_NONE(i));
    DEBUG_ASSERT(s->pages[i].free);

    size_t o = s->pages[i].order;
    size_t cur = s->free_list[o];

    // it is the head
    if (cur == i) {
        s->free_list[o] = s->pages[i].next;
        s->pages[i].next = NONE;
        s->pages[i].free = false;
        return;
    }

    // search the list
    while (!IS_NONE(cur)) {
        size_t next = s->pages[cur].next;

        if (next == i) {
            s->pages[cur].next = s->pages[i].next;
            s->pages[i].next = NONE;
            s->pages[i].free = false;
            return;
        }

        cur = next;
    }

    PANIC("list_remove: node not found in free list");
}


// removes it from the free list, splits and reserves
// it is expected to get an already reserved page, it will free the buddies
static void split_until(size_t i, size_t target_o)
{
    size_t o = s->pages[i].order;

    ASSERT_ORDER(i, o);
    ASSERT_ORDER(i, target_o);
    DEBUG_ASSERT(!s->pages[i].free);
    DEBUG_ASSERT(o != 0);

    ASSERT(o >= target_o);

    while (o > target_o) {
        size_t new_o = o - 1;
        size_t b = buddy_of(i, new_o);

        s->pages[b].order = new_o;
        s->pages[b].next = NONE;
        list_push(b);

        o--;
    }

    s->pages[i].order = target_o;
    s->pages[i].next = NONE;
}


static void try_merge(size_t i)
{
    size_t o = s->pages[i].order;


    ASSERT_ORDER(i, o);
    DEBUG_ASSERT(s->pages[i].free && is_in_free_list(i, o));

    // early return
    size_t b = buddy_of(i, o);
    if (i >= s->N || b >= s->N || !s->pages[b].free || s->pages[b].order != o)
        return;


    list_remove(i);

    while (o < s->max_order) {
        b = buddy_of(i, o);

        if (b >= s->N)
            break;

        if (!s->pages[b].free || s->pages[b].order != o)
            break;

        list_remove(b);

        i = min(i, b);

        s->pages[i].order = ++o;
    }

    s->pages[i].next = NONE;
    list_push(i);
}


static inline mm_page build_page(size_t i)
{
    return (mm_page) {
        .order = s->pages[i].order,
        .phys = i * MM_PAGE_BYTES,
        .data = s->pages[i].page,
    };
}


mm_page page_malloc(size_t order, mm_page_data p)
{
    ASSERT(order <= s->max_order);

    size_t o = order;

    while (o <= s->max_order) {
        size_t i = list_pop(o); // reserves the page if some

        if (!IS_NONE(i)) {
            if (o != order)
                split_until(i, order);

            s->pages[i].page = p;
            return build_page(i);
        }

        o++;
    }

    return (mm_page) {
        .order = UINT8_MAX,
    };
}


void page_free(mm_page p)
{
    size_t i = p.phys / MM_PAGE_BYTES;

    ASSERT(i < s->N);
    ASSERT(!s->pages[i].free, "page_allocator: double free");
    DEBUG_ASSERT(s->pages[i].order == p.order);

    s->pages[i].page = UNINIT_PAGE;

    list_push(i);
    try_merge(i);
}


void page_free_by_tag(const char* tag)
{
    ASSERT(tag != NULL);

    for (size_t i = 0; i < s->N; i++) {
        page_node* n = &s->pages[i];

        if (n->free)
            continue;

        if (!n->page.tag)
            continue;

        if (!strcmp(n->page.tag, tag))
            continue;

        mm_page p = {
            .phys = i * MM_PAGE_BYTES,
            .order = n->order,
            .data = n->page,
        };

        page_free(p);
    }
}


void page_allocator_init()
{
    size_t entries = mm_info_page_count();

    size_t max_order = log2_floor_u64(entries);

    size_t state_bytes = align_up(sizeof(page_allocator_state), _Alignof(size_t));
    size_t free_list_bytes = align_up(sizeof(size_t) * (max_order + 1), _Alignof(page_node));

    size_t pages_bytes = sizeof(page_node) * entries;

    size_t bytes = state_bytes + free_list_bytes + pages_bytes;

    s = (void*)early_kalloc(bytes, "page_allocator", true, false);
    p_uintptr free_list_addr = (p_uintptr)s + state_bytes;
    p_uintptr pages_addr = free_list_addr + free_list_bytes;

    *s = (page_allocator_state) {
        .lock = {0},
        .free_list = (void*)free_list_addr,
        .pages = (void*)pages_addr,
        .N = entries,
        .max_order = max_order,
    };
    spinlock_init(&s->lock);

    for (size_t i = 0; i <= s->max_order; i++)
        s->free_list[i] = NONE;

    for (size_t i = 0; i < s->N; i++)
        s->pages[i] = (page_node) {
            .page = UNINIT_PAGE,
            .free = false,
            .order = 0,
            .next = NONE,
        };


    // calculate the needed orders and initialize them
    size_t i = 0;
    size_t remaining = entries;

    while (remaining > 0) {
        size_t align_order = (i == 0) ? s->max_order : (size_t)__builtin_ctzll(i);
        max_order = min(log2_floor_u64(remaining), align_order);
        max_order = min(max_order, s->max_order);

        s->pages[i].order = max_order;
        list_push(i);

        size_t size = 1UL << max_order;
        i += size;
        remaining -= size;
    }

#ifdef DEBUG
    page_allocator_debug_pages(false);
#endif
}


static void reserve_order0(size_t i, mm_page_data data)
{
    ASSERT(i < s->N);

    int8 o_found = -1;
    size_t base = 0;

    for (int8 o = s->max_order; o >= 0; o--) {
        size_t mask = (1UL << o) - 1;
        size_t b = i & ~mask;

        if (b >= s->N)
            continue;

        if (s->pages[b].free && s->pages[b].order == o && is_in_free_list(b, o)) {
            o_found = o;
            base = b;
            break;
        }
    }

    ASSERT(o_found >= 0, "reserve_order0: page not free");

    list_remove(base);

    size_t o = (size_t)o_found;

    while (o > 0) {
        o--;

        size_t buddy = base ^ (1UL << o);

        s->pages[buddy].order = o;
        s->pages[buddy].next = NONE;
        s->pages[buddy].free = true;

        list_push(buddy);

        if (i & (1UL << o))
            base = buddy;
    }

    ASSERT(base == i);

    s->pages[i].free = false;
    s->pages[i].order = 0;
    s->pages[i].page = data;
    s->pages[i].next = NONE;
}


static void page_reserve_range(p_uintptr start, size_t pages, mm_page_data data)
{
    size_t i = start / MM_PAGE_BYTES;

    while (pages--)
        reserve_order0(i++, data);
}


void page_allocator_reserve_memblocks(memblock* mblcks, size_t n)
{
    uintptr addr = 0;

    for (size_t i = 0; i < n; i++) {
        memblock mblck = mblcks[i];

        ASSERT(mblck.addr == addr);


        mm_page_data data = (mm_page_data) {
            .tag = mblck.tag,
            .virt = addr,
            .device_mem = mblck.device_memory,
            .permanent = mblck.permanent,
        };


        page_reserve_range(addr, mblck.blocks, data);

        addr += mblck.blocks * MM_PAGE_BYTES;
    }
}


/*
    DEBUG
*/

#ifdef DEBUG

p_uintptr page_allocator_testing_init()
{
    size_t entries = 1 << PAGE_ALLOCATOR_DEBUG_MAX_ORDER;

    size_t state_bytes = align_up(sizeof(page_allocator_state), _Alignof(size_t));
    size_t free_list_bytes =
        align_up(sizeof(size_t) * (PAGE_ALLOCATOR_DEBUG_MAX_ORDER + 1), _Alignof(page_node));
    size_t pages_bytes = sizeof(page_node) * entries;

    size_t bytes = state_bytes + free_list_bytes + pages_bytes;

    s = (void*)early_kalloc(bytes, "page_allocator_testing", true, false);
    p_uintptr free_list_addr = (p_uintptr)s + state_bytes;
    p_uintptr pages_addr = free_list_addr + free_list_bytes;


    *s = (page_allocator_state) {
        .lock = {0},
        .free_list = (void*)free_list_addr,
        .pages = (void*)pages_addr,
        .N = entries,
        .max_order = PAGE_ALLOCATOR_DEBUG_MAX_ORDER,
    };

    for (size_t i = 0; i <= s->max_order; i++)
        s->free_list[i] = NONE;

    for (size_t i = 0; i < s->N; i++)
        s->pages[i] = (page_node) {
            .page = UNINIT_PAGE,
            .free = false,
            .order = 0,
            .next = NONE,
        };

    s->pages[0].order = PAGE_ALLOCATOR_DEBUG_MAX_ORDER;
    list_push(0);

    return (p_uintptr)s;
}

#    include "drivers/uart/uart.h"

static void putc(char c)
{
    uart_putc_sync(&UART2_DRIVER, c);
}

static void puts(const char* s, ...)
{
    va_list ap;
    va_start(ap, s);

    str_fmt_print(putc, s, ap);

    va_end(ap);
}

void page_allocator_debug_pages(bool full_print)
{
    puts("\n\r");

    if (!s) {
        puts("[page_alloc]\tstate = NULL\n\r");
        return;
    }

    puts("==== PAGE ALLOCATOR STATE ====\n\r");
    puts("N\t\t= %d\n\r", s->N);
    puts("max_order\t= %d\n\r", s->max_order);
    puts("free_list\t= %p\n\r", s->free_list);
    puts("pages\t\t= %p\n\r", s->pages);
    puts("\n\r");

    puts("-- FREE LISTS --\n\r");
    for (size_t o = 0; o <= s->max_order; o++) {
        puts("order %d: ", o);

        size_t cur = s->free_list[o];
        if (IS_NONE(cur)) {
            puts("<empty>\n\r");
            continue;
        }

        while (!IS_NONE(cur)) {
            puts("%d ", cur);
            cur = s->pages[cur].next;
        }
        puts("\n\r");
    }

    puts("\n\r");

    puts("-- PAGE NODES --\n\r");

    for (size_t i = 0; i < s->N;) {
        page_node* n = &s->pages[i];

        if (!full_print) {
            size_t o = n->order;

            if (o <= s->max_order && (i & ((1UL << o) - 1)) != 0) {
                i++;
                continue;
            }
        }

        puts("[node %d]\n\r", i);
        puts("\tfree\t= %s\n\r", n->free ? "true" : "false");
        puts("\torder\t= %d\n\r", n->order);

        if (IS_NONE(n->next))
            puts("\tnext\t= NONE\n\r");
        else
            puts("\tnext\t= %d\n\r", n->next);

        if (n->page.tag)
            puts("\ttag\t= %s\n\r", n->page.tag);
        else
            puts("\ttag\t= NULL\n\r");

        puts("\tphys\t= %p\n\r", i * MM_PAGE_BYTES);
        puts("\tdevice\t= %s\n\r", n->page.device_mem ? "y" : "n");


        if (n->order > s->max_order)
            puts("\t!! INVALID ORDER\n\r");

        if (!n->free && !IS_NONE(n->next))
            puts("\t!! USED BUT IN FREE LIST\n\r");

        puts("\n\r");

        if (!full_print && n->order <= s->max_order)
            i += (1UL << n->order);
        else
            i++;
    }

    puts("==== END PAGE ALLOCATOR STATE ====\n\r");
}


void page_allocator_validate()
{
    bool seen[s->N];

    for (size_t i = 0; i < sizeof(seen); i++) {
        seen[i] = false;
    }

    size_t free_pages = 0;

    for (size_t o = 0; o <= s->max_order; o++) {
        size_t cur = s->free_list[o];

        while (!IS_NONE(cur)) {
            ASSERT(cur < s->N);

            DEBUG_ASSERT(s->pages[cur].free);
            DEBUG_ASSERT(s->pages[cur].order == o);

            size_t mask = (1UL << o) - 1;
            DEBUG_ASSERT((cur & mask) == 0);

            DEBUG_ASSERT(!seen[cur]);
            seen[cur] = true;

            free_pages += (1UL << o);

            cur = s->pages[cur].next;
        }
    }

    for (size_t i = 0; i < s->N; i++) {
        if (s->pages[i].free) {
            DEBUG_ASSERT(seen[i]);
        }
    }

    for (size_t i = 0; i < s->N; i++) {
        if (!seen[i])
            continue;

        size_t o = s->pages[i].order;
        size_t size = 1UL << o;

        for (size_t j = i + 1; j < s->N; j++) {
            if (!seen[j])
                continue;

            size_t oj = s->pages[j].order;
            size_t sizej = 1UL << oj;

            bool overlap = !(i + size <= j || j + sizej <= i);

            DEBUG_ASSERT(!overlap);
        }
    }

    DEBUG_ASSERT(free_pages <= s->N);
}


static inline bool page_data_equal_dbg(mm_page_data a, mm_page_data b)
{
    return a.tag == b.tag && a.device_mem == b.device_mem && a.permanent == b.permanent;
}

static inline bool is_head(size_t i)
{
    size_t o = s->pages[i].order;
    if (o > s->max_order)
        return false;

    return (i & ((1UL << o) - 1)) == 0;
}

void page_allocator_debug()
{
    puts("==== PAGE ALLOCATOR CONTENT ====\n\r");

    for (size_t i = 0; i < s->N;) {
        if (!is_head(i)) {
            i++;
            continue;
        }

        page_node* n = &s->pages[i];
        size_t o = n->order;
        size_t block_pages = 1UL << o;

        size_t start = i;
        size_t total_pages = block_pages;

        bool reserved = !n->free;
        mm_page_data data = n->page;

        size_t j = i + block_pages;

        while (reserved && j < s->N) {
            if (!is_head(j))
                break;

            page_node* next = &s->pages[j];

            if (next->free)
                break;

            if (next->order != o)
                break;

            if (!page_data_equal_dbg(data, next->page))
                break;

            total_pages += (1UL << next->order);
            j += (1UL << next->order);
        }

        if (reserved)
            puts("[USED]\torder=%d  \ttag=%s\tIDX %d..%d\t(%d pages, %d KiB)\n\r", o,
                 data.tag ? data.tag : "<none>", start, start + total_pages - 1, total_pages,
                 total_pages * 4);
        else
            puts("[FREE]\torder=%d  \tIDX %d..%d\t(%d pages)\n\r", o, start,
                 start + block_pages - 1, block_pages);


        i = j;
    }

    puts("==== END PAGE ALLOCATOR DEBUG ====\n\r");
}

#endif
