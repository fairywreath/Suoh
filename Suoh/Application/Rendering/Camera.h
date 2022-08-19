#pragma once

#include <CoreTypes.h>

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

class FirstPersonCameraController : public CameraController
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

} // namespace Suoh
