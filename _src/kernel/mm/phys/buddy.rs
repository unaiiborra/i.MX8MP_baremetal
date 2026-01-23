use core::{cmp::max, usize};

use crate::kernel::mm::phys::mm_page;

#[repr(C)]
#[derive(Clone)]
struct page_node {
    page: mm_page,
    next: Option<usize>,
    order: u8,
    free: bool,
}

pub struct page_allocator<const FREE_LISTS: usize, const N: usize> {
    free_lists: [Option<usize>; FREE_LISTS],
    nodes: [page_node; N],
    max_order: usize,
}

impl<const FREE_LISTS: usize, const N: usize> page_allocator<FREE_LISTS, N> {
    pub fn init(&mut self) {
        debug_assert!(FREE_LISTS == self.max_order + 1 && N.is_power_of_two());

        for p in &mut self.nodes {
            *p = page_node {
                page: mm_page::new(),
                next: None,
                order: 0,
                free: true,
            };
        }

        for fl in &mut self.free_lists {
            *fl = None;
        }
    }

    #[inline]
    fn buddy_of(&self, i: usize, order: u8) -> usize {
        i ^ (1_usize << order as usize)
    }

    fn list_push(&mut self, i: usize, order: u8) {
        assert!(self.nodes[i].free);

        self.nodes[i].next = self.free_lists[order as usize];
        self.free_lists[order as usize] = Some(i);
    }

    fn list_pop(&mut self, order: u8) -> Option<usize> {
        let i = self.free_lists[order as usize]?;

        self.free_lists[order as usize] = self.nodes[i].next;
        self.nodes[i].next = None;

        Some(i)
    }

    fn list_remove(&mut self, i: usize, order: u8) {
        let mut cur = self.free_lists[order as usize];
        let mut prev: Option<usize> = None;

        while let Some(c) = cur {
            if c == i {
                if let Some(p) = prev {
                    self.nodes[p].next = self.nodes[c].next;
                } else {
                    self.free_lists[order as usize] = self.nodes[c].next;
                }
                self.nodes[c].next = None;
                return;
            }
            prev = cur;
            cur = self.nodes[c].next;
        }
    }

    fn reserve(&mut self, i: usize) -> Option<usize> {
        debug_assert!(self.nodes[i].free);
        self.nodes[i].free = false;

        Some(i)
    }

    fn split(&mut self, i: usize) -> Option<usize> {
        if self.nodes[i].order == 0 {
            return None;
        }

        debug_assert!(self.nodes[i].free);

        let order = self.nodes[i].order;
        let new_order = order - 1;

        let right = i + (1 << new_order);

        self.nodes[i].order = new_order;
        self.nodes[right].order = new_order;

        self.nodes[i].free = true;
        self.nodes[right].free = true;

        self.nodes[i].next = None;
        self.nodes[right].next = None;

        self.list_push(right, new_order);

        Some(i)
    }

    fn merge_once(&mut self, i: usize) -> Option<usize> {
        let order = self.nodes[i].order;
        let buddy = self.buddy_of(i, order);

        if buddy >= N {
            return None;
        }

        if !self.nodes[i].free || !self.nodes[buddy].free || self.nodes[buddy].order != order {
            return None;
        }

        // quitar buddy de la free list
        self.list_remove(buddy, order);

        let merged = core::cmp::min(i, buddy);
        self.nodes[merged].order = order + 1;
        self.nodes[merged].free = true;

        Some(merged)
    }

    pub fn alloc(&mut self, order: u8) -> Option<usize> {
        if let Some(i) = self.list_pop(order) {
            return self.reserve(i);
        }

        for o in (order as usize + 1)..=self.max_order {
            if let Some(i) = self.list_pop(o as u8) {
                let mut cur = i;

                for _ in (order as usize..o).rev() {
                    cur = self.split(cur)?;
                }

                return self.reserve(cur);
            }
        }

        None
    }

    pub fn free(&mut self, i: usize) {
        let mut cur = i;
        self.nodes[cur].free = true;

        loop {
            match self.merge_once(cur) {
                Some(next) => cur = next,
                None => break,
            }
        }

        let order = self.nodes[cur].order;
        self.list_push(cur, order);
    }
}

#[unsafe(no_mangle)]
extern "C" fn page_alloc() {}
