#include "GameTimer.h"

GameTimer::GameTimer()
{
	__int64 CountsPerSeconcd;
	QueryPerformanceFrequency((LARGE_INTEGER*)&CountsPerSeconcd);
	mSecondsPerCount = 1.0 / (double)CountsPerSeconcd;
}
//��Ϸ������ʱ�䣨����������ʱ��-��Ϸ��ͣ��ʱ�䣩
float GameTimer::TotalTime()const
{
	if (mStopped)
		return (mStopTime - mStartTime - mTotalPausdTime) * mSecondsPerCount;
	else
		return (mCurrentTime - mStartTime - mTotalPausdTime) * mSecondsPerCount;
}
//����֡ʱ���
float GameTimer::DeltaTime()const
{
	return (float)mDeltaTime;
}
//���ü�ʱ��
void GameTimer::Reset()
{
	__int64 CurrentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);

	mStartTime = CurrentTime;
	mStopTime = 0;
	mPrevTime = CurrentTime;
	mStopped = false;
}
//������ʱ��
void GameTimer::Start()
{
	if (mStopped)
	{
		__int64 CurrentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);

		mTotalPausdTime += (CurrentTime - mStopTime);
		mPrevTime = CurrentTime;//�轫��ͣʱǰһ���ʱ�̱�Ϊ��ʼʱ��
		mStopTime = 0;
		mStopped = false;
	}
}
//��ͣ��ʱ��
void GameTimer::Stop()
{
	if (!mStopped)
	{
		__int64 CurrentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);
		mStopTime = CurrentTime;

		mStopped = true;
	}
}
//֡��ʱ
void GameTimer::Tick()
{
	__int64 CurrentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);
	mCurrentTime = CurrentTime;
	mDeltaTime = (mCurrentTime - mPrevTime) * mSecondsPerCount;
	mPrevTime = mCurrentTime;

	if (mDeltaTime < 0.0)
		 mDeltaTime = 0.0;//��ֹ��Ϊ�ⲿԭ���¦�t<0
}