#pragma once
#include <Windows.h>

class GameTimer
{
public:
	GameTimer();
	~GameTimer() = default;
public:
	float TotalTime()const;//��Ϸ������ʱ�䣨����������ʱ��-��Ϸ��ͣ��ʱ�䣩
	float DeltaTime()const;//����֡ʱ���
	void Reset();//���ü�ʱ��
	void Start();//������ʱ��
	void Stop();//��ͣ��ʱ��
	void Tick();//֡��ʱ
private:
	double mSecondsPerCount = 0.0;//�ü�ʱ��ÿ�μ��������������
	double mDeltaTime = -1.0;
	__int64 mStartTime = 0;//����ʼ����ʱ��ʱ��
	__int64 mTotalPausdTime = 0;//������ͣʱ���ܺ�
	__int64 mStopTime = 0;//������ͣ��ʱ��
	__int64 mPrevTime = 0;//�ϴμ�ʱʱ��
	__int64 mCurrentTime = 0;//��ǰʱ��
	bool mStopped = false;
};