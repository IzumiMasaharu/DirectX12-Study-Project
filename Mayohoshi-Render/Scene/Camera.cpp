#include "Camera.h"
#include "MathUtilities.h"
#include <algorithm>

using namespace DirectX;

Camera::Camera()
{
	position = { 0.0f, 0.0f, -10.0f };
	lookDir = { 0.0f, 0.0f, 1.0f };
	upDir = { 0.0f, 1.0f, 0.0f };
	rightDir = { 1.0f, 0.0f, 0.0f };

	fov = XM_PIDIV2;
	aspectRatio = 1.0f;
	nearZ = 0.1f;
	farZ = 1000.0f;

	viewTransform = MathUtilities::Identity4x4();
	projectionTransform = MathUtilities::Identity4x4();

	updateViewMatrix();
	updateProjectionMatrix();
}

void Camera::setPosition(float x, float y, float z)
{
	position = { x,y,z };
	viewDirty = true;
}
void Camera::setPosition(const XMFLOAT3& pos)
{
	position = pos;
	viewDirty = true;
}

void Camera::lookAt(const XMFLOAT3& target)
{
	XMVECTOR posV = XMLoadFloat3(&position);
	XMVECTOR targetV = XMLoadFloat3(&target);
	XMVECTOR lookDirV = XMVector3Normalize(targetV - posV);
	XMVECTOR upDirV = XMVectorSet(0, 1, 0, 0);
	lookAt(lookDirV, upDirV);
}
void Camera::lookAt(const XMFLOAT3& target, float rollAngle)
{
	lookAt(target);
	if (fabsf(rollAngle) > 1e-6f)
		roll(rollAngle);
}

void Camera::lookAt(XMVECTOR lookAtDir, XMVECTOR worldUp)
{
	lookAtDir = XMVector3Normalize(lookAtDir);
	XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(worldUp, lookAtDir));
	XMVECTOR upVec = XMVector3Cross(lookAtDir, rightVec);

	XMStoreFloat3(&lookDir, lookAtDir);
	XMStoreFloat3(&upDir, upVec);
	XMStoreFloat3(&rightDir, rightVec);
	viewDirty = true;
}

