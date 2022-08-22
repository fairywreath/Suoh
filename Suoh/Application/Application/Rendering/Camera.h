#pragma once

#include <Core/Types.h>

#include <glm/ext.hpp>

namespace Suoh
{

using quat = glm::quat;

class CameraController
{
public:
    virtual mat4 getViewMatrix() const = 0;
    virtual vec3 getPosition() const = 0;
};

class Camera
{
public:
    explicit Camera(CameraController& controller);

    mat4 getViewMatrix() const;
    vec3 getPosition() const;

private:
    const CameraController* mController;
};

/*
 * Quaternion based first-person/look-at camera
 */
class FirstPersonCameraController final : public CameraController
{
public:
    FirstPersonCameraController() = default;
    FirstPersonCameraController(const vec3& pos, const vec3& target, const vec3& up);

    mat4 getViewMatrix() const override;
    vec3 getPosition() const override;

    void update(double deltaSeconds, const vec2& mousePos, bool mousePressed);

    void setPosition(const vec3& pos);
    void resetMousePosition(const vec2& pos);
    void setUpVector(const vec3& up);

    inline void lookAt(const vec3& pos, const vec3& target, const vec3& up)
    {
        mCameraPosition = pos;
        mCameraOrientation = glm::lookAt(pos, target, up);
    }

public:
    struct Movement
    {
        bool forward{false};
        bool backward{false};
        bool left{false};
        bool right{false};
        bool up{false};
        bool down{false};

        bool fast{false};
    } Movement;

public:
    float MouseSpeed{4.0f};
    float Acceleration{150.0f};
    float Damping{0.1f};
    float MaxSpeed{10.0f};
    float FastCoef{10.0f};

private:
    vec2 mMousePosition{0.0f, 0.0f};
    vec3 mCameraPosition{0.0f, 10.0f, 10.0f};
    quat mCameraOrientation{quat(vec3(0))};
    vec3 mMoveSpeed{0.0f, 0.0f, 0.0f};
    vec3 mUp{0.0f, 1.0f, 0.0f};
};

/*
 * Euler angles based free-moving/move-to camera
 */
class FreeMovingCameraController final : public CameraController
{
public:
    FreeMovingCameraController() = default;
    FreeMovingCameraController(const vec3& pos, const vec3& angles);

    mat4 getViewMatrix() const override final;
    vec3 getPosition() const override final;

    void update(float deltaSeconds);

    void setPosition(const vec3& pos);
    void setAngles(float pitch, float pan, float roll);
    void setAngles(const vec3& angles);

    void setDesiredPosition(const vec3& pos);
    void setDesiredAngles(float pitch, float pan, float roll);
    void setDesiredAngles(const vec3& angles);

public:
    float DampingLinear{10.0f};
    vec3 DampingEulerAngles{5.0f, 5.0f, 5.0f};

private:
    vec3 mPositionCurrent{vec3(0.0f)};
    vec3 mPositionDesired{vec3(0.0f)};

    // pitch, pan, roll
    vec3 mAnglesCurrent{vec3(0.0f)};
    vec3 mAnglesDesired{vec3(0.0f)};

    mat4 mCurrentTransform{mat4(1.0f)};

    static inline float clipAngle(float d)
    {
        if (d < -180.0f)
            return d + 360.0f;
        if (d > +180.0f)
            return d - 360.f;
        return d;
    }

    static inline vec3 clipAngles(const vec3& angles)
    {
        return vec3(
            std::fmod(angles.x, 360.0f),
            std::fmod(angles.y, 360.0f),
            std::fmod(angles.z, 360.0f));
    }

    static inline vec3 angleDelta(const vec3& anglesCurrent, const vec3& anglesDesired)
    {
        const vec3 d = clipAngles(anglesCurrent) - clipAngles(anglesDesired);
        return vec3(clipAngle(d.x), clipAngle(d.y), clipAngle(d.z));
    }
};

} // namespace Suoh
