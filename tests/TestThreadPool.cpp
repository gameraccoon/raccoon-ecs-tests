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
	int taskValue = 0;
	int finalizeValue = 0;
	pool.executeTask([&taskValue]{ ++taskValue; return std::any{}; }, [&finalizeValue](std::any&&){ ++finalizeValue; });
	pool.finalizeTasks();

	EXPECT_EQ(1, taskValue);
	EXPECT_EQ(1, finalizeValue);
}

TEST(ThreadPool, ExecuteOneTaskThreeThreads)
{
	RaccoonEcs::ThreadPool pool(3);
	int taskValue = 0;
	int finalizeValue = 0;
	pool.executeTask([&taskValue]{ ++taskValue; return std::any{}; }, [&finalizeValue](std::any&&){ ++finalizeValue; });
	pool.finalizeTasks();

	EXPECT_EQ(1, taskValue);
	EXPECT_EQ(1, finalizeValue);
}

TEST(ThreadPool, ExecuteTenTasksThreeThreads)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> taskValue = 0;
	int finalizeValue = 0;
	for (int i = 0; i < 10; ++i)
	{
		pool.executeTask(
			[&taskValue]{ std::atomic_fetch_add(&taskValue, 1); return std::any{}; },
			[&finalizeValue](std::any&&){ ++finalizeValue; }
		);
	}
	pool.finalizeTasks();

	EXPECT_EQ(10, taskValue);
	EXPECT_EQ(10, finalizeValue);
}

TEST(ThreadPool, ExecuteTwoTasksThreeThreads)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> taskValue = 0;
	int finalizeValue = 0;
	for (int i = 0; i < 2; ++i)
	{
		pool.executeTask(
			[&taskValue]{ std::atomic_fetch_add(&taskValue, 1); return std::any{}; },
			[&finalizeValue](std::any&&){ ++finalizeValue; }
		);
	}
	pool.finalizeTasks();

	EXPECT_EQ(2, taskValue);
	EXPECT_EQ(2, finalizeValue);
}

TEST(ThreadPool, PassTaskResultToFinalizer)
{
	RaccoonEcs::ThreadPool pool(1);
	int taskValue = 0;
	int finalizeValue = 0;

	pool.executeTask([&taskValue]{ ++taskValue; return taskValue * 10; },
	[&finalizeValue](std::any&& result){ finalizeValue += std::any_cast<int>(result); });

	pool.finalizeTasks();

	EXPECT_EQ(1, taskValue);
	EXPECT_EQ(10, finalizeValue);
}

TEST(ThreadPool, DestroyPoolWithoutFinalization)
{
	RaccoonEcs::ThreadPool pool(2);

	pool.executeTask([]{ return std::any(); }, [](std::any&&){});
}

TEST(ThreadPool, ExecuteTasksThatCanSpawnNewTasks)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> taskValue = 0;
	int finalizeValue = 0;

	auto taskLambda = [&taskValue]{ std::atomic_fetch_add(&taskValue, 1); return std::any{}; };
	auto finalizerLambda = [&taskLambda, &finalizeValue, &pool](std::any&&){
		++finalizeValue;
		pool.executeTask(taskLambda, nullptr);
		pool.executeTask(taskLambda, nullptr);
	};

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskLambda, finalizerLambda);
	}

	pool.finalizeTasks();

	EXPECT_EQ(15, taskValue);
	EXPECT_EQ(5, finalizeValue);
}

TEST(ThreadPool, RunTwoTaskGroupsSequentially)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> taskValue = 0;
	int finalizeValue = 0;

	auto taskLambda = [&taskValue]{ std::atomic_fetch_add(&taskValue, 1); return std::any{}; };
	auto finalizerLambda = [&finalizeValue](std::any&&){ ++finalizeValue; };

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskLambda, finalizerLambda);
	}

	pool.finalizeTasks();

	EXPECT_EQ(5, taskValue);
	EXPECT_EQ(5, finalizeValue);

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskLambda, finalizerLambda, 1);
	}

	pool.finalizeTasks(1);

	EXPECT_EQ(10, taskValue);
	EXPECT_EQ(10, finalizeValue);
}

TEST(ThreadPool, RunTwoTaskGroupsParallelDirectOrder)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> taskValue = 0;
	int finalizeValue = 0;

	auto taskLambda = [&taskValue]{ std::atomic_fetch_add(&taskValue, 1); return std::any{}; };
	auto finalizerLambda = [&finalizeValue](std::any&&){ ++finalizeValue; };

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskLambda, finalizerLambda);
	}

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskLambda, finalizerLambda, 1);
	}

	pool.finalizeTasks();

	EXPECT_EQ(5, finalizeValue);

	pool.finalizeTasks(1);

	EXPECT_EQ(10, taskValue);
	EXPECT_EQ(10, finalizeValue);
}

