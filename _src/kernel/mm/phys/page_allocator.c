#include "page_allocator.h"

#include <lib/align.h>
#include <lib/lock/spinlock_irq.h>
#include <stddef.h>

#include "../mm_info.h"
#include "boot/panic.h"
#include "lib/lock/_lock_types.h"
#include "lib/lock/spinlock.h"
#include "lib/math.h"
#include "lib/mem.h"

typedef struct {
    mm_page_data page;
    bool free;
    uint8 order;
    size_t next;
} page_node;

typedef struct {
    spinlock_t lock;
    size_t* free_list;
    page_node* nodes;
    size_t N; // number of nodes
    size_t max_order;

} page_allocator_state;


page_allocator_state* state_;


void page_allocator_init()
{
    size_t N = mm_info_page_count();

    size_t max_order = log2_floor_u64(N);
    size_t free_lists = max_order + 1;

    size_t fl_start = align_up(sizeof(page_allocator_state), _Alignof(size_t*));

    size_t nodes_start = fl_start + align_up(sizeof(size_t*) * free_lists, _Alignof(page_node));

    size_t bytes = nodes_start + align_up(sizeof(page_node) * N, MM_PAGE_BYTES);


    p_uintptr start = early_kalloc(bytes, "mm_page_allocator", true);
    ASSERT(start % _Alignof(page_allocator_state) == 0);

    state_ = (page_allocator_state*)start;


    *state_ = (page_allocator_state) {
        .free_list = (size_t*)(start + fl_start),
        .nodes = (page_node*)(start + nodes_start),
        .N = N,
        .max_order = max_order,
    };

    spinlock_init(&state_->lock);


    for (size_t i = 0; i <= max_order; i++)
        state_->free_list[i] = NONE;

    for (size_t i = 0; i < N; i++) {
        state_->nodes[i] = (page_node) {
            .page = UNINIT_PAGE,
            .free = true,
            .order = max_order,
            .next = NONE,
        };

        state_->free_list[max_order] = 0;

        // TODO: init not pow2 N values


        memblock* mblcks;
        size_t mblck_count;
        early_kalloc_get_memblocks(&mblcks, &mblck_count);

        size_t i = 0;
        while (i < mblck_count) {
            ASSERT(mblcks[i].blocks <= max_order);

            if (is_pow2(mblcks[i].blocks)) {
                page_malloc(log2_floor_u64(mblcks[i].blocks), mblcks->tag);
                i += mblcks[i].blocks;
            }
            else {
                for (size_t j = 0; j < mblcks[i].blocks; j++) {
                    page_malloc(0, mblcks->tag);
                    i++;
                }
            }
        }
    }
}


static inline mm_page get_page_(page_allocator_state* s, size_t i)
{
    return (mm_page) {
        .order = s->nodes[i].order,
        .phys = i * MM_PAGE_BYTES,
        .data = s->nodes[i].page,
    };
}


static inline size_t buddy_of(size_t i, size_t o)
{
    return i ^ (1 << o);
}


static void list_push_(page_allocator_state* s, size_t i, size_t o)
{
    DEBUG_ASSERT(s->nodes[i].free);

    s->nodes[i].next = s->free_list[o];
    s->free_list[o] = i;
}


static size_t list_pop_(page_allocator_state* s, size_t o)
{
    if (s->free_list[o] == NONE)
        return NONE;


    size_t i = s->free_list[o];

    s->free_list[o] = s->nodes[i].next;
    s->nodes[i].next = NONE;

    return i;
}


static void list_remove_(page_allocator_state* s, size_t i, size_t o)
{
    size_t cur = s->free_list[o];
    size_t prev = NONE;

    while (cur != NONE) {
        if (cur == i) {
            if (prev != NONE)
                s->nodes[prev].next = s->nodes[cur].next;
            else
                s->free_list[o] = s->nodes[cur].next;
        }

        prev = cur;
        cur = s->nodes[cur].next;
    }
}


static size_t reserve_(page_allocator_state* s, size_t i)
{
    DEBUG_ASSERT(s->nodes[i].free);
    s->nodes[i].free = false;

    return i;
}

static size_t split_(page_allocator_state* s, size_t i)
{
    if (s->nodes[i].order == 0)
        return NONE;

    DEBUG_ASSERT(s->nodes[i].free);

    size_t o = s->nodes[i].order;
    size_t new_o = o - 1;

    size_t right = i + (1 << new_o);


    s->nodes[i].order = new_o;
    s->nodes[right].order = new_o;

    s->nodes[i].free = true;
    s->nodes[right].free = true;

    s->nodes[i].next = NONE;
    s->nodes[right].next = NONE;

    list_push_(s, right, new_o);

    return i;
}


static size_t merge_once_(page_allocator_state* s, size_t i)
{
    size_t o = s->nodes[i].order;
    size_t bud = buddy_of(i, o);

    if (bud >= s->N)
        return NONE;

    if (!s->nodes[i].free || s->nodes[bud].free || o != s->nodes[bud].order)
        return NONE;


    list_remove_(s, bud, o);


    size_t merged = min(i, bud);
    s->nodes[merged].order = o + 1;
    s->nodes[merged].free = true;

    return merged;
}


mm_page page_malloc(size_t order, const char* tag)
{
    mm_page page;
    irqlock_t f = _spin_lock_irqsave(&state_->lock);

    size_t fl = list_pop_(state_, order);
    if (fl != NONE) {
        page = get_page_(state_, reserve_(state_, fl));

        _spin_unlock_irqrestore(&state_->lock, f);
        return page;
    }

    for (size_t o = order; o <= state_->max_order; o++) {
        fl = list_pop_(state_, o);
        if (fl != NONE) {
            size_t cur = fl;

            for (size_t i = o; i < order; i--) {
                
            }
        }
    }

    page = (mm_page) {0};


    _spin_unlock_irqrestore(&state_->lock, f);
    return page;
}


void page_free(mm_page p)
{
    irq_spinlocked(&state_->lock)
    {
    }
}