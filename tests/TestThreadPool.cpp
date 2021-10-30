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
	pool.executeTask([&testValue1]{ ++testValue1; return std::any{}; }, [&testValue2](std::any&&){ ++testValue2; });
	pool.finalizeTasks();

	EXPECT_EQ(1, testValue1);
	EXPECT_EQ(1, testValue2);
}

TEST(ThreadPool, ExecuteOneTaskThreeThreads)
{
	RaccoonEcs::ThreadPool pool(3);
	int testValue1 = 0;
	int testValue2 = 0;
	pool.executeTask([&testValue1]{ ++testValue1; return std::any{}; }, [&testValue2](std::any&&){ ++testValue2; });
	pool.finalizeTasks();

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
		pool.executeTask(
			[&testValue1]{ std::atomic_fetch_add(&testValue1, 1); return std::any{}; },
			[&testValue2](std::any&&){ ++testValue2; }
		);
	}
	pool.finalizeTasks();

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
		pool.executeTask(
			[&testValue1]{ std::atomic_fetch_add(&testValue1, 1); return std::any{}; },
			[&testValue2](std::any&&){ ++testValue2; }
		);
	}
	pool.finalizeTasks();

	EXPECT_EQ(2, testValue1);
	EXPECT_EQ(2, testValue2);
}

TEST(ThreadPool, ExecuteTasksThatCanSpawnNewTasks)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> testValue1 = 0;
	int testValue2 = 0;

	auto taskLambda = [&testValue1]{ std::atomic_fetch_add(&testValue1, 1); return std::any{}; };
	auto finalizerLambda = [&taskLambda, &testValue2, &pool](std::any&&){
		++testValue2;
		pool.executeTask(taskLambda, nullptr);
		pool.executeTask(taskLambda, nullptr);
	};

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskLambda, finalizerLambda);
	}

	pool.finalizeTasks();

	EXPECT_EQ(15, testValue1);
	EXPECT_EQ(5, testValue2);
}