TEST(ThreadPool, RunTwoTaskGroupsParallelReverseOrder)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> taskValue = 0;
	int finalizeValue = 0;

	auto taskLambda = [&taskValue]{ std::atomic_fetch_add(&taskValue, 1); return std::any{}; };
	auto finalizerLambda = [&finalizeValue](std::any&&){ ++finalizeValue; };

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskLambda, finalizerLambda);
	}

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskLambda, finalizerLambda, 1);
	}

	pool.finalizeTasks(1);

	EXPECT_EQ(5, finalizeValue);

	pool.finalizeTasks();

	EXPECT_EQ(10, taskValue);
	EXPECT_EQ(10, finalizeValue);
}

TEST(ThreadPool, RunTwoTaskGroupsOneInTaskOfOtherWithEnoughWorkingThreads)
{
	RaccoonEcs::ThreadPool pool(6);
	std::atomic<int> taskValueInner = 0;
	std::atomic<int> taskValueOuter = 0;
	int finalizeValueInner = 0;
	int finalizeValueOuter = 0;

	auto taskInnerLambda = [&taskValueInner]{ std::atomic_fetch_add(&taskValueInner, 1); return std::any{}; };
	auto finalizerInnerLambda = [&finalizeValueInner](std::any&&){ ++finalizeValueInner; };

	auto taskOuterLambda = [&pool, &taskInnerLambda, &finalizerInnerLambda, &taskValueOuter]{
		std::atomic_fetch_add(&taskValueOuter, 1);
		pool.executeTask(taskInnerLambda, finalizerInnerLambda, 1);
		pool.executeTask(taskInnerLambda, finalizerInnerLambda, 1);
		pool.finalizeTasks(1);
		return std::any{};
	};
	auto finalizerOuterLambda = [&finalizeValueOuter](std::any&&){ ++finalizeValueOuter; };

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskOuterLambda, finalizerOuterLambda);
	}

	pool.finalizeTasks();

	EXPECT_EQ(10, taskValueInner);
	EXPECT_EQ(5, taskValueOuter);
	EXPECT_EQ(10, finalizeValueInner);
	EXPECT_EQ(5, finalizeValueOuter);
}

// this is not supported yet, however the support is needed
TEST(ThreadPool, DISABLED_RunTwoTaskGroupsOneInTaskOfOtherWithLowAmountOfThreads)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> taskValueInner = 0;
	std::atomic<int> taskValueOuter = 0;
	int finalizeValueInner = 0;
	int finalizeValueOuter = 0;

	auto taskInnerLambda = [&taskValueInner]{ std::atomic_fetch_add(&taskValueInner, 1); return std::any{}; };
	auto finalizerInnerLambda = [&finalizeValueInner](std::any&&){ ++finalizeValueInner; };

	auto taskOuterLambda = [&pool, &taskInnerLambda, &finalizerInnerLambda, &taskValueOuter]{
		std::atomic_fetch_add(&taskValueOuter, 1);
		pool.executeTask(taskInnerLambda, finalizerInnerLambda, 1);
		pool.executeTask(taskInnerLambda, finalizerInnerLambda, 1);
		pool.finalizeTasks(1);
		return std::any{};
	};
	auto finalizerOuterLambda = [&finalizeValueOuter](std::any&&){ ++finalizeValueOuter; };

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskOuterLambda, finalizerOuterLambda);
	}

	pool.finalizeTasks();

	EXPECT_EQ(10, taskValueInner);
	EXPECT_EQ(5, taskValueOuter);
	EXPECT_EQ(10, finalizeValueInner);
	EXPECT_EQ(5, finalizeValueOuter);
}

TEST(ThreadPool, RunTwoTaskGroupsOneInFinalizerOfOther)
{
	RaccoonEcs::ThreadPool pool(3);
	std::atomic<int> taskValueInner = 0;
	std::atomic<int> taskValueOuter = 0;
	int finalizeValueInner = 0;
	int finalizeValueOuter = 0;

	auto taskInnerLambda = [&taskValueInner]{ std::atomic_fetch_add(&taskValueInner, 1); return std::any{}; };
	auto finalizerInnerLambda = [&finalizeValueInner](std::any&&){ ++finalizeValueInner; };

	auto taskOuterLambda = [&taskValueOuter]{ std::atomic_fetch_add(&taskValueOuter, 1); return std::any{}; };
	auto finalizerOuterLambda = [&pool, &taskInnerLambda, &finalizerInnerLambda, &finalizeValueOuter](std::any&&){
		++finalizeValueOuter;
		pool.executeTask(taskInnerLambda, finalizerInnerLambda, 1);
		pool.executeTask(taskInnerLambda, finalizerInnerLambda, 1);
		pool.finalizeTasks(1);
	};

	for (int i = 0; i < 5; ++i)
	{
		pool.executeTask(taskOuterLambda, finalizerOuterLambda);
	}

	pool.finalizeTasks();

	EXPECT_EQ(10, taskValueInner);
	EXPECT_EQ(5, taskValueOuter);
	EXPECT_EQ(10, finalizeValueInner);
	EXPECT_EQ(5, finalizeValueOuter);
}
