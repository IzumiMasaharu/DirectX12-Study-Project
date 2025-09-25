#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	position = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	look = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
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

void Camera::updateView()
{
	updateViewMatrix();
}

void Camera::move(XMVECTOR delta)
{
	position = XMVectorAdd(position, delta);
	updateViewMatrix();
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
