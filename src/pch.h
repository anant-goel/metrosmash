#pragma once
// ============================================================
// PRECOMPILED HEADER — must be processed before ALL other headers
// Fixes MSVC cmath errors caused by windows.h polluting the C
// math namespace (sinf, cosf, tgammaf, ilogbf, etc. not found).
//
// Strategy: include every C math/stdlib header in C form FIRST,
// before windows.h (dragged in by SFML) can interfere.
// ============================================================

// C headers first — puts all math symbols in global namespace
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

// Now Windows — already too late to break math symbols
#if defined(_WIN32) || defined(_WIN64)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #ifndef VC_EXTRA_LEAN
    #define VC_EXTRA_LEAN
  #endif
  #include <windows.h>
#endif

// Now safe: C++ wrappers just alias what's already in global namespace
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <climits>
#include <cfloat>

// STL
#include <algorithm>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <map>
#include <unordered_map>
#include <array>
#include <iostream>
