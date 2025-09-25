#pragma once
#include "DXBase.h"

class Camera
{
public:
    Camera();
    ~Camera() = default;

    // 位置设置
    void setPosition(float x, float y, float z);
    void setPosition(const DirectX::XMFLOAT3& position);
    
    // 朝向设置
    void lookAt(const DirectX::XMFLOAT3& target);
    void lookAt(const DirectX::XMFLOAT3& target, float rollAngle);
    void lookAt(DirectX::XMVECTOR lookAtDir, DirectX::XMVECTOR worldUp);
    
    // 透镜设置
    void setLens(float fovY, float aspectRatio, float nearZ, float farZ);

    // 移动方法
    void move(DirectX::XMVECTOR delta);
    
    // 旋转方法
    void roll(float angle);
    void pitch(float angle);
    void yaw(float angle);

    // Getter 方法（内联优化）
    const DirectX::XMFLOAT3& getPositionFloat3() const { return position; }
    const DirectX::XMFLOAT3& getLookFloat3() const { return look; }
    const DirectX::XMFLOAT3& getUpFloat3() const { return up; }
    const DirectX::XMFLOAT3& getRightFloat3() const { return right; }
    
    DirectX::XMVECTOR getPosition() const { return DirectX::XMLoadFloat3(&position); }
    DirectX::XMVECTOR getLook() const { return DirectX::XMLoadFloat3(&look); }
    DirectX::XMVECTOR getUp() const { return DirectX::XMLoadFloat3(&up); }
    DirectX::XMVECTOR getRight() const { return DirectX::XMLoadFloat3(&right); }

    float getNearZ() const { return nearZ; }
    float getFarZ() const { return farZ; }
    float getFov() const { return fov; }
    float getAspect() const { return aspect; }

    // 矩阵获取（延迟更新）
    DirectX::XMFLOAT4X4 getViewMatrix();
    DirectX::XMMATRIX getViewMatrixXM();
    const DirectX::XMFLOAT4X4& getProjMatrix() const { return projectionTransform; }
    DirectX::XMMATRIX getProjMatrixXM() const { return DirectX::XMLoadFloat4x4(&projectionTransform); }

private:
    void rotate(DirectX::XMVECTOR quaternion);
    void updateViewMatrix();

private:
    // 相机状态
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 look;
    DirectX::XMFLOAT3 up;
    DirectX::XMFLOAT3 right;

    // 投影参数
    float nearZ = 0.0f;
    float farZ = 0.0f;
    float fov = 0.0f;
    float aspect = 0.0f;

    // 变换矩阵
    DirectX::XMFLOAT4X4 viewTransform = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 projectionTransform = MathHelper::Identity4x4();
    
    // 优化标志
    mutable bool viewDirty = true;  // 标记视图矩阵是否需要更新
};