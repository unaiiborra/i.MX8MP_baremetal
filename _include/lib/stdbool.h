#pragma once

// FIXME: work with clangd

#ifdef __cplusplus
#error "Do not include this header from C++ code"
#endif

#ifdef _STDBOOL_H
#error "C std lib stdbool used instead of custom baremetal implementation"
#endif

#define _STDBOOL_H 1

/// Standard bool type
#if defined(__cplusplus) && __cplusplus < 201103L

#error "Imported custom stdbool inside a cpp file"

#else


#define bool _Bool
#define true 1
#define false 0

#endif