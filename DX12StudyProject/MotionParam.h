#include <DirectXMath.h>

struct MotionParam
{
	DirectX::XMFLOAT3 moveDirection{ 0.0f, 0.0f, 0.0f };
	float moveSpeed = 3.0f;

	float rollingDirection = 0.0f;
	float rollingSpeed = 0.2f;
};