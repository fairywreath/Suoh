#pragma once

namespace Suoh
{

class FramesPerSecondCounter
{
public:
    explicit FramesPerSecondCounter(float avgIntervalSec = 0.5f);

    bool tick(float deltaSeconds, bool frameRendered = true);

    inline float getFps() const
    {
        return mCurrentFps;
    }

    bool PrintFps = true;

private:
    const float mAvgIntervalSec{0.5f};

    unsigned int mNumFrames{0};
    double mAccumulatedTime{0};
    float mCurrentFps{0.0f};
};

} // namespace Suoh