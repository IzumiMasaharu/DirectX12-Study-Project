#include "GameTimer.h"

GameTimer::GameTimer()
{
	__int64 CountsPerSeconcd;
	QueryPerformanceFrequency((LARGE_INTEGER*)&CountsPerSeconcd);
	mSecondsPerCount = 1.0 / (double)CountsPerSeconcd;
}
//游戏运行总时间（程序运行总时间-游戏暂停总时间）
float GameTimer::TotalTime()const
{
	if (mStopped)
		return (mStopTime - mStartTime - mTotalPausdTime) * mSecondsPerCount;
	else
		return (mCurrentTime - mStartTime - mTotalPausdTime) * mSecondsPerCount;
}
//返回帧时间差
float GameTimer::DeltaTime()const
{
	return (float)mDeltaTime;
}
//重置计时器
void GameTimer::Reset()
{
	__int64 CurrentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);

	mStartTime = CurrentTime;
	mStopTime = 0;
	mPrevTime = CurrentTime;
	mStopped = false;
}
//开启计时器
void GameTimer::Start()
{
	if (mStopped)
	{
		__int64 CurrentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);

		mTotalPausdTime += (CurrentTime - mStopTime);
		mPrevTime = CurrentTime;//需将暂停时前一阵的时刻变为开始时刻
		mStopTime = 0;
		mStopped = false;
	}
}
//暂停计时器
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
//帧计时
void GameTimer::Tick()
{
	__int64 CurrentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);
	mCurrentTime = CurrentTime;
	mDeltaTime = (mCurrentTime - mPrevTime) * mSecondsPerCount;
	mPrevTime = mCurrentTime;

	if (mDeltaTime < 0.0)
		 mDeltaTime = 0.0;//防止因为外部原因导致Δt<0
}