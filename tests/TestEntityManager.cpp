#include <gtest/gtest.h>

#include <functional>

#include "raccoon-ecs/entity_manager.h"

namespace ComponentTests
{
	enum ComponentType { TransformComponentId, MovementComponentId, LifetimeCheckerComponentId };
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<ComponentType>;
	using EntityGenerator = RaccoonEcs::EntityGenerator;
	using EntityManager = RaccoonEcs::EntityManagerImpl<ComponentType>;
	using Entity = RaccoonEcs::Entity;

	struct TestVector2
	{
		int x;
		int y;

		bool operator==(TestVector2 other) { return x == other.x && y == other.y; }
	};

	struct TransformComponent
	{
		TestVector2 pos;

		static ComponentType GetTypeId() { return TransformComponentId; };
	};

	struct MovementComponent
	{
		TestVector2 move;

		static ComponentType GetTypeId() { return MovementComponentId; };
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

	struct EntityManagerData
	{
		ComponentFactory componentFactory;
		EntityGenerator entityGenerator{42};
		EntityManager entityManager{componentFactory, entityGenerator};
	};

	static std::unique_ptr<EntityManagerData> prepareEntityManager()
	{
		auto data = std::make_unique<EntityManagerData>();
		data->componentFactory.registerComponent<TransformComponent>();
		data->componentFactory.registerComponent<MovementComponent>();
		data->componentFactory.registerComponent<LifetimeCheckerComponent>();
		return data;
	}
}

TEST(EntityManager, EntitiesCanBeCreatedAndRemoved)
{
	using namespace ComponentTests;

	auto entityManagerData = prepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	Entity testEntity1 = entityManager.addEntity();
	Entity testEntity2 = entityManager.addEntity();

	EXPECT_NE(testEntity1, testEntity2);
	EXPECT_NE(testEntity1.getId(), testEntity2.getId());

	entityManager.removeEntity(testEntity2);

	Entity testEntity3 = entityManager.addEntity();

	EXPECT_NE(testEntity1, testEntity3);
	EXPECT_NE(testEntity1.getId(), testEntity3.getId());
}

TEST(EntityManager, ComponentsCanBeAddedToEntities)
{
	using namespace ComponentTests;

	auto entityManagerData = prepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	TestVector2 location{1, 0};

	Entity testEntity = entityManager.addEntity();

	{
		TransformComponent* transform = entityManager.addComponent<TransformComponent>(testEntity);
		ASSERT_NE(nullptr, transform);
		transform->pos = location;
	}

	{
		auto [resultTransform] = entityManager.getEntityComponents<TransformComponent>(testEntity);
		EXPECT_TRUE(location == resultTransform->pos);
	}
}

TEST(EntityManager, EntitesWithComponentsCanBeRemoved)
{
	using namespace ComponentTests;

	auto entityManagerData = prepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	TestVector2 location1{1, 0};
	TestVector2 location2{0, 1};
	TestVector2 location3{1, 1};

	Entity testEntity1 = entityManager.addEntity();
	TransformComponent* transform1 = entityManager.addComponent<TransformComponent>(testEntity1);
	transform1->pos = location1;

	Entity testEntity2 = entityManager.addEntity();
	TransformComponent* transform2 = entityManager.addComponent<TransformComponent>(testEntity2);
	transform2->pos = location2;

	std::vector<std::tuple<TransformComponent*>> components;
	entityManager.getComponents<TransformComponent>(components);
	EXPECT_EQ(static_cast<size_t>(2u), components.size());

	bool location1Found = false;
	bool location2Found = false;
	for (auto& [transform] : components)
	{
		TestVector2 location = transform->pos;
		if (location == location1 && location1Found == false)
		{
			location1Found = true;
		}
		else if (location == location2 && location2Found == false)
		{
			location2Found = true;
		}
		else
		{
			GTEST_FAIL();
		}
	}
	EXPECT_TRUE(location1Found);
	EXPECT_TRUE(location2Found);

	entityManager.removeEntity(testEntity2);

	Entity testEntity3 = entityManager.addEntity();
	TransformComponent* transform3 = entityManager.addComponent<TransformComponent>(testEntity3);
	transform3->pos = location3;

	location1Found = false;
	location2Found = false;
	bool location3Found = false;
	std::vector<std::tuple<TransformComponent*>> transforms;
	entityManager.getComponents<TransformComponent>(transforms);
	EXPECT_EQ(static_cast<size_t>(2u), transforms.size());
	for (auto& [transform] : transforms)
	{
		TestVector2 location = transform->pos;
		if (location == location1 && !location1Found)
		{
			location1Found = true;
		}
		else if (location == location2 && !location2Found)
		{
			location2Found = true;
		}
		else if (location == location3 && !location3Found)
		{
			location3Found = true;
		}
		else
		{
			GTEST_FAIL();
		}
	}
	EXPECT_TRUE(location1Found);
	EXPECT_FALSE(location2Found);
	EXPECT_TRUE(location3Found);

	entityManager.clearCaches();
	transforms.clear();
	entityManager.getComponents<TransformComponent>(transforms);
	EXPECT_EQ(static_cast<size_t>(2u), transforms.size());

	entityManager.removeEntity(testEntity3);
	entityManager.clearCaches();
	transforms.clear();
	entityManager.getComponents<TransformComponent>(transforms);
	EXPECT_EQ(static_cast<size_t>(1u), transforms.size());
}

TEST(EntityManager, ComponentsNeverCopiedOrMovedAndAlwaysDestroyed)
{
	using namespace ComponentTests;

	std::array<bool, 3> destroyedObjects{false, false, false};
	int copiesCount = 0;

	const auto destructionFn = [&destroyedObjects](int objectIndex) { destroyedObjects[objectIndex] = true; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };

	{
		auto entityManagerData = prepareEntityManager();
		EntityManager& entityManager = entityManagerData->entityManager;

		Entity testEntity1 = entityManager.addEntity();
		Entity testEntity2 = entityManager.addEntity();
		Entity testEntity3 = entityManager.addEntity();

		{
			LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity1);
			lifetimeChecker->destructionCallback = [&destructionFn](){ destructionFn(0); };
			lifetimeChecker->copyCallback = copyFn;
		}

		{
			LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity2);
			lifetimeChecker->destructionCallback = [&destructionFn](){ destructionFn(1); };
			lifetimeChecker->copyCallback = copyFn;
		}

		{
			LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity3);
			lifetimeChecker->destructionCallback = [&destructionFn](){ destructionFn(2); };
			lifetimeChecker->copyCallback = copyFn;
		}

		EXPECT_EQ(destroyedObjects[0], false);
		entityManager.removeComponent<LifetimeCheckerComponent>(testEntity1);
		EXPECT_EQ(destroyedObjects[0], true);

		EXPECT_EQ(destroyedObjects[1], false);
		EXPECT_EQ(destroyedObjects[2], false);
	}

	EXPECT_EQ(destroyedObjects[1], true);
	EXPECT_EQ(destroyedObjects[2], true);

	EXPECT_EQ(copiesCount, 0);
}
