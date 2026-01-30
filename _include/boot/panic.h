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


#define PANIC_IMPL(panic_message)                  \
    set_and_throw_panic((panic_info) {             \
        .message = (panic_message),                \
        .location =                                \
            (panic_location) {                     \
                .file = __FILE__,                  \
                .line = __LINE__,                  \
                .col = 0,                          \
            },                                     \
        .panic_reason = PANIC_REASON_MANUAL_ABORT, \
    })

#define _PANIC_0() PANIC_IMPL("no message")
#define _PANIC_1(msg) PANIC_IMPL(msg)

#define _PANIC_SELECT(_0, _1, NAME, ...) NAME
#define PANIC(...) _PANIC_SELECT(_, ##__VA_ARGS__, _PANIC_1, _PANIC_0)(__VA_ARGS__)


#define _ASSERT_1(cond) ((cond) ? (void)0 : PANIC("assert not met"))
#define _ASSERT_2(cond, msg) ((cond) ? (void)0 : PANIC(msg))

#define _ASSERT_SELECT(_0, _1, _2, NAME, ...) NAME
#define ASSERT(...) _ASSERT_SELECT(_, ##__VA_ARGS__, _ASSERT_2, _ASSERT_1)(__VA_ARGS__)

#ifdef DEBUG
#    define _DEBUG_ASSERT_1(cond) ((cond) ? (void)0 : PANIC("debug assert not met"))
#    define _DEBUG_ASSERT_2(cond, msg) ((cond) ? (void)0 : PANIC(msg))
#    define DEBUG_ASSERT(...) \
        _ASSERT_SELECT(_, ##__VA_ARGS__, _DEBUG_ASSERT_2, _DEBUG_ASSERT_1)(__VA_ARGS__)
#else
#    define DEBUG_ASSERT(...)
#endif


/// In the case of an exception panic, saves all the register in the extern
/// savers
extern void _panic_exception_save_gpr();
