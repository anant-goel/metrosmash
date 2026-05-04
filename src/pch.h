#pragma once
// This MUST be included first in every .cpp to prevent MSVC's cmath conflict
// with windows.h (pulled in by SFML headers).
// The issue: windows.h pollutes the global namespace, breaking <cmath> on MSVC.

// Step 1: force C math symbols into global namespace BEFORE anything else
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

// Step 2: now safe to include C++ standard headers
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <map>
#include <iostream>
