#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX
#    define MAX(a, b)               \
        ({                          \
            __typeof__(a) _a = (a); \
            __typeof__(b) _b = (b); \
            _a > _b ? _a : _b;      \
        })
#endif

#ifndef MIN
#    define MIN(a, b)               \
        ({                          \
            __typeof__(a) _a = (a); \
            __typeof__(b) _b = (b); \
            _a < _b ? _a : _b;      \
        })
#endif

#ifndef BIT_CHECK
#    define BIT_CHECK(x, n) (((x) >> (n)) & 1)
#endif

#ifndef BIT_SET
#    define BIT_SET(x, n)           \
        ({                          \
            __typeof__(x) _x = (1); \
            (x) |= (_x << (n));     \
        })
#endif

#ifndef BIT_CLEAR
#    define BIT_CLEAR(x, n)         \
        ({                          \
            __typeof__(x) _x = (1); \
            (x) &= ~(_x << (n));    \
        })
#endif

#ifndef COUNT_OF
#    define COUNT_OF(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifdef __cplusplus
}
#endif
