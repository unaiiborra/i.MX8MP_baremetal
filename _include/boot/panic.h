#pragma once
#include <lib/stdbool.h>
#include <lib/stdint.h>

extern uint64 PANIC_MESSAGE_BUF_SIZE;
extern uint64 PANIC_FILE_BUF_SIZE;

extern uint8* PANIC_MESSAGE_BUF_PTR;
extern uint8* PANIC_FILE_BUF_PTR;

extern uint32 PANIC_LINE;
extern uint32 PANIC_COL;

extern uint32 PANIC_REASON;

extern uint64 PANIC_REGISTERS[32];

typedef enum {
    PANIC_REASON_UNDEFINED = 0,
    PANIC_REASON_EXCEPTION = 1,
    PANIC_REASON_MANUAL_ABORT = 2,
    PANIC_REASON_RUST_PANIC = 3,
} panic_reason;

typedef struct {
    char* file;
    uint32 line;
    uint32 col;
} panic_location;

typedef struct {
    const char* message;
    panic_location location;
    panic_reason panic_reason;
} panic_info;

void init_panic();

void set_panic(panic_info panic_info);
_Noreturn void set_and_throw_panic(panic_info panic_info);
_Noreturn void panic();

#define PANIC(panic_message)                       \
    set_and_throw_panic((panic_info) {             \
        .message = panic_message,                  \
        .location =                                \
            (panic_location) {                     \
                .file = __FILE__,                  \
                .line = __LINE__,                  \
                .col = 0,                          \
            },                                     \
        .panic_reason = PANIC_REASON_MANUAL_ABORT, \
    })


/*
    Assert
*/

#define _ASSERT1(cond) ((cond) ? (void)0 : PANIC("assert not met!"))
#define _DEBUG_ASSERT1(cond) ((cond) ? (void)0 : PANIC("debug assert not met!"))
#define _ASSERT2(cond, msg) ((cond) ? (void)0 : PANIC(msg))
#define GET_ASSERT(_1, _2, NAME, ...) NAME


#define ASSERT(...) GET_ASSERT(__VA_ARGS__, _ASSERT2, _ASSERT1)(__VA_ARGS__)
#ifdef DEBUG
#    define DEBUG_ASSERT(...) GET_ASSERT(__VA_ARGS__, _ASSERT2, _DEBUG_ASSERT1)(__VA_ARGS__)
#else
#    define DEBUG_ASSERT(...)
#endif


/// In the case of an exception panic, saves all the register in the extern
/// savers
extern void _panic_exception_save_gpr();
