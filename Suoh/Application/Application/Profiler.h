#pragma once

#include <Core/Types.h>

// #define RUN_PROFILER

#ifdef RUN_PROFILER
#include <optick.h>
#endif

#ifdef RUN_PROFILER
#define PROF_FUNCTION(...) OPTICK_EVENT()
#define PROF_BLOCK(name, ...) \
    {                         \
        OptickScopeWrapper Wrapper(name);
#define PROF_END_BLOCK \
    }                  \
    ;
#define PROF_THREAD_SCOPE(...) OPTICK_START_THREAD(__VA_ARGS__)
#define PROF_ENABLE OPTICK_START_CAPTURE()
#define PROF_MAIN_THREAD OPTICK_THREAD("MainThread")
#define PROF_FRAME(name) OPTICK_FRAME(name)
#define PROF_DUMP(fileName) \
    OPTICK_STOP_CAPTURE();  \
    OPTICK_SAVE_CAPTURE(fileName);

#else
#define PROF_FUNCTION(...)
#define PROF_BLOCK(...)
#define PROF_END_BLOCK
#define PROF_THREAD_SCOPE(...)
#define PROF_ENABLE
#define PROF_MAIN_THREAD
#define PROF_FRAME(...)
#define PROF_DUMP(fileName)
#endif

namespace Suoh
{

#ifdef RUN_PROFILER

namespace Profiler
{

namespace Colors
{

const s32 Magenta = Optick::Color::Magenta;
const s32 Green = Optick::Color::Green;
const s32 Red = Optick::Color::Red;

} // namespace Colors

} // namespace Profiler

class OptickScopeWrapper
{
public:
    OptickScopeWrapper(const std::string& name)
    {
        OPTICK_PUSH(name.c_str());
    }

    ~OptickScopeWrapper()
    {
        OPTICK_POP();
    }
};

#endif

} // namespace Suoh