#pragma once
#include "DXBase.h"

class Camera
{
public:
	Camera();
	~Camera() = default;

	void setPosition(float x,float y,float z);
	void setLens(float fovY, float aspectRatio, float zn, float zf);
	void updateView();
	void move(DirectX::XMVECTOR delta);
	void rotate(DirectX::XMVECTOR quaternion);

	DirectX::XMFLOAT4X4 getViewMatrix() const { return viewTransform; }
	DirectX::XMFLOAT4X4 getProjMatrix() const { return projectionTransform; }

private:
	void updateViewMatrix();

private:
	DirectX::XMVECTOR position;
	DirectX::XMVECTOR look;
	DirectX::XMVECTOR up;
	DirectX::XMVECTOR right;

	float nearZ = 0.0f;
	float farZ = 0.0f;
	float fov = 0.0f;
	float aspect = 0.0f;

	DirectX::XMFLOAT4X4 viewTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 projectionTransform = MathHelper::Identity4x4();
};
