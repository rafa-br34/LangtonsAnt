#pragma once

#define CELL_TYPE uint8_t
#define SIZE_TYPE int

#define FORCE_INLINE inline
//__attribute__((always_inline)) __forceinline
#define FLATTEN_2D(X, Y, Width) ((Width) * (Y) + (X))
#define XOR_SHIFT32(S) S ^= (S << 13); S ^= (S >> 17); S ^= (S << 5);


