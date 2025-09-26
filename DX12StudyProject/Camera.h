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

    void rotate(float rollAngle, float pitchAngle, float yawAngle);

    // 透镜设置
    void setLens(float fov, float aspectRatio, float nearZ, float farZ);
    void zoom(float factor);
    void setFov(float fov);
    void setAspectRatio(float aspectRatio);
    void setNearZ(float nearZ);
    void setFarZ(float farZ);

    // 移动
    void move(float x, float y, float z);

    // Getter
    const DirectX::XMFLOAT3& getPositionFloat3() const { return position; }
    const DirectX::XMFLOAT3& getLookFloat3() const { return lookDir; }
    const DirectX::XMFLOAT3& getUpFloat3()   const { return upDir; }
    const DirectX::XMFLOAT3& getRightFloat3()const { return rightDir; }

    float getNearZ()   const { return nearZ; }
    float getFarZ()    const { return farZ; }
    float getFov()     const { return fov; }
    float getAspect()  const { return aspectRatio; }

    // 矩阵获取（延迟更新）
    DirectX::XMFLOAT4X4 getViewMatrix();
    DirectX::XMMATRIX   getViewMatrixXM();
    DirectX::XMFLOAT4X4 getProjMatrix();
    DirectX::XMMATRIX   getProjMatrixXM();
private:
    // roll  : 绕当前 lookDir
    // yaw   : 绕当前 right 在 XZ 平面投影轴
    // pitch : 绕相机 Y 轴
    void roll(float rollAngle);
    void pitch(float pitchAngle);
    void yaw(float yawAngle);

    void updateViewMatrix();
    void updateProjectionMatrix();
private:
    // 相机位置
    DirectX::XMFLOAT3 position;

    // 方向基向量（由 orientation 推导）
    DirectX::XMFLOAT3 lookDir;
    DirectX::XMFLOAT3 upDir;
    DirectX::XMFLOAT3 rightDir;

    // 投影参数
    float nearZ = 0.0f;
    float farZ = 0.0f;
    float fov = 0.0f;
    float aspectRatio = 0.0f;

    // 变换矩阵
    DirectX::XMFLOAT4X4 viewTransform = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 projectionTransform = MathHelper::Identity4x4();

    // 脏标记
    mutable bool viewDirty = true;
    mutable bool projDirty = true;
};