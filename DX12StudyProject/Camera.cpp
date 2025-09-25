#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
    position = { 0.0f, 0.0f, 0.0f };
    look = { 0.0f, 0.0f, 1.0f };
    up = { 0.0f, 1.0f, 0.0f };
    right = { 1.0f, 0.0f, 0.0f };
    fov = XM_PIDIV2;
    aspectRatio = 1.0f;
    nearZ = 0.1f;
    farZ = 1000.0f;
    updateViewMatrix();
    updateProjectionMatrix();
}

void Camera::setPosition(float x, float y, float z)
{
    position = { x, y, z };
    viewDirty = true; 
}
void Camera::setPosition(const DirectX::XMFLOAT3& pos)
{
    position = pos;
    viewDirty = true;
}

void Camera::lookAt(const DirectX::XMFLOAT3& target)
{
    XMVECTOR positionVec = XMLoadFloat3(&position);
    XMVECTOR targetVec = XMLoadFloat3(&target);
    XMVECTOR lookAtDir = XMVectorSubtract(targetVec, positionVec);
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    lookAt(lookAtDir, worldUp);
}
void Camera::lookAt(const DirectX::XMFLOAT3& target, float angle)
{
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR positionVec = XMLoadFloat3(&position);
    XMVECTOR targetVec = XMLoadFloat3(&target);
    XMVECTOR lookAtDir = XMVectorSubtract(targetVec, positionVec);

    lookAtDir = XMVector3Normalize(lookAtDir);
    XMVECTOR quaternion = XMQuaternionRotationAxis(lookAtDir, angle);
    worldUp = XMVector3Rotate(worldUp, quaternion);
    lookAt(lookAtDir, worldUp);
}
void Camera::lookAt(DirectX::XMVECTOR lookAtDir, DirectX::XMVECTOR worldUp)
{
    XMVECTOR lookVec = XMVector3Normalize(lookAtDir);
    XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(worldUp, lookVec));
    XMVECTOR upVec = XMVector3Cross(lookVec, rightVec);

    XMStoreFloat3(&look, lookVec);
    XMStoreFloat3(&right, rightVec);
    XMStoreFloat3(&up, upVec);

    viewDirty = true;
}

void Camera::setLens(float fov, float aspectRatio, float nearZ, float farZ)
{
	this->fov = fov;
	this->aspectRatio = aspectRatio;
	this->nearZ = nearZ;
	this->farZ = farZ;

    projDirty = true;
}
void Camera::zoom(float factor)
{
    float newFov = this->fov * factor;
    newFov = std::max(0.05f, std::min(newFov, XM_PI));

    if (abs(newFov - this->fov) > 1e-6f) {
        this->fov = newFov;
        projDirty = true;
    }
}
void Camera::setFov(float newFov)
{
    newFov = std::max(0.05f, std::min(newFov, XM_PI));

    if (abs(newFov - this->fov) > 1e-6f) {
        this->fov = newFov;
        projDirty = true;
    }
}
void Camera::setAspectRatio(float aspectRatio)
{
    if (abs(aspectRatio - this->aspectRatio) > 1e-6f) {
        this->aspectRatio = aspectRatio;
        projDirty = true;
    }
}
void Camera::setNearZ(float nearZ)
{
    nearZ = std::max(0.01f, std::min(nearZ, farZ - 0.01f));

    if (abs(nearZ - this->nearZ) > 1e-6f) {
        this->nearZ = nearZ;
        projDirty = true;
    }
}
void Camera::setFarZ(float farZ)
{
    if (abs(farZ - this->farZ) > 1e-6f) {
        this->farZ = farZ;
        projDirty = true;
    }
}

void Camera::move(DirectX::XMVECTOR delta)
{
    XMVECTOR positionVec = XMLoadFloat3(&position);
    positionVec = XMVectorAdd(positionVec, delta);
    XMStoreFloat3(&position, positionVec);
    viewDirty = true;
}

void Camera::roll(float angle)
{
    if (abs(angle) < 1e-6f) return; 
    
    XMVECTOR lookVec = XMLoadFloat3(&look);
    XMVECTOR quaternion = XMQuaternionRotationAxis(lookVec, angle);
    rotate(quaternion);
}
void Camera::pitch(float angle)
{
    if (abs(angle) < 1e-6f) return;
    
    XMVECTOR rightVec = XMLoadFloat3(&right);
    XMVECTOR quaternion = XMQuaternionRotationAxis(rightVec, angle);
    rotate(quaternion);
}
void Camera::yaw(float angle)
{
    if (abs(angle) < 1e-6f) return;
    
    XMVECTOR upVec = XMLoadFloat3(&up);
    XMVECTOR quaternion = XMQuaternionRotationAxis(upVec, angle);
    rotate(quaternion);
}

