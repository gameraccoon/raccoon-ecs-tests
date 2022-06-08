#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "raccoon-ecs/component_set_holder.h"

namespace ComponentHolderTestInternal
{
	enum ComponentType { EmptyComponentId, DataComponentId, DataComponent2Id, LifetimeCheckerComponentId };
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<ComponentType>;
	using ComponentSetHolder = RaccoonEcs::ComponentSetHolderImpl<ComponentType>;
	using TypedComponent = RaccoonEcs::TypedComponentImpl<ComponentType>;

	struct TestVector2
	{
		TestVector2() = default;
		TestVector2(int x, int y)
			: x(x)
			, y(y)
		{}

		int x;
		int y;

		bool operator==(TestVector2 other) const { return x == other.x && y == other.y; }
	};

	struct EmptyComponent
	{
		static ComponentType GetTypeId() { return EmptyComponentId; };
	};

	struct ComponentWithData
	{
		TestVector2 pos;

		static ComponentType GetTypeId() { return DataComponentId; };
	};

	struct ComponentWithData2
	{
		TestVector2 pos;

		static ComponentType GetTypeId() { return DataComponent2Id; };
	};

	struct LifetimeCheckerComponent
	{
		std::function<void()> destructionCallback;
		std::function<void()> copyCallback;
		std::function<void()> moveCallback;

		LifetimeCheckerComponent() = default;
		LifetimeCheckerComponent(const LifetimeCheckerComponent& other)
			: destructionCallback(other.destructionCallback)
			, copyCallback(other.copyCallback)
			, moveCallback(other.moveCallback)
		{
			copyCallback();
		}

		LifetimeCheckerComponent operator=(const LifetimeCheckerComponent& other)
		{
			destructionCallback = other.destructionCallback;
			copyCallback = other.copyCallback;
			moveCallback = other.moveCallback;

			copyCallback();
			return *this;
		}

		LifetimeCheckerComponent(LifetimeCheckerComponent&& other)
			: destructionCallback(other.destructionCallback)
			, copyCallback(other.copyCallback)
			, moveCallback(other.moveCallback)
		{
			moveCallback();
		}

		LifetimeCheckerComponent operator=(LifetimeCheckerComponent&& other)
		{
			destructionCallback = other.destructionCallback;
			copyCallback = other.copyCallback;
			moveCallback = other.moveCallback;

			moveCallback();
			return *this;
		}

		~LifetimeCheckerComponent() { destructionCallback(); }

		static ComponentType GetTypeId() { return LifetimeCheckerComponentId; };
	};

	struct ComponentSetHolderData
	{
		ComponentFactory componentFactory;
		ComponentSetHolder componentSetHolder{componentFactory};
	};

	static void RegisterComponents(ComponentFactory& inOutFactory)
	{
		inOutFactory.registerComponent<EmptyComponent>();
		inOutFactory.registerComponent<ComponentWithData>();
		inOutFactory.registerComponent<ComponentWithData2>();
		inOutFactory.registerComponent<LifetimeCheckerComponent>();
	}

	static std::unique_ptr<ComponentSetHolderData> PrepareComponentSetHolder()
	{
		auto data = std::make_unique<ComponentSetHolderData>();
		RegisterComponents(data->componentFactory);
		return data;
	}
}

TEST(CompoentSetHolder, ComponentsCanBeCreatedAndRemoved)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();
	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;

	EXPECT_FALSE(componentSetHolder.hasAnyComponents());

	const TestVector2 location{1, 0};

	{
		ComponentWithData* transform = componentSetHolder.addComponent<ComponentWithData>();
		ASSERT_NE(nullptr, transform);
		transform->pos = location;
	}

	EXPECT_TRUE(componentSetHolder.hasAnyComponents());

	{
		const auto [resultTransform] = componentSetHolder.getComponents<ComponentWithData>();
		EXPECT_NE(nullptr, resultTransform);
		if (resultTransform)
		{
			EXPECT_TRUE(location == resultTransform->pos);
		}
	}

	componentSetHolder.removeComponent(ComponentWithData::GetTypeId());

	EXPECT_FALSE(componentSetHolder.hasAnyComponents());

	{
		const auto [resultTransform] = componentSetHolder.getComponents<ComponentWithData>();
		EXPECT_EQ(nullptr, resultTransform);
	}
}

TEST(CompoentSetHolder, ComponentsNeverCopiedOrMovedAndAlwaysDestroyed)
{
	using namespace ComponentHolderTestInternal;

	std::array<bool, 2> destroyedObjects{false, false};
	int copiesCount = 0;
	int movesCount = 0;

	const auto destructionFn = [&destroyedObjects](int objectIndex) { destroyedObjects[objectIndex] = true; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };
	const auto moveFn = [&movesCount]() { ++movesCount; };

	{
		auto componentSetHolderData = PrepareComponentSetHolder();
		ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;

		{
			LifetimeCheckerComponent* lifetimeChecker = componentSetHolder.addComponent<LifetimeCheckerComponent>();
			lifetimeChecker->destructionCallback = [&destructionFn](){ destructionFn(0); };
			lifetimeChecker->copyCallback = copyFn;
			lifetimeChecker->moveCallback = moveFn;
		}

		EXPECT_EQ(false, destroyedObjects[0]);

		{
			componentSetHolder.getOrAddComponent<LifetimeCheckerComponent>();
		}

		EXPECT_EQ(false, destroyedObjects[0]);
		componentSetHolder.removeComponent(LifetimeCheckerComponent::GetTypeId());
		EXPECT_EQ(true, destroyedObjects[0]);

		{
			LifetimeCheckerComponent* lifetimeChecker = componentSetHolder.addComponent<LifetimeCheckerComponent>();
			lifetimeChecker->destructionCallback = [&destructionFn](){ destructionFn(1); };
			lifetimeChecker->copyCallback = copyFn;
			lifetimeChecker->moveCallback = moveFn;
		}

		EXPECT_EQ(false, destroyedObjects[1]);
	}

	EXPECT_EQ(true, destroyedObjects[1]);

	EXPECT_EQ(0, copiesCount);
}

TEST(CompoentSetHolder, AllComponentsCanBeCollected)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();
	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;

	componentSetHolder.addComponent<EmptyComponent>();
	componentSetHolder.addComponent<ComponentWithData>();

	std::vector<TypedComponent> components = componentSetHolder.getAllComponents();

	EXPECT_EQ(static_cast<size_t>(2u), components.size());
	if (components.size() >= 2)
	{
		EXPECT_NE(nullptr, components[0].component);
		EXPECT_NE(nullptr, components[1].component);
		if (components[0].typeId == EmptyComponent::GetTypeId())
		{
			EXPECT_EQ(ComponentWithData::GetTypeId(), components[1].typeId);
		}
		else
		{
			EXPECT_EQ(ComponentWithData::GetTypeId(), components[0].typeId);
			EXPECT_EQ(EmptyComponent::GetTypeId(), components[1].typeId);
		}
	}
}

TEST(CompoentSetHolder, CompoentSetHolderCanBeCloned)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();
	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;

	ComponentWithData* dataComponent1 = componentSetHolder.addComponent<ComponentWithData>();
	dataComponent1->pos = TestVector2{10, 20};
	ComponentWithData2* dataComponent2 = componentSetHolder.addComponent<ComponentWithData2>();
	dataComponent2->pos = TestVector2{30, 40};

	ComponentSetHolder clonedComponentSetHolder(componentSetHolderData->componentFactory);
	clonedComponentSetHolder.overrideBy(componentSetHolder);

	{
		auto [data1, data2] = clonedComponentSetHolder.getComponents<ComponentWithData, ComponentWithData2>();
		ASSERT_NE(data1, nullptr);
		ASSERT_NE(data2, nullptr);
		EXPECT_EQ(data1->pos, TestVector2(10, 20));
		EXPECT_EQ(data2->pos, TestVector2(30, 40));
		EXPECT_NE(data1, dataComponent1);
		EXPECT_NE(data2, dataComponent2);
	}
}

TEST(CompoentSetHolder, CloningCompoentSetHolderCopiesComponentsOnlyOnce)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();
	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;
	int destructionsCount = 0;
	int copiesCount = 0;
	int movesCount = 0;

	const auto destructionFn = [&destructionsCount]() { ++destructionsCount; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };
	const auto moveFn = [&movesCount]() { ++movesCount; };

	{
		LifetimeCheckerComponent* lifetimeChecker = componentSetHolder.addComponent<LifetimeCheckerComponent>();
		lifetimeChecker->destructionCallback = destructionFn;
		lifetimeChecker->copyCallback = copyFn;
		lifetimeChecker->moveCallback = moveFn;
	}

	{
		ComponentSetHolder newComponentSetHolder(componentSetHolderData->componentFactory);
		newComponentSetHolder.overrideBy(componentSetHolder);
		EXPECT_EQ(destructionsCount, 0);
		EXPECT_EQ(copiesCount, 1);
		EXPECT_EQ(movesCount, 0);
	}

	EXPECT_EQ(destructionsCount, 1);
	EXPECT_EQ(copiesCount, 1);
	EXPECT_EQ(movesCount, 0);
}

