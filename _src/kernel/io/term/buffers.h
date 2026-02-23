#pragma once

#include <kernel/io/term.h>


void term_buffers_init();


void UNLOCKED_term_add_output(term_out out);
void UNLOCKED_term_remove_output(term_out out);
void UNLOCKED_term_enable_output(term_out out);
void UNLOCKED_term_disable_output(term_out out);

void UNLOCKED_term_add_input(term_in in);
void UNLOCKED_term_remove_input(term_in in);
void UNLOCKED_term_enable_input(term_in in);
void UNLOCKED_term_disable_input(term_in in);