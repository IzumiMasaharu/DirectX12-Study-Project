#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
    position = { 0.0f, 0.0f, 0.0f };
    look = { 0.0f, 0.0f, 1.0f };
    up = { 0.0f, 1.0f, 0.0f };
    right = { 1.0f, 0.0f, 0.0f };
    updateViewMatrix();  // 初始化时计算一次
}

void Camera::setPosition(float x, float y, float z)
{
    position = { x, y, z };
    viewDirty = true;  // 标记需要更新
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
    
    // 避免重复计算normalize
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

void Camera::move(DirectX::XMVECTOR delta)
{
    XMVECTOR positionVec = XMLoadFloat3(&position);
    positionVec = XMVectorAdd(positionVec, delta);
    XMStoreFloat3(&position, positionVec);
    viewDirty = true;
}

void Camera::roll(float angle)
{
    if (abs(angle) < 1e-6f) return;  // 避免微小旋转
    
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
    
    // 一次性计算所有点积
    XMVECTOR dotR = XMVector3Dot(R, P);
    XMVECTOR dotU = XMVector3Dot(U, P);
    XMVECTOR dotL = XMVector3Dot(L, P);

    float x = -XMVectorGetX(dotR);
    float y = -XMVectorGetX(dotU);
    float z = -XMVectorGetX(dotL);

    // 更新存储的向量
    XMStoreFloat3(&look, L);
    XMStoreFloat3(&up, U);
    XMStoreFloat3(&right, R);

    // 直接构建视图矩阵
    viewTransform._11 = XMVectorGetX(R);  viewTransform._12 = XMVectorGetX(U);  viewTransform._13 = XMVectorGetX(L);  viewTransform._14 = 0.0f;
    viewTransform._21 = XMVectorGetY(R);  viewTransform._22 = XMVectorGetY(U);  viewTransform._23 = XMVectorGetY(L);  viewTransform._24 = 0.0f;
    viewTransform._31 = XMVectorGetZ(R);  viewTransform._32 = XMVectorGetZ(U);  viewTransform._33 = XMVectorGetZ(L);  viewTransform._34 = 0.0f;
    viewTransform._41 = x;               viewTransform._42 = y;               viewTransform._43 = z;               viewTransform._44 = 1.0f;

    viewDirty = false;
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