#pragma once

// MSVC safety prelude:
// The project previously had a local Math.h/math.h-style header, which can
// shadow the C runtime math.h on Windows. This file is force-included before
// every translation unit so the real CRT math declarations are loaded first.

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <math.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