TEST(CompoentSetHolder, CloningComponentSetHolderKeepsPreviousInstanceUntouched)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();

	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;
	{
		ComponentWithData* dataComponent1 = componentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{10, 20};
		ComponentWithData2* dataComponent2 = componentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{30, 40};
	}

	ComponentSetHolder newComponentSetHolder(componentSetHolderData->componentFactory);
	{
		ComponentWithData* dataComponent1 = newComponentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{50, 60};
		ComponentWithData2* dataComponent2 = newComponentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{70, 80};
	}

	newComponentSetHolder.overrideBy(componentSetHolder);

	{
		auto [data1, data2] = componentSetHolder.getComponents<ComponentWithData, ComponentWithData2>();
		ASSERT_NE(data1, nullptr);
		ASSERT_NE(data2, nullptr);
		EXPECT_EQ(data1->pos, TestVector2(10, 20));
		EXPECT_EQ(data2->pos, TestVector2(30, 40));
	}
}

TEST(CompoentSetHolder, CloningComponentSetHolderOverridesPreviousComponents)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();

	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;
	{
		ComponentWithData* dataComponent1 = componentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{10, 20};
		ComponentWithData2* dataComponent2 = componentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{30, 40};
	}

	ComponentSetHolder newComponentSetHolder(componentSetHolderData->componentFactory);
	{
		ComponentWithData* dataComponent1 = newComponentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{50, 60};
		ComponentWithData2* dataComponent2 = newComponentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{70, 80};
	}

	newComponentSetHolder.overrideBy(componentSetHolder);

	{
		auto [data1, data2] = newComponentSetHolder.getComponents<ComponentWithData, ComponentWithData2>();
		ASSERT_NE(data1, nullptr);
		ASSERT_NE(data2, nullptr);
		EXPECT_EQ(data1->pos, TestVector2(10, 20));
		EXPECT_EQ(data2->pos, TestVector2(30, 40));
	}
}

TEST(CompoentSetHolder, CompoentSetHolderCanBeMoveConstructed)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();
	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;

	ComponentWithData* dataComponent1 = componentSetHolder.addComponent<ComponentWithData>();
	dataComponent1->pos = TestVector2{10, 20};
	ComponentWithData2* dataComponent2 = componentSetHolder.addComponent<ComponentWithData2>();
	dataComponent2->pos = TestVector2{30, 40};

	ComponentSetHolder clonedComponentSetHolder(std::move(componentSetHolder));

	{
		auto [data1, data2] = clonedComponentSetHolder.getComponents<ComponentWithData, ComponentWithData2>();
		ASSERT_NE(data1, nullptr);
		ASSERT_NE(data2, nullptr);
		EXPECT_EQ(data1->pos, TestVector2(10, 20));
		EXPECT_EQ(data2->pos, TestVector2(30, 40));
		EXPECT_EQ(data1, dataComponent1);
		EXPECT_EQ(data2, dataComponent2);
	}
}

TEST(CompoentSetHolder, MoveConstructingCompoentSetHolderDoesNotMoveComponentsIndividually)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();
	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;
	int destructionsCount = 0;
	int copiesCount = 0;
	int movesCount = 0;

	const auto destructionFn = [&destructionsCount]() { ++destructionsCount; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };
	const auto moveFn = [&movesCount]() { ++movesCount; };

	{
		LifetimeCheckerComponent* lifetimeChecker = componentSetHolder.addComponent<LifetimeCheckerComponent>();
		lifetimeChecker->destructionCallback = destructionFn;
		lifetimeChecker->copyCallback = copyFn;
		lifetimeChecker->moveCallback = moveFn;
	}

	{
		ComponentSetHolder newComponentSetHolder(std::move(componentSetHolder));
		EXPECT_EQ(destructionsCount, 0);
		EXPECT_EQ(copiesCount, 0);
		EXPECT_EQ(movesCount, 0);
	}

	EXPECT_EQ(destructionsCount, 1);
	EXPECT_EQ(copiesCount, 0);
	EXPECT_EQ(movesCount, 0);
}

TEST(CompoentSetHolder, MoveConstructingComponentSetHolderClearsMovedFromInstance)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();

	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;
	{
		ComponentWithData* dataComponent1 = componentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{10, 20};
		ComponentWithData2* dataComponent2 = componentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{30, 40};
	}

	ComponentSetHolder newComponentSetHolder(std::move(componentSetHolder));

	{
		auto [data1, data2] = componentSetHolder.getComponents<ComponentWithData, ComponentWithData2>();
		EXPECT_EQ(data1, nullptr);
		EXPECT_EQ(data2, nullptr);
	}
}

TEST(CompoentSetHolder, CompoentSetHolderCanBeMoveAssigned)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();
	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;

	ComponentWithData* dataComponent1 = componentSetHolder.addComponent<ComponentWithData>();
	dataComponent1->pos = TestVector2{10, 20};
	ComponentWithData2* dataComponent2 = componentSetHolder.addComponent<ComponentWithData2>();
	dataComponent2->pos = TestVector2{30, 40};

	ComponentSetHolder clonedComponentSetHolder(componentSetHolderData->componentFactory);
	clonedComponentSetHolder = std::move(componentSetHolder);

	{
		auto [data1, data2] = clonedComponentSetHolder.getComponents<ComponentWithData, ComponentWithData2>();
		ASSERT_NE(data1, nullptr);
		ASSERT_NE(data2, nullptr);
		EXPECT_EQ(data1->pos, TestVector2(10, 20));
		EXPECT_EQ(data2->pos, TestVector2(30, 40));
		EXPECT_EQ(data1, dataComponent1);
		EXPECT_EQ(data2, dataComponent2);
	}
}

TEST(CompoentSetHolder, MoveAssigningCompoentSetHolderDoesNotMoveComponentsIndividually)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();
	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;
	int destructionsCount = 0;
	int copiesCount = 0;
	int movesCount = 0;

	const auto destructionFn = [&destructionsCount]() { ++destructionsCount; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };
	const auto moveFn = [&movesCount]() { ++movesCount; };

	{
		LifetimeCheckerComponent* lifetimeChecker = componentSetHolder.addComponent<LifetimeCheckerComponent>();
		lifetimeChecker->destructionCallback = destructionFn;
		lifetimeChecker->copyCallback = copyFn;
		lifetimeChecker->moveCallback = moveFn;
	}

	{
		ComponentSetHolder newComponentSetHolder = std::move(componentSetHolder);
		EXPECT_EQ(destructionsCount, 0);
		EXPECT_EQ(copiesCount, 0);
		EXPECT_EQ(movesCount, 0);
	}

	EXPECT_EQ(destructionsCount, 1);
	EXPECT_EQ(copiesCount, 0);
	EXPECT_EQ(movesCount, 0);
}

TEST(CompoentSetHolder, MoveAssigningComponentSetHolderClearsMovedFromInstance)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();

	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;
	{
		ComponentWithData* dataComponent1 = componentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{10, 20};
		ComponentWithData2* dataComponent2 = componentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{30, 40};
	}

	ComponentSetHolder newComponentSetHolder(componentSetHolderData->componentFactory);
	{
		ComponentWithData* dataComponent1 = newComponentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{50, 60};
		ComponentWithData2* dataComponent2 = newComponentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{70, 80};
	}

	newComponentSetHolder = std::move(componentSetHolder);

	{
		auto [data1, data2] = componentSetHolder.getComponents<ComponentWithData, ComponentWithData2>();
		ASSERT_EQ(data1, nullptr);
		ASSERT_EQ(data2, nullptr);
	}
}

TEST(CompoentSetHolder, MoveAssigningComponentSetHolderOverridesPreviousComponents)
{
	using namespace ComponentHolderTestInternal;

	auto componentSetHolderData = PrepareComponentSetHolder();

	ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;
	{
		ComponentWithData* dataComponent1 = componentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{10, 20};
		ComponentWithData2* dataComponent2 = componentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{30, 40};
	}

	ComponentSetHolder newComponentSetHolder(componentSetHolderData->componentFactory);
	{
		ComponentWithData* dataComponent1 = newComponentSetHolder.addComponent<ComponentWithData>();
		dataComponent1->pos = TestVector2{50, 60};
		ComponentWithData2* dataComponent2 = newComponentSetHolder.addComponent<ComponentWithData2>();
		dataComponent2->pos = TestVector2{70, 80};
	}

	newComponentSetHolder = std::move(componentSetHolder);

	{
		auto [data1, data2] = newComponentSetHolder.getComponents<ComponentWithData, ComponentWithData2>();
		ASSERT_NE(data1, nullptr);
		ASSERT_NE(data2, nullptr);
		EXPECT_EQ(data1->pos, TestVector2(10, 20));
		EXPECT_EQ(data2->pos, TestVector2(30, 40));
	}
}
