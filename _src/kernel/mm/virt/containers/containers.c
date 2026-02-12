#include "containers.h"

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdbitfield.h>
#include <lib/stdint.h>

#include "../../malloc/early_kalloc.h"
#include "../../malloc/reserve_malloc.h"
#include "../../mm_info.h"


static vmalloc_container* first_fva_container;
static vmalloc_container* first_rva_container;


typedef enum {
    VMALLOC_FVA,
    VMALLOC_RVA,
} vmalloc_container_enum;


static inline vmalloc_container* alloc_container(vmalloc_container* last)
{
    DEBUG_ASSERT(last);

    v_uintptr va = reserve_malloc().va;

    DEBUG_ASSERT(last->undef.hdr.next == NULL);
    DEBUG_ASSERT((va & (KPAGE_SIZE - 1)) == 0);

    vmalloc_container* allocated = (vmalloc_container*)va;

    allocated->undef.hdr.next = NULL;
    allocated->undef.hdr.prev = last;
    for (size_t i = 0; i < (DATA_BYTES / sizeof(uint64)); i++)
        allocated->undef.data[i] = 0;

    last->undef.hdr.next = allocated;

    return allocated;
}


static inline void* vmaloc_get_new(vmalloc_container* first, const vmalloc_container_enum e)
{
#ifdef DEBUG
    DEBUG_ASSERT(first);
    size_t x = 0;
#endif
    vmalloc_container* cur = first;
    vmalloc_container* prev = NULL;

    size_t i, j, k;
    rva_node* rva_nodes;
    fva_node* fva_nodes;
    bf* reserved_nodes;
    size_t count = (e == VMALLOC_FVA) ? FVA_NODE_COUNT : RVA_NODE_COUNT;


find_empty:
    while (cur) {
        if (e == VMALLOC_FVA) {
            fva_nodes = &cur->fva.data.nodes[0];
            reserved_nodes = &cur->fva.data.reserved_nodes[0];
        }
        else {
            rva_nodes = &cur->rva.data.nodes[0];
            reserved_nodes = &cur->rva.data.reserved_nodes[0];
        }

        for (i = 0; i < count; i++) {
            j = i / BITFIELD_CAPACITY(bf);
            k = i % BITFIELD_CAPACITY(bf);

            if (!bitfield_get(reserved_nodes[j], k)) {
                // found free node
                bitfield_set_high(reserved_nodes[j], k);

                return (e == VMALLOC_FVA) ? (void*)(fva_nodes + i) : (void*)(rva_nodes + i);
            }
        }

        prev = cur;
        cur = cur->undef.hdr.next;
    }

#ifdef DEBUG
    DEBUG_ASSERT(x++ == 0);
#endif

    cur = alloc_container(prev); // it updates prev->next inside

    goto find_empty;
}


static inline void free_container(vmalloc_container* first, vmalloc_container* to_free)
{
    DEBUG_ASSERT(first && to_free);

    vmalloc_container* cur = first;
    while (cur) {
        if (cur->undef.hdr.next == to_free) {
            cur->undef.hdr.next = to_free->undef.hdr.next;

            if (to_free->undef.hdr.next)
                to_free->undef.hdr.next->undef.hdr.prev = cur;

            raw_kfree(to_free);
            return;
        }

        cur = cur->undef.hdr.next;
    }

    PANIC();
}


fva_node* vmalloc_init_containers()
{
    first_fva_container =
        mm_kpa_to_kva_ptr(early_kalloc(sizeof(vmalloc_container), "vmalloc fva", true, false));
    first_rva_container =
        mm_kpa_to_kva_ptr(early_kalloc(sizeof(vmalloc_container), "vmalloc rva", true, false));


    *first_fva_container = (vmalloc_container) {0};
    *first_rva_container = (vmalloc_container) {0};


    // set as free all the kernel virtual memory
    fva_node* fva = get_new_fva_node();


    fva->start = KERNEL_BASE;
    fva->size = ~(uint64)0 - KERNEL_BASE + 1; // + 1 because ~0 is not inclusive


    ASSERT(KERNEL_BASE + fva->size - 1 == MM_KSECTIONS.heap.end);
    ASSERT(bitfield_get(first_fva_container->fva.data.reserved_nodes[0], 0) == true);
    fva_node* first_fva_node = &first_fva_container->fva.data.nodes[0];

    return first_fva_node;
}


fva_node* get_new_fva_node()
{
    fva_node* fva = vmaloc_get_new(first_fva_container, VMALLOC_FVA);

    DEBUG_ASSERT((v_uintptr)fva % _Alignof(fva_node) == 0);

    return fva;
}


rva_node* get_new_rva_node()
{
    rva_node* rva = vmaloc_get_new(first_rva_container, VMALLOC_RVA);

    DEBUG_ASSERT((v_uintptr)rva % _Alignof(rva_node) == 0);

    return rva;
}


void free_fva_node(fva_node* node)
{
    // get container by aligning down to 4096
    vmalloc_container* container = (vmalloc_container*)((v_uintptr)node & ~(KPAGE_SIZE - 1ULL));

    fva_container_data* d = (fva_container_data*)&container->fva.data;

    size_t i = (size_t)(node - d->nodes);

    bf* reserved_nodes = d->reserved_nodes;

    ASSERT(i < FVA_NODE_COUNT);

    size_t j = i / BITFIELD_CAPACITY(bf);
    size_t k = i % BITFIELD_CAPACITY(bf);


    DEBUG_ASSERT(bitfield_get(reserved_nodes[j], k), "vmalloc_node_free_fva: double free");

    bitfield_clear(reserved_nodes[j], k);


    // check if the container is empty, and if so, free the full container
    if (container == first_fva_container)
        return;

    for (i = 0; i < BITFIELD_COUNT_FOR(FVA_NODE_COUNT, bf); i++)
        if (reserved_nodes[i] != 0)
            return;

    free_container(first_fva_container, container);
}

void free_rva_node(rva_node* node)
{
    // get container by aligning down to 4096
    vmalloc_container* container = (vmalloc_container*)((v_uintptr)node & ~(KPAGE_SIZE - 1ULL));

    rva_container_data* c = (rva_container_data*)&container->rva.data;

    size_t i = (size_t)(node - c->nodes);

    bf* reserved_nodes = c->reserved_nodes;

    ASSERT(i < RVA_NODE_COUNT);

    size_t j = i / BITFIELD_CAPACITY(bf);
    size_t k = i % BITFIELD_CAPACITY(bf);


    DEBUG_ASSERT(bitfield_get(reserved_nodes[j], k), "vmalloc_node_free_rva: double free");

    bitfield_clear(reserved_nodes[j], k);


    // check if the container is empty, and if so, free the full container
    if (container == first_rva_container)
        return;

    for (i = 0; i < BITFIELD_COUNT_FOR(RVA_NODE_COUNT, bf); i++)
        if (reserved_nodes[i] != 0)
            return;

    free_container(first_rva_container, container);
}
