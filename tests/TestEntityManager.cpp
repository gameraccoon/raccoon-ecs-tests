#include <gtest/gtest.h>

#include <functional>
#include <vector>

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

	EXPECT_FALSE(entityManager.hasAnyEntities());

	const Entity testEntity1 = entityManager.addEntity();

	EXPECT_TRUE(entityManager.hasAnyEntities());
	EXPECT_TRUE(entityManager.hasEntity(testEntity1));

	const Entity testEntity2 = entityManager.addEntity();

	EXPECT_TRUE(entityManager.hasAnyEntities());
	EXPECT_TRUE(entityManager.hasEntity(testEntity1));
	EXPECT_TRUE(entityManager.hasEntity(testEntity2));
	EXPECT_NE(testEntity1, testEntity2);
	EXPECT_NE(testEntity1.getId(), testEntity2.getId());

	entityManager.removeEntity(testEntity2);

	EXPECT_TRUE(entityManager.hasAnyEntities());
	EXPECT_TRUE(entityManager.hasEntity(testEntity1));
	EXPECT_FALSE(entityManager.hasEntity(testEntity2));

	Entity testEntity3 = entityManager.addEntity();

	EXPECT_TRUE(entityManager.hasAnyEntities());
	EXPECT_TRUE(entityManager.hasEntity(testEntity1));
	EXPECT_FALSE(entityManager.hasEntity(testEntity2));
	EXPECT_TRUE(entityManager.hasEntity(testEntity3));
	EXPECT_NE(testEntity1, testEntity3);
	EXPECT_NE(testEntity1.getId(), testEntity3.getId());
}

TEST(EntityManager, ComponentsCanBeAddedToEntities)
{
	using namespace ComponentTests;

	auto entityManagerData = prepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const TestVector2 location{1, 0};

	const Entity testEntity = entityManager.addEntity();

	{
		TransformComponent* transform = entityManager.addComponent<TransformComponent>(testEntity);
		ASSERT_NE(nullptr, transform);
		transform->pos = location;
	}

	{
		const auto [resultTransform] = entityManager.getEntityComponents<TransformComponent>(testEntity);
		EXPECT_TRUE(location == resultTransform->pos);
	}
}

TEST(EntityManager, EntitesWithComponentsCanBeRemoved)
{
	using namespace ComponentTests;

	auto entityManagerData = prepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const auto checkOnlyLocationsExist = [&entityManager](std::vector<TestVector2> locations)
	{
		std::vector<std::tuple<TransformComponent*>> components;
		entityManager.getComponents<TransformComponent>(components);
		EXPECT_EQ(locations.size(), components.size());

		for (auto& [transform] : components)
		{
			const size_t oldSize = locations.size();
			locations.erase(
				std::remove(
					locations.begin(),
					locations.end(),
					transform->pos),
				locations.end()
			);
			// each iteration should remove exactly one element
			EXPECT_EQ(oldSize, locations.size() + 1);
		}
		EXPECT_TRUE(locations.empty());
	};

	const TestVector2 location1{1, 0};
	const TestVector2 location2{0, 1};
	const TestVector2 location3{1, 1};

	const Entity testEntity1 = entityManager.addEntity();
	TransformComponent* transform1 = entityManager.addComponent<TransformComponent>(testEntity1);
	transform1->pos = location1;

	const Entity testEntity2 = entityManager.addEntity();
	TransformComponent* transform2 = entityManager.addComponent<TransformComponent>(testEntity2);
	transform2->pos = location2;

	checkOnlyLocationsExist({ location1, location2 });

	entityManager.removeEntity(testEntity2);

	const Entity testEntity3 = entityManager.addEntity();
	TransformComponent* transform3 = entityManager.addComponent<TransformComponent>(testEntity3);
	transform3->pos = location3;

	checkOnlyLocationsExist({ location1, location3 });

	entityManager.removeEntity(testEntity3);

	checkOnlyLocationsExist({ location1 });

	entityManager.removeEntity(testEntity1);

	checkOnlyLocationsExist({});
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

		const Entity testEntity1 = entityManager.addEntity();
		const Entity testEntity2 = entityManager.addEntity();
		const Entity testEntity3 = entityManager.addEntity();

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
