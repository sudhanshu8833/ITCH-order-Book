#ifndef ITCH_BYTE_ORDER_H
#define ITCH_BYTE_ORDER_H

// Big-endian to host conversions without <endian.h>, so the parser builds on
// Linux, macOS and Emscripten alike. ITCH 5.0 fields are big-endian.
#include <cstdint>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
inline uint16_t itch_be16toh(uint16_t v) { return v; }
inline uint32_t itch_be32toh(uint32_t v) { return v; }
inline uint64_t itch_be64toh(uint64_t v) { return v; }
#else
inline uint16_t itch_be16toh(uint16_t v) { return __builtin_bswap16(v); }
inline uint32_t itch_be32toh(uint32_t v) { return __builtin_bswap32(v); }
inline uint64_t itch_be64toh(uint64_t v) { return __builtin_bswap64(v); }
#endif

#endif
