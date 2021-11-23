#include <gtest/gtest.h>

#include <thread>

#include "raccoon-ecs/async_stack.h"

TEST(AsyncStack, EmptyStackPop)
{
	RaccoonEcs::AsyncStack<int> stack;
	int a;
	EXPECT_FALSE(stack.popFront(a));
}

TEST(AsyncStack, PushAndPop)
{
	RaccoonEcs::AsyncStack<int> stack;

	stack.pushFront(10);

	int a;
	EXPECT_TRUE(stack.popFront(a));
	EXPECT_EQ(10, a);
}

TEST(AsyncStack, PushMultiplePopOneAndDestroy)
{
	RaccoonEcs::AsyncStack<int> stack;

	stack.pushFront(10);
	stack.pushFront(20);
	stack.pushFront(30);
	stack.pushFront(40);

	int a;
	EXPECT_TRUE(stack.popFront(a));
	EXPECT_EQ(40, a);
}

TEST(AsyncStack, PushMultiplePopMultiple)
{
	RaccoonEcs::AsyncStack<int> stack;

	stack.pushFront(10);
	stack.pushFront(20);
	stack.pushFront(30);
	stack.pushFront(40);

	int a;
	EXPECT_TRUE(stack.popFront(a));
	EXPECT_EQ(40, a);
	EXPECT_TRUE(stack.popFront(a));
	EXPECT_EQ(30, a);
	EXPECT_TRUE(stack.popFront(a));
	EXPECT_EQ(20, a);
	EXPECT_TRUE(stack.popFront(a));
	EXPECT_EQ(10, a);
	EXPECT_FALSE(stack.popFront(a));
}

TEST(AsyncStack, ProduceAndConsumeTwoThreads)
{
	RaccoonEcs::AsyncStack<int> stack;
	const int itemsCount = 20000;

	std::vector<int> results;
	std::thread consumer([&results, &stack]{
		int a;
		int itemsLeft = itemsCount;
		while(itemsLeft > 0)
		{
			if (stack.popFront(a))
			{
				--itemsLeft;
				results.push_back(a);
			}
			else
			{
				std::this_thread::yield();
			}
		}
	});

	std::thread producer([&stack]{
		for (int i = 0; i < itemsCount; ++i)
		{
			stack.pushFront(i * 10);
		}
	});

	producer.join();
	consumer.join();

	ASSERT_EQ(static_cast<size_t>(itemsCount), results.size());
	// order is not guaranteed
	std::sort(results.begin(), results.end());
	for (int i = 0; i < itemsCount; ++i)
	{
		EXPECT_EQ(i * 10, results[i]);
	}
}
