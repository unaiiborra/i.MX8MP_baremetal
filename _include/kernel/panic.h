#pragma once
#include <lib/stdbool.h>
#include <lib/stdint.h>

#define PANIC(panic_message)         \
	set_and_throw_panic((PanicInfo){ \
		.message = #panic_message,   \
		.location =                  \
			(PanicLocation){         \
				.file = __FILE__,    \
				.line = __LINE__,    \
				.col = 0,            \
			},                       \
	})

extern uint64 PANIC_MESSAGE_LEN;
extern uint64 PANIC_FILE_LEN;

extern uint8 *PANIC_MESSAGE_PTR;
extern uint8 *PANIC_FILE_PTR;

extern uint32 PANIC_LINE;
extern uint32 PANIC_COL;

typedef struct {
	char *file;
	uint32 line;
	uint32 col;
} PanicLocation;

typedef struct {
	char *message;
	PanicLocation location;
} PanicInfo;

void init_panic();

void set_panic(PanicInfo panic_info);
void set_and_throw_panic(PanicInfo panic_info);
void panic();
