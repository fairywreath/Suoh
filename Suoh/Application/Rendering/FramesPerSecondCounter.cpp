#include "FramesPerSecondCounter.h"

#include <Logger.h>

namespace Suoh
{

FramesPerSecondCounter::FramesPerSecondCounter(float avgIntervalSec)
    : mAvgIntervalSec(avgIntervalSec)
{
}

bool FramesPerSecondCounter::tick(float deltaSeconds, bool frameRendered)
{
    if (frameRendered)
        mNumFrames++;

    mAccumulatedTime += deltaSeconds;

    if (mAccumulatedTime > mAvgIntervalSec)
    {
        mCurrentFps = static_cast<float>(mNumFrames / mAccumulatedTime);

        // if (PrintFps)
        // {
        //     LOG_INFO("FPS: ", mCurrentFps);
        // }

        mNumFrames = 0;
        mAccumulatedTime = 0;
        return true;
    }

    return false;
}

} // namespace Suoh