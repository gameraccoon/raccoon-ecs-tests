#include <gtest/gtest.h>

#include <thread>

#include "raccoon-ecs/async_stack.h"

TEST(AsyncStack, EmptyStackPop)
{
	RaccoonEcs::AsyncStack<int> stack;
	int a;
	EXPECT_FALSE(stack.pop_front(a));
}

TEST(AsyncStack, PushAndPop)
{
	RaccoonEcs::AsyncStack<int> stack;

	stack.push_front(10);

	int a;
	EXPECT_TRUE(stack.pop_front(a));
	EXPECT_EQ(10, a);
}

TEST(AsyncStack, PushMultiplePopOneAndDestroy)
{
	RaccoonEcs::AsyncStack<int> stack;

	stack.push_front(10);
	stack.push_front(20);
	stack.push_front(30);
	stack.push_front(40);

	int a;
	EXPECT_TRUE(stack.pop_front(a));
	EXPECT_EQ(40, a);
}

TEST(AsyncStack, PushMultiplePopMultiple)
{
	RaccoonEcs::AsyncStack<int> stack;

	stack.push_front(10);
	stack.push_front(20);
	stack.push_front(30);
	stack.push_front(40);

	int a;
	EXPECT_TRUE(stack.pop_front(a));
	EXPECT_EQ(40, a);
	EXPECT_TRUE(stack.pop_front(a));
	EXPECT_EQ(30, a);
	EXPECT_TRUE(stack.pop_front(a));
	EXPECT_EQ(20, a);
	EXPECT_TRUE(stack.pop_front(a));
	EXPECT_EQ(10, a);
	EXPECT_FALSE(stack.pop_front(a));
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
			if (stack.pop_front(a))
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
			stack.push_front(i * 10);
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