void Camera::rotateByQuaternion(float yawRadians, float pitchRadians, float rollRadians)
{
    using namespace DirectX;
    
    // 创建各轴旋转四元数
    XMVECTOR yawQuat = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), yawRadians);     // 绕Y轴(世界上方)
    XMVECTOR pitchQuat = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), pitchRadians); // 绕X轴(右方)
    XMVECTOR rollQuat = XMQuaternionRotationAxis(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rollRadians);   // 绕Z轴(前方)
    
    // 当前方向四元数
    XMVECTOR currentOrientationQuat = XMLoadFloat4(&orientation);
    
    // 合成旋转：先yaw，再pitch，最后roll
    XMVECTOR deltaRotation = XMQuaternionMultiply(XMQuaternionMultiply(yawQuat, pitchQuat), rollQuat);
    
    // 应用旋转到当前方向
    XMVECTOR newOrientationQuat = XMQuaternionMultiply(currentOrientationQuat, deltaRotation);
    
    // 归一化四元数
    newOrientationQuat = XMQuaternionNormalize(newOrientationQuat);
    
    // 保存新的方向四元数
    XMStoreFloat4(&orientation, newOrientationQuat);
    
    // 更新相机向量
    updateVectorsFromQuaternion();
}
void Camera::setOrientationFromQuaternion(const DirectX::XMFLOAT4& quat)
{
    orientation = quat;
    updateVectorsFromQuaternion();
}
void Camera::updateVectorsFromQuaternion()
{
    using namespace DirectX;
    
    XMVECTOR orientationQuat = XMLoadFloat4(&orientation);
    
    // 初始方向向量（相机默认看向-Z，上方为+Y，右方为+X）
    XMVECTOR defaultForward = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
    XMVECTOR defaultUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR defaultRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    
    // 通过四元数旋转得到当前方向向量
    XMVECTOR currentLook = XMVector3Rotate(defaultForward, orientationQuat);
    XMVECTOR currentUp = XMVector3Rotate(defaultUp, orientationQuat);
    XMVECTOR currentRight = XMVector3Rotate(defaultRight, orientationQuat);
    
    // 存储到成员变量
    XMStoreFloat3(&look, currentLook);
    XMStoreFloat3(&up, currentUp);
    XMStoreFloat3(&right, currentRight);
    
    viewDirty = true;
}

void Camera::rotate(DirectX::XMVECTOR quaternion)
{
    XMVECTOR lookVec = XMLoadFloat3(&look);
    XMVECTOR upVec = XMLoadFloat3(&up);
    XMVECTOR rightVec = XMLoadFloat3(&right);

    lookVec = XMVector3Rotate(lookVec, quaternion);
    upVec = XMVector3Rotate(upVec, quaternion);
    rightVec = XMVector3Rotate(rightVec, quaternion);

    XMStoreFloat3(&look, lookVec);
    XMStoreFloat3(&up, upVec);
    XMStoreFloat3(&right, rightVec);

    viewDirty = true;
}

void Camera::updateViewMatrix()
{
    XMVECTOR L = XMVector3Normalize(XMLoadFloat3(&look));
    XMVECTOR U = XMLoadFloat3(&up);
    XMVECTOR R = XMVector3Normalize(XMVector3Cross(U, L));
    U = XMVector3Cross(L, R);

    XMVECTOR P = XMLoadFloat3(&position);
    
    XMVECTOR dotR = XMVector3Dot(R, P);
    XMVECTOR dotU = XMVector3Dot(U, P);
    XMVECTOR dotL = XMVector3Dot(L, P);

    float x = -XMVectorGetX(dotR);
    float y = -XMVectorGetX(dotU);
    float z = -XMVectorGetX(dotL);

    XMStoreFloat3(&look, L);
    XMStoreFloat3(&up, U);
    XMStoreFloat3(&right, R);

    viewTransform._11 = XMVectorGetX(R);  viewTransform._12 = XMVectorGetX(U);  viewTransform._13 = XMVectorGetX(L);  viewTransform._14 = 0.0f;
    viewTransform._21 = XMVectorGetY(R);  viewTransform._22 = XMVectorGetY(U);  viewTransform._23 = XMVectorGetY(L);  viewTransform._24 = 0.0f;
    viewTransform._31 = XMVectorGetZ(R);  viewTransform._32 = XMVectorGetZ(U);  viewTransform._33 = XMVectorGetZ(L);  viewTransform._34 = 0.0f;
    viewTransform._41 = x;               viewTransform._42 = y;               viewTransform._43 = z;               viewTransform._44 = 1.0f;

    viewDirty = false;
}
void Camera::updateProjectionMatrix()
{
    XMMATRIX P = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
    XMStoreFloat4x4(&projectionTransform, P);

    projDirty = false;
}

// 延迟更新的getter方法
DirectX::XMFLOAT4X4 Camera::getViewMatrix()
{
    if (viewDirty) {
        updateViewMatrix();
    }
    return viewTransform;
}
XMMATRIX Camera::getViewMatrixXM()
{
    if (viewDirty) {
        updateViewMatrix();
    }
    return XMLoadFloat4x4(&viewTransform);
}
DirectX::XMFLOAT4X4 Camera::getProjMatrix()
{
    if (projDirty) {
        updateProjectionMatrix();
    }
    return projectionTransform;
}
XMMATRIX Camera::getProjMatrixXM()
{
    if (projDirty) {
        updateProjectionMatrix();
    }
    return XMLoadFloat4x4(&projectionTransform);
}