#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef float  f32;
typedef double f64;

static inline f32 MinF32(f32 a, f32 b) {
    return a < b ? a : b;
}

static inline i32 MinI32(i32 a, i32 b) {
    return a < b ? a : b;
}

static inline f32 MaxF32(f32 a, f32 b) {
    return a > b ? a : b;
}

static inline i32 MaxI32(i32 a, i32 b) {
    return a > b ? a : b;
}

#endif