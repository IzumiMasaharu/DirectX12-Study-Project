#pragma once
#include "DXBase.h"

class Camera
{
public:
	Camera() = delete;
	~Camera() = default;

	void setPosition(float x,float y,float z);
	void lookAt(DirectX::XMFLOAT3 target);
	void lookAt(DirectX::XMFLOAT3 target, float angle);
	void lookAt(DirectX::XMVECTOR lookAtDir, DirectX::XMVECTOR worldUp);
	void setLens(float fovY, float aspectRatio, float zn, float zf);

	void move(DirectX::XMVECTOR delta);
	void roll(float angle);
	void pitch(float angle);
	void yaw(float angle);

	DirectX::XMVECTOR getPosition() const { return position; }
	DirectX::XMVECTOR getLook() const { return look; }
	DirectX::XMVECTOR getUp() const { return up; }
	DirectX::XMVECTOR getRight() const { return right; }

	float getNearZ() const { return nearZ; }
	float getFarZ() const { return farZ; }
	float getFov() const { return fov; }
	float getAspect() const { return aspect; }

	DirectX::XMFLOAT4X4 getViewMatrix() const { return viewTransform; }
	DirectX::XMFLOAT4X4 getProjMatrix() const { return projectionTransform; }
private:
	void rotate(DirectX::XMVECTOR quaternion);
	void updateViewMatrix();
private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 look;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 right;

	float nearZ = 0.0f;
	float farZ = 0.0f;
	float fov = 0.0f;
	float aspect = 0.0f;

	DirectX::XMFLOAT4X4 viewTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 projectionTransform = MathHelper::Identity4x4();
};
