#include "kernel/io/term.h"

#include <lib/branch.h>
#include <lib/lock/spinlock_irq.h>
#include <lib/stdarg.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "kernel/panic.h"
#include "lib/lock/corelock.h"

// TODO: needs handling in case of TERM_OUT_RES_NOT_TAKEN results

typedef enum {
    TERM_UNINIT = 0,
    TERM_EARLY_MODE,
    TERM_FULL_MODE,
} term_early;


static term_early mode;
static term_out early_output;
static corelock_t lock;


void term_init_early(term_out early_out)
{
    mode = TERM_EARLY_MODE;
    early_output = early_out;

    corelock_init(&lock);

    term_prints("\x1B[2J\x1B[H"); // clear screen

#ifdef DEBUG
    term_prints("[term_early]: initialized\n\r");
#endif
}


/*
    TODO:
    This is a temporal solution before having kmalloc, it just is a static variable that saves one
    output. When kmalloc is designed this must be removed
*/
static term_out full_output;


void term_init_full()
{
    corelock_init(&lock);

    mode = TERM_FULL_MODE;
    full_output = NULL;
}


void term_add_output(term_out out)
{
    core_lock(&lock);
    full_output = out;
    core_unlock(&lock);
}

void term_remove_output(term_out out);

void term_add_input(term_in in);
void term_remove_input(term_in in);


/*
    Prints
*/
static inline term_out get_term_out()
{
    switch (mode) {
        case TERM_UNINIT:
            goto hang;
        case TERM_EARLY_MODE:
            if (UNLIKELY(!early_output))
                goto hang;

            return early_output;
        case TERM_FULL_MODE:
            if (UNLIKELY(!full_output))
                goto hang;

            return full_output;
        default:
            goto hang;
    }

    loop hang : asm volatile("wfi");
}


void term_printc(const char c)
{
    core_lock(&lock);
    get_term_out()(c);
    core_unlock(&lock);
}

void term_prints(const char* s)
{
    core_lock(&lock);

    term_out out = get_term_out();

    while (*s)
        out(*s++);

    core_unlock(&lock);
}


// TODO: allow variable formatting with allocations etc.

static char fmt_buf[4096];
static size_t fmt_i;

static void putfmt(char c)
{
    fmt_buf[fmt_i++] = c;

    ASSERT(fmt_i < 4096);
}


/// The formatting happens before the printing because in future versions checking the result of the
/// term_out will be needed and that cannot happen inside the fmt fn. This fn will need a big remake
/// when kmalloc is implemented TODO:
void term_printf(const char* s, ...)
{
    va_list ap;
    va_start(ap, s);

    core_lock(&lock);

    fmt_i = 0;
    str_fmt_print(putfmt, s, ap);
    fmt_buf[fmt_i] = '\0';
    fmt_i = 0;

    term_out out = get_term_out();

    s = fmt_buf;
    while (*s)
        out(*s++);

    core_unlock(&lock);

    va_end(ap);
}