#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	position = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	look = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
}

void Camera::setPosition(float x, float y, float z)
{
	position = XMVectorSet(x, y, z, 1.0f);
	updateViewMatrix();
}

void Camera::lookAt(DirectX::XMFLOAT3 target)
{
	// 未指定Up方向时尽可能取正
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookAtDir = XMVectorSubtract(XMLoadFloat3(&target), position);
	lookAt(lookAtDir, worldUp);
}
void Camera::lookAt(DirectX::XMFLOAT3 target， float angle)
{
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookAtDir = XMVectorSubtract(XMLoadFloat3(&target), position);
	lookAt(lookAtDir, worldUp);
	XMVECTOR quaternion = XMQuaternionRotationAxis(look, angle);
	rotate(quaternion);
}
void Camera::lookAt(DirectX::XMVECTOR lookAtDir, DirectX::XMVECTOR worldUp)
{
	look = XMVector3Normalize(lookAtDir);
	right = XMVector3Normalize(XMVector3Cross(worldUp, look));
	up = XMVector3Cross(look, right);
	updateViewMatrix();
}

void Camera::setLens(float fovY, float aspectRatio, float zn, float zf)
{
	fov = fovY;
	aspect = aspectRatio;
	nearZ = zn;
	farZ = zf;

	XMMATRIX P = XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);
	XMStoreFloat4x4(&projectionTransform, P);
}

void Camera::move(XMVECTOR delta)
{
	position = XMVectorAdd(position, delta);
	updateViewMatrix();
}

void Camera::roll(float angle)
{
	XMVECTOR quaternion = XMQuaternionRotationAxis(look, angle);
	rotate(quaternion);
}

void Camera::pitch(float angle)
{
	XMVECTOR quaternion = XMQuaternionRotationAxis(right, angle);
	rotate(quaternion);
}

void Camera::yaw(float angle)
{
	XMVECTOR quaternion = XMQuaternionRotationAxis(up, angle);
	rotate(quaternion);
}


void Camera::rotate(XMVECTOR quaternion)
{
	look = XMVector3Rotate(look, quaternion);
	up = XMVector3Rotate(up, quaternion);
	right = XMVector3Rotate(right, quaternion);
	updateViewMatrix();
}

void Camera::updateViewMatrix()
{
	look = XMVector3Normalize(look);
	right = XMVector3Normalize(XMVector3Cross(up, look));
	up = XMVector3Cross(look, right);

	float x = -XMVectorGetX(XMVector3Dot(position, right));
	float y = -XMVectorGetX(XMVector3Dot(position, up));
	float z = -XMVectorGetX(XMVector3Dot(position, look));

	viewTransform = {
		XMVectorGetX(right), XMVectorGetX(up), XMVectorGetX(look), 0.0f,
		XMVectorGetY(right), XMVectorGetY(up), XMVectorGetY(look), 0.0f,
		XMVectorGetZ(right), XMVectorGetZ(up), XMVectorGetZ(look), 0.0f,
		x,                   y,                z,                1.0f
	};
}