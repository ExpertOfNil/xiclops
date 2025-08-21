#ifndef CORE_H
#define CORE_H

#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define ARRAY_COUNT(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define DEFAULT_ARRAY_CAPACITY 256
#define DEFINE_DYNAMIC_ARRAY(type, name)                                    \
    typedef struct name {                                                   \
        type* items;                                                        \
        size_t count;                                                       \
        size_t capacity;                                                    \
    } name;                                                                 \
                                                                            \
    static inline void name##_init(name* arr) {                             \
        arr->items = NULL;                                                  \
        arr->count = 0;                                                     \
        arr->capacity = 0;                                                  \
    }                                                                       \
                                                                            \
    static inline void name##_reserve(name* arr, size_t cap) {              \
        if ((cap) > (arr)->capacity) {                                      \
            if ((arr)->capacity == 0) {                                     \
                (arr)->capacity = DEFAULT_ARRAY_CAPACITY;                   \
            }                                                               \
            while ((cap) > (arr)->capacity) {                               \
                (arr)->capacity *= 2;                                       \
            }                                                               \
            (arr)->items = realloc(                                         \
                (arr)->items, (arr)->capacity * sizeof(*(arr)->items));     \
            assert((arr)->items != NULL && "ARRAY_RESERVE: Out of memory"); \
        }                                                                   \
    }                                                                       \
                                                                            \
    static inline void name##_push(name* arr, type item) {                  \
        name##_reserve((arr), (arr)->count + 1);                            \
        arr->items[arr->count++] = item;                                    \
    }                                                                       \
                                                                            \
    static inline void name##_push_many(                                    \
        name* arr, const type* new_items, uint32_t new_items_count) {       \
        name##_reserve((arr), (arr)->count + (new_items_count));            \
        memcpy(                                                             \
            (arr)->items + (arr)->count,                                    \
            (new_items),                                                    \
            (new_items_count) * sizeof(*(arr)->items));                     \
        (arr)->count += (new_items_count);                                  \
    }                                                                       \
                                                                            \
    static inline void name##_clear(name* arr) {                            \
        memset(arr->items, 0, arr->count);                                  \
        arr->count = 0;                                                     \
        arr->capacity = 0;                                                  \
    }                                                                       \
                                                                            \
    static inline void name##_free(name* arr) {                             \
        if (arr->items) {                                                   \
            free(arr->items);                                               \
            arr->items = NULL;                                              \
        }                                                                   \
        arr->count = 0;                                                     \
        arr->capacity = 0;                                                  \
    }

typedef enum {
    ERROR,
    WARN,
    INFO,
    DEBUG,
    TRACE,
} LEVEL;

DEFINE_DYNAMIC_ARRAY(char, String)

static LEVEL VERBOSITY = INFO;

const char* LevelStr(LEVEL lvl) {
    switch (lvl) {
        case ERROR: {
            return "ERROR";
        }
        case WARN: {
            return "WARN";
        }
        case INFO: {
            return "INFO";
        }
        case DEBUG: {
            return "DEBUG";
        }
        case TRACE: {
            return "TRACE";
        }
    }
}

void Log(LEVEL v, String* msg) {
    if (v <= VERBOSITY) {
        printf("[XICLOPS %s] %s", LevelStr(v), msg->items);
    }
    String_clear(msg);
}

static inline void String_printf(String* str, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int args_len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (args_len > 0) {
        String_reserve(str, str->count + args_len);

        va_start(args, fmt);
        vsnprintf(str->items + str->count, args_len + 1, fmt, args);
        str->count += args_len;
        va_end(args);
    }
}

#endif /* CORE_H */
