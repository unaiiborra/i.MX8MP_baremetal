#pragma once


#ifdef DEBUG

#    define PAGE_ALLOCATOR_DEBUG_MAX_ORDER 6

void test_single_alloc_free(void);
void test_split_to_zero(void);
void test_many_small_allocs(void);
void test_mixed_orders(void);
void test_full_merge(void);
void test_stress_pattern(void);

void run_page_allocator_tests(void);


#else

#    define test_single_alloc_free()
#    define test_split_to_zero()
#    define test_many_small_allocs()
#    define test_mixed_orders()
#    define test_full_merge()
#    define test_stress_pattern()

#    define run_page_allocator_tests()
#endif