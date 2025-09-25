#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	position = { 0.0f, 0.0f, 0.0f };
	look = { 0.0f, 0.0f, 1.0f };
	up = { 0.0f, 1.0f, 0.0f };
	right = { 1.0f, 0.0f, 0.0f };
}

void Camera::setPosition(float x, float y, float z)
{
	position = { x, y, z };
	updateViewMatrix();
}

void Camera::lookAt(DirectX::XMFLOAT3 target)
{
	// 默认Up方向为Y轴正方向
	XMVECTOR lookAtDir = XMVectorSubtract(XMLoadFloat3(&target), position);
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	lookAt(lookAtDir, worldUp);
}
void Camera::lookAt(DirectX::XMFLOAT3 target， float angle)
{
	// 指定Up方向时绕Look轴旋转指定角度
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookAtDir = XMVectorSubtract(XMLoadFloat3(&target), position);
	XMVECTOR quaternion = XMQuaternionRotationAxis(XMVector3Normalize(lookAtDir), angle);
	worldUp = XMVector3Rotate(worldUp, quaternion);
	lookAt(lookAtDir, worldUp);
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

	XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, aspectRatio, zn, zf);
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
	XMVECTOR L = XMVector3Normalize(look);
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(up, L));
	XMVECTOR U = XMVector3Cross(L, R);

	float x = -XMVectorGetX(XMVector3Dot(R, position));
	float y = -XMVectorGetY(XMVector3Dot(U, position));
	float z = -XMVectorGetZ(XMVector3Dot(L, position));

	XMStoreFloat3(&position, position);
	XMStoreFloat3(&look, L);
	XMStoreFloat3(&up, U);
	XMStoreFloat3(&right, R);

	viewTransform(0, 0) = XMVectorGetX(R);
	viewTransform(1, 0) = XMVectorGetY(R);
	viewTransform(2, 0) = XMVectorGetZ(R);
	viewTransform(3, 0) = x;

	viewTransform(0, 1) = XMVectorGetX(U);
	viewTransform(1, 1) = XMVectorGetY(U);
	viewTransform(2, 1) = XMVectorGetZ(U);
	viewTransform(3, 1) = y;

	viewTransform(0, 2) = XMVectorGetX(L);
	viewTransform(1, 2) = XMVectorGetY(L);
	viewTransform(2, 2) = XMVectorGetZ(L);
	viewTransform(3, 2) = z;

	viewTransform(0, 3) = 0.0f;
	viewTransform(1, 3) = 0.0f;
	viewTransform(2, 3) = 0.0f;
	viewTransform(3, 3) = 1.0f;
}