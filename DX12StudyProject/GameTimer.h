#pragma once
#include <Windows.h>

class GameTimer
{
public:
	GameTimer();
	~GameTimer() = default;
public:
	float TotalTime()const;//游戏运行总时间（程序运行总时间-游戏暂停总时间）
	float DeltaTime()const;//返回帧时间差
	void Reset();//重置计时器
	void Start();//开启计时器
	void Stop();//暂停计时器
	void Tick();//帧计时
private:
	double mSecondsPerCount = 0.0;//该计时器每次计数所间隔的秒数
	double mDeltaTime = -1.0;
	__int64 mStartTime = 0;//程序开始运行时的时刻
	__int64 mTotalPausdTime = 0;//程序暂停时间总和
	__int64 mStopTime = 0;//程序暂停的时刻
	__int64 mPrevTime = 0;//上次计时时刻
	__int64 mCurrentTime = 0;//当前时刻
	bool mStopped = false;
};