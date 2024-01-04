#pragma once

#define FORCE_INLINE inline
//__attribute__((always_inline))

#define INDEX_2D_FROM_XY(X, Y, Width) ((Width) * (Y) + (X))
#define XOR_SHIFT32(S) S ^= (S << 13); S ^= (S >> 17); S ^= (S << 5);