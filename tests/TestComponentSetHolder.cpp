#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "raccoon-ecs/component_set_holder.h"

namespace ComponentHolderTestInternal
{
	enum ComponentType { EmptyComponentId, DataComponentId, LifetimeCheckerComponentId };
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<ComponentType>;
	using ComponentSetHolder = RaccoonEcs::ComponentSetHolderImpl<ComponentType>;
	using TypedComponent = RaccoonEcs::TypedComponentImpl<ComponentType>;

	struct TestVector2
	{
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

	struct LifetimeCheckerComponent
	{
		std::function<void()> destructionCallback;
		std::function<void()> copyCallback;
		std::function<void()> moveCallback;

		LifetimeCheckerComponent() = default;
		LifetimeCheckerComponent(LifetimeCheckerComponent& other)
			: destructionCallback(other.destructionCallback)
			, copyCallback(other.copyCallback)
			, moveCallback(other.moveCallback)
		{
			copyCallback();
		}

		LifetimeCheckerComponent operator=(LifetimeCheckerComponent& other)
		{
			destructionCallback = other.destructionCallback;
			copyCallback = other.copyCallback;
			moveCallback = other.moveCallback;

			copyCallback();
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

	const auto destructionFn = [&destroyedObjects](int objectIndex) { destroyedObjects[objectIndex] = true; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };

	{
		auto componentSetHolderData = PrepareComponentSetHolder();
		ComponentSetHolder& componentSetHolder = componentSetHolderData->componentSetHolder;

		{
			LifetimeCheckerComponent* lifetimeChecker = componentSetHolder.addComponent<LifetimeCheckerComponent>();
			lifetimeChecker->destructionCallback = [&destructionFn](){ destructionFn(0); };
			lifetimeChecker->copyCallback = copyFn;
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
