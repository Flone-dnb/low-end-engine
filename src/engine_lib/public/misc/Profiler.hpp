#pragma once

#if defined(ENGINE_PROFILER_ENABLED)
#include "tracy/public/tracy/Tracy.hpp"
#define PROFILE_FUNC ZoneScoped;
#define PROFILE_SCOPE(name) ZoneScopedN(name);
#define PROFILE_ADD_SCOPE_TEXT(text, size) ZoneText(text, size)
#else
#define PROFILE_FUNC
#define PROFILE_SCOPE(name)
#define PROFILE_ADD_SCOPE_TEXT(text, size)
#endif
