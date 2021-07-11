#include <gtest/gtest.h>

#include <functional>
#include <vector>
#include <atomic>

#include "raccoon-ecs/thread_pool.h"

TEST(ThreadPool, DefaultInitializeSpawnThreadsAndDestroy)
{
	RaccoonEcs::ThreadPool pool;
	pool.spawnThreads(3);
}

TEST(ThreadPool, ExecuteOneTaskOneThread)
{
	RaccoonEcs::ThreadPool pool(1);
	int testValue1 = 0;
	int testValue2 = 0;
	pool.submitTask([&testValue1]{ ++testValue1; }, [&testValue2]{ ++testValue2; });
	pool.executeAll();

	EXPECT_EQ(1, testValue1);
	EXPECT_EQ(1, testValue2);
}

TEST(ThreadPool, ExecuteOneTaskThreeThreads)
{
	RaccoonEcs::ThreadPool pool(3);
	int testValue1 = 0;
	int testValue2 = 0;
	pool.submitTask([&testValue1]{ ++testValue1; }, [&testValue2]{ ++testValue2; });
	pool.executeAll();

	EXPECT_EQ(1, testValue1);
	EXPECT_EQ(1, testValue2);
}

TEST(ThreadPool, ExecuteTenTasksThreeThreads)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> testValue1 = 0;
	int testValue2 = 0;
	for (int i = 0; i < 10; ++i)
	{
		pool.submitTask(
			[&testValue1]{ std::atomic_fetch_add(&testValue1, 1); },
			[&testValue2]{ ++testValue2; }
		);
	}
	pool.executeAll();

	EXPECT_EQ(10, testValue1);
	EXPECT_EQ(10, testValue2);
}

TEST(ThreadPool, ExecuteTwoTasksThreeThreads)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> testValue1 = 0;
	int testValue2 = 0;
	for (int i = 0; i < 2; ++i)
	{
		pool.submitTask(
			[&testValue1]{ std::atomic_fetch_add(&testValue1, 1); },
			[&testValue2]{ ++testValue2; }
		);
	}
	pool.executeAll();

	EXPECT_EQ(2, testValue1);
	EXPECT_EQ(2, testValue2);
}