void Camera::rotate(float rollAngle, float pitchAngle, float yawAngle)
{
	if (rollAngle != 0.0f)  roll(rollAngle);
	if (pitchAngle != 0.0f) pitch(pitchAngle);
	if (yawAngle != 0.0f)   yaw(yawAngle);

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
void Camera::setFov(float newFov)
{
	newFov = std::max(0.05f, std::min(newFov, XM_PI * 0.75f));
	if (fabsf(newFov - fov) > 1e-6f) {
		fov = newFov;
		projDirty = true;
	}
}
void Camera::setAspectRatio(float aspectRatio)
{
	if (fabsf(aspectRatio - this->aspectRatio) > 1e-6f) {
		this->aspectRatio = aspectRatio;
		projDirty = true;
	}
}
void Camera::setNearZ(float nearZ)
{
	nearZ = std::max(0.01f, std::min(nearZ, farZ - 0.01f));
	if (fabsf(nearZ - this->nearZ) > 1e-6f) {
		this->nearZ = nearZ;
		projDirty = true;
	}
}
void Camera::setFarZ(float farZ)
{
	if (fabsf(farZ - this->farZ) > 1e-6f) {
		this->farZ = farZ;
		projDirty = true;
	}
}

void Camera::move(float x, float y, float z)
{
	XMFLOAT3 delta(x, y, z);
	XMVECTOR posV = XMLoadFloat3(&position);
	posV = XMVectorAdd(posV, XMLoadFloat3(&delta));
	XMStoreFloat3(&position, posV);
	viewDirty = true;
}
void Camera::move(const XMFLOAT3& delta)
{
	XMVECTOR posV = XMLoadFloat3(&position);
	posV = XMVectorAdd(posV, XMLoadFloat3(&delta));
	XMStoreFloat3(&position, posV);
	viewDirty = true;
}
void Camera::moveForward_Backward(float distance)
{
	XMVECTOR posV = XMLoadFloat3(&position);
	XMVECTOR lookV = XMLoadFloat3(&lookDir);
	posV = XMVectorAdd(posV, XMVectorScale(lookV, distance));
	XMStoreFloat3(&position, posV);
	viewDirty = true;
}
void Camera::moveRight_left(float distance)
{
	XMVECTOR posV = XMLoadFloat3(&position);
	XMVECTOR rightV = XMLoadFloat3(&rightDir);
	// 投影到 XZ 平面
	XMVECTOR proj = XMVectorSet(XMVectorGetX(rightV), 0.0f, XMVectorGetZ(rightV), 0.0f);
	if (XMVector3LengthSq(proj).m128_f32[0] < 1e-6f)
	{
		proj = XMVectorSet(1, 0, 0, 0); // fallback
	}
	proj = XMVector3Normalize(proj);

	posV = XMVectorAdd(posV, XMVectorScale(proj, distance));
	XMStoreFloat3(&position, posV);
	viewDirty = true;
}
void Camera::fly_drop(float distance)
{
	XMVECTOR posV = XMLoadFloat3(&position);
	XMVECTOR upV = XMVectorSet(0, 1, 0, 0);
	posV = XMVectorAdd(posV, XMVectorScale(upV, distance));
	XMStoreFloat3(&position, posV);
	viewDirty = true;
}

// roll: 绕当前 lookDir
void Camera::roll(float rollAngle)
{
	XMVECTOR look = XMVector3Normalize(XMLoadFloat3(&lookDir));
	XMMATRIX R = XMMatrixRotationAxis(look, rollAngle);

	XMVECTOR right = XMVector3TransformNormal(XMLoadFloat3(&rightDir), R);
	XMVECTOR up = XMVector3TransformNormal(XMLoadFloat3(&upDir), R);

	right = XMVector3Normalize(right);
	up = XMVector3Normalize(up);

	XMStoreFloat3(&rightDir, right);
	XMStoreFloat3(&upDir, up);
}
// yaw: 绕全局 Y 轴
void Camera::yaw(float yawAngle)
{
	XMVECTOR worldY = XMVectorSet(0, 1, 0, 0);
	XMMATRIX R = XMMatrixRotationAxis(worldY, yawAngle);

	XMVECTOR look = XMVector3TransformNormal(XMLoadFloat3(&lookDir), R);
	XMVECTOR right = XMVector3TransformNormal(XMLoadFloat3(&rightDir), R);
	XMVECTOR up = XMVector3Cross(look, right);

	look = XMVector3Normalize(look);
	right = XMVector3Normalize(right);
	up = XMVector3Normalize(up);

	XMStoreFloat3(&lookDir, look);
	XMStoreFloat3(&rightDir, right);
	XMStoreFloat3(&upDir, up);
}
// pitch: 绕 right 在 XZ 平面的投影
void Camera::pitch(float pitchAngle)
{
	XMVECTOR right = XMLoadFloat3(&rightDir);

	// 投影到 XZ 平面
	XMVECTOR proj = XMVectorSet(XMVectorGetX(right), 0.0f, XMVectorGetZ(right), 0.0f);
	if (XMVector3LengthSq(proj).m128_f32[0] < 1e-6f)
	{
		proj = XMVectorSet(0, 1, 0, 0); // fallback
	}
	proj = XMVector3Normalize(proj);

	XMMATRIX R = XMMatrixRotationAxis(proj, pitchAngle);

	XMVECTOR look = XMVector3TransformNormal(XMLoadFloat3(&lookDir), R);
	XMVECTOR up = XMVector3TransformNormal(XMLoadFloat3(&upDir), R);
	right = XMVector3Cross(up, look);

	look = XMVector3Normalize(look);
	up = XMVector3Normalize(up);
	right = XMVector3Normalize(right);

	XMStoreFloat3(&lookDir, look);
	XMStoreFloat3(&upDir, up);
	XMStoreFloat3(&rightDir, right);
}

void Camera::updateViewMatrix()
{
	XMVECTOR posV = XMLoadFloat3(&position);
	XMVECTOR lookV = XMLoadFloat3(&lookDir);
	XMVECTOR upV = XMLoadFloat3(&upDir);

	XMMATRIX V = XMMatrixLookToLH(posV, lookV, upV);
	XMStoreFloat4x4(&viewTransform, V);

	viewDirty = false;
}
void Camera::updateProjectionMatrix()
{
	XMMATRIX P = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
	XMStoreFloat4x4(&projectionTransform, P);

	projDirty = false;
}

XMFLOAT4X4 Camera::getViewMatrix()
{
	if (viewDirty) 
		updateViewMatrix();
	return viewTransform;
}
XMMATRIX Camera::getViewMatrixXM()
{
	if (viewDirty)
		updateViewMatrix();
	return XMLoadFloat4x4(&viewTransform);
}
XMFLOAT4X4 Camera::getProjMatrix()
{
	if (projDirty) 
		updateProjectionMatrix();
	return projectionTransform;
}
XMMATRIX Camera::getProjMatrixXM()
{
	if (projDirty) 
		updateProjectionMatrix();
	return XMLoadFloat4x4(&projectionTransform);
}