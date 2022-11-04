#include "Camera.h"

#include <glm/gtx/euler_angles.hpp>

Camera::Camera(CameraController& controller) : mController(&controller)
{
}

mat4 Camera::getViewMatrix() const
{
    return mController->getViewMatrix();
}

vec3 Camera::getPosition() const
{
    return mController->getPosition();
}

FirstPersonCameraController::FirstPersonCameraController(const vec3& pos, const vec3& target,
                                                         const vec3& up)
    : mCameraPosition(pos), mCameraOrientation(glm::lookAt(pos, target, up)), mUp(up)
{
}

mat4 FirstPersonCameraController::getViewMatrix() const
{
    const glm::mat4 transMatrix = glm::translate(glm::mat4(1.0f), -mCameraPosition);
    const glm::mat4 rotMatrix = glm::mat4_cast(mCameraOrientation);
    return rotMatrix * transMatrix;
}

vec3 FirstPersonCameraController::getPosition() const
{
    return mCameraPosition;
}

void FirstPersonCameraController::update(double deltaSeconds, const glm::vec2& mousePos,
                                         bool mousePressed)
{
    if (mousePressed)
    {
        const vec2 delta = mousePos - mMousePosition;
        const quat deltaQuat = quat(vec3(-MouseSpeed * delta.y, MouseSpeed * delta.x, 0.0f));
        mCameraOrientation = deltaQuat * mCameraOrientation;
        mCameraOrientation = glm::normalize(mCameraOrientation);
        setUpVector(mUp);
    }
    mMousePosition = mousePos;

    const mat4 v = mat4_cast(mCameraOrientation);

    const vec3 forward = -vec3(v[0][2], v[1][2], v[2][2]);
    const vec3 right = vec3(v[0][0], v[1][0], v[2][0]);
    const vec3 up = glm::cross(right, forward);

    glm::vec3 accel(0.0f);

    if (Movement.forward)
        accel += forward;
    if (Movement.backward)
        accel -= forward;

    if (Movement.left)
        accel -= right;
    if (Movement.right)
        accel += right;

    if (Movement.up)
        accel += up;
    if (Movement.down)
        accel -= up;

    if (Movement.fast)
        accel *= FastCoef;

    if (accel == vec3(0))
    {
        // decelerate naturally according to the damping value
        mMoveSpeed
            -= mMoveSpeed * std::min((1.0f / Damping) * static_cast<float>(deltaSeconds), 1.0f);
    }
    else
    {
        // acceleration
        mMoveSpeed += accel * Acceleration * static_cast<float>(deltaSeconds);
        const float maxSpeed = Movement.fast ? MaxSpeed * FastCoef : MaxSpeed;
        if (glm::length(mMoveSpeed) > maxSpeed)
            mMoveSpeed = glm::normalize(mMoveSpeed) * maxSpeed;
    }

    mCameraPosition += mMoveSpeed * static_cast<float>(deltaSeconds);
}

void FirstPersonCameraController::setPosition(const vec3& pos)
{
    mCameraPosition = pos;
}

void FirstPersonCameraController::resetMousePosition(const vec2& pos)
{
    mMousePosition = pos;
};

void FirstPersonCameraController::setUpVector(const vec3& up)
{
    const mat4 view = getViewMatrix();
    const vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
    mCameraOrientation = glm::lookAt(mCameraPosition, mCameraPosition + dir, up);
}

/*
 * Free moving camera
 */
FreeMovingCameraController::FreeMovingCameraController(const vec3& pos, const vec3& angles)
    : mPositionCurrent(pos), mPositionDesired(pos), mAnglesCurrent(angles), mAnglesDesired(angles)
{
}

mat4 FreeMovingCameraController::getViewMatrix() const
{
    return mCurrentTransform;
}

vec3 FreeMovingCameraController::getPosition() const
{
    return mPositionCurrent;
}

void FreeMovingCameraController::update(float deltaSeconds)
{
    mPositionCurrent += DampingLinear * deltaSeconds * (mPositionDesired - mPositionCurrent);

    // clip angles if "spin" is too great
    mAnglesCurrent = clipAngles(mAnglesCurrent);
    mAnglesDesired = clipAngles(mAnglesDesired);

    mAnglesCurrent
        -= angleDelta(mAnglesCurrent, mAnglesDesired) * DampingEulerAngles * deltaSeconds;

    // normalize/clip angles again if "spin" is too great
    mAnglesCurrent = clipAngles(mAnglesCurrent);

    const vec3 a = glm::radians(mAnglesCurrent);

    mCurrentTransform = glm::translate(glm::yawPitchRoll(a.y, a.x, a.z), -mPositionCurrent);
}

void FreeMovingCameraController::setPosition(const vec3& pos)
{
    mPositionCurrent = pos;
}

void FreeMovingCameraController::setAngles(float pitch, float pan, float roll)
{
    mAnglesCurrent = {pitch, pan, roll};
}

void FreeMovingCameraController::setAngles(const vec3& angles)
{
    mAnglesCurrent = angles;
}

void FreeMovingCameraController::setDesiredPosition(const vec3& pos)
{
    mPositionDesired = pos;
}

void FreeMovingCameraController::setDesiredAngles(float pitch, float pan, float roll)
{
    mAnglesDesired = {pitch, pan, roll};
}

void FreeMovingCameraController::setDesiredAngles(const vec3& angles)
{
    mAnglesDesired = angles;
}
