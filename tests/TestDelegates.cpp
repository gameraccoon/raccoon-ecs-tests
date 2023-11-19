#include <gtest/gtest.h>

#include "raccoon-ecs/delegates.h"


TEST(SinglecastDelegate, NotAssigned_CallSafe_ExpectNothingHappened)
{
	RaccoonEcs::SinglecastDelegate<int> delegate;

	delegate.callSafe(1);
}

TEST(SinglecastDelegate, Assigned_CallSafe_ExpectCalled)
{
	RaccoonEcs::SinglecastDelegate<int> delegate;

	int value = 0;
	delegate.assign([&value](int v) { value = v; });
	delegate.callSafe(1);

	EXPECT_EQ(value, 1);
}

TEST(SinglecastDelegate, Assigned_CallTwice_ExpectCalledTwice)
{
	RaccoonEcs::SinglecastDelegate<int> delegate;

	int value = 0;
	delegate.assign([&value](int v) { value += v; });
	delegate.callSafe(1);
	delegate.callSafe(2);

	EXPECT_EQ(value, 3);
}

TEST(SinglecastDelegate, Assigned_ReassignAndCall_OnlyLastCalled)
{
	RaccoonEcs::SinglecastDelegate<int> delegate;

	int value = 0;
	delegate.assign([&value](int v) { value = v; });
	delegate.assign([&value](int v) { value = v + 1; });
	delegate.callSafe(1);

	EXPECT_EQ(value, 2);
}

TEST(MulticastDelegate, NotAssigned_Broadcast_ExpectNothingHappened)
{
	RaccoonEcs::MulticastDelegate<int> delegate;

	delegate.broadcast(1);
}


TEST(MulticastDelegate, OneFunctionBound_Broadcast_ExpectCalled)
{
	RaccoonEcs::MulticastDelegate<int> delegate;

	int value = 0;
	delegate.bind([&value](int v) { value = v; });
	delegate.broadcast(1);

	EXPECT_EQ(value, 1);
}

TEST(MulticastDelegate, TwoFunctionsBound_Broadcast_ExpectAllCalled)
{
	RaccoonEcs::MulticastDelegate<int> delegate;

	int value = 0;
	delegate.bind([&value](int v) { value += v; });
	delegate.bind([&value](int v) { value += v * 2; });
	delegate.broadcast(3);

	EXPECT_EQ(value, 9);
}
