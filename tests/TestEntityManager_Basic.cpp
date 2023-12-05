#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "raccoon-ecs/entity_manager.h"

namespace TestEntityManager_Basic_Internal
{
	enum ComponentType
	{
		EmptyComponentId,
		TransformComponentId,
		MovementComponentId,
		LifetimeCheckerComponentId,
		NotUsedComponentId,
	};

	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<ComponentType>;
	using EntityGenerator = RaccoonEcs::IncrementalEntityGenerator;
	using EntityManager = RaccoonEcs::EntityManagerImpl<ComponentType>;
	using Entity = RaccoonEcs::Entity;
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
		LifetimeCheckerComponent(const LifetimeCheckerComponent& other)
			: destructionCallback(other.destructionCallback)
			, copyCallback(other.copyCallback)
			, moveCallback(other.moveCallback)
		{
			copyCallback();
		}

		LifetimeCheckerComponent& operator=(const LifetimeCheckerComponent& other)
		{
			destructionCallback = other.destructionCallback;
			copyCallback = other.copyCallback;
			moveCallback = other.moveCallback;

			copyCallback();
			return *this;
		}

		LifetimeCheckerComponent(LifetimeCheckerComponent&& other) noexcept
			: destructionCallback(other.destructionCallback)
			, copyCallback(other.copyCallback)
			, moveCallback(other.moveCallback)
		{
			moveCallback();
		}

		LifetimeCheckerComponent& operator=(LifetimeCheckerComponent&& other) noexcept
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

	struct NotUsedComponent
	{
		static ComponentType GetTypeId() { return NotUsedComponentId; };
	};

	struct EntityManagerData
	{
		ComponentFactory componentFactory;
		EntityGenerator entityGenerator;
		EntityManager entityManager{componentFactory, entityGenerator};
	};

	static void RegisterComponents(ComponentFactory& inOutFactory)
	{
		inOutFactory.registerComponent<EmptyComponent>();
		inOutFactory.registerComponent<TransformComponent>();
		inOutFactory.registerComponent<MovementComponent>();
		inOutFactory.registerComponent<LifetimeCheckerComponent>();
	}

	static std::unique_ptr<EntityManagerData> PrepareEntityManager()
	{
		auto data = std::make_unique<EntityManagerData>();
		RegisterComponents(data->componentFactory);
		return data;
	}
} // namespace EntityManagerTestInternals

TEST(EntityManager, EntitiesCanBeCreatedAndRemoved)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
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
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
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
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const auto checkOnlyLocationsExist = [&entityManager](std::vector<TestVector2> locations) {
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
					transform->pos
				),
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

	checkOnlyLocationsExist({location1, location2});

	entityManager.removeEntity(testEntity2);

	const Entity testEntity3 = entityManager.addEntity();
	TransformComponent* transform3 = entityManager.addComponent<TransformComponent>(testEntity3);
	transform3->pos = location3;

	checkOnlyLocationsExist({location1, location3});

	entityManager.removeEntity(testEntity3);

	checkOnlyLocationsExist({location1});

	entityManager.removeEntity(testEntity1);

	checkOnlyLocationsExist({});
}

TEST(EntityManager, ComponentsNeverCopiedOrMovedAndAlwaysDestroyed)
{
	using namespace TestEntityManager_Basic_Internal;

	std::array<bool, 3> destroyedObjects{false, false, false};
	int copiesCount = 0;
	int movesCount = 0;

	const auto destructionFn = [&destroyedObjects](int objectIndex) { destroyedObjects[objectIndex] = true; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };
	const auto moveFn = [&movesCount]() { ++movesCount; };

	{
		auto entityManagerData = PrepareEntityManager();
		EntityManager& entityManager = entityManagerData->entityManager;

		const Entity testEntity1 = entityManager.addEntity();
		const Entity testEntity2 = entityManager.addEntity();
		const Entity testEntity3 = entityManager.addEntity();

		{
			LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity1);
			lifetimeChecker->destructionCallback = [&destructionFn]() { destructionFn(0); };
			lifetimeChecker->copyCallback = copyFn;
			lifetimeChecker->moveCallback = moveFn;
		}

		{
			LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity2);
			lifetimeChecker->destructionCallback = [&destructionFn]() { destructionFn(1); };
			lifetimeChecker->copyCallback = copyFn;
			lifetimeChecker->moveCallback = moveFn;
		}

		{
			LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity3);
			lifetimeChecker->destructionCallback = [&destructionFn]() { destructionFn(2); };
			lifetimeChecker->copyCallback = copyFn;
			lifetimeChecker->moveCallback = moveFn;
		}

		EXPECT_EQ(false, destroyedObjects[0]);
		entityManager.removeComponent<LifetimeCheckerComponent>(testEntity1);
		EXPECT_EQ(true, destroyedObjects[0]);

		EXPECT_EQ(false, destroyedObjects[1]);
		EXPECT_EQ(false, destroyedObjects[2]);
	}

	EXPECT_EQ(true, destroyedObjects[1]);
	EXPECT_EQ(true, destroyedObjects[2]);

	EXPECT_EQ(0, copiesCount);
	EXPECT_EQ(0, movesCount);
}

TEST(EntityManager, EntitiesCanBeMatchedByHavingComponents)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity1);
	entityManager.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity2);
	entityManager.addComponent<EmptyComponent>(testEntity2);

	// check that components assigned correctly (way 1)
	EXPECT_TRUE(entityManager.doesEntityHaveComponent<TransformComponent>(testEntity1));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent<MovementComponent>(testEntity1));
	EXPECT_FALSE(entityManager.doesEntityHaveComponent<EmptyComponent>(testEntity1));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent<TransformComponent>(testEntity2));
	EXPECT_FALSE(entityManager.doesEntityHaveComponent<MovementComponent>(testEntity2));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent<EmptyComponent>(testEntity2));
	// check that components assigned correctly (way 2)
	EXPECT_TRUE(entityManager.doesEntityHaveComponent(testEntity1, TransformComponentId));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent(testEntity1, MovementComponentId));
	EXPECT_FALSE(entityManager.doesEntityHaveComponent(testEntity1, EmptyComponentId));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent(testEntity2, TransformComponentId));
	EXPECT_FALSE(entityManager.doesEntityHaveComponent(testEntity2, MovementComponentId));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent(testEntity2, EmptyComponentId));

	// use getEntitiesHavingComponents with one component matching one entity
	{
		std::vector<Entity> matchedEntities;
		entityManager.getEntitiesHavingComponents({MovementComponentId}, matchedEntities);
		EXPECT_EQ(static_cast<size_t>(1), matchedEntities.size());
		if (!matchedEntities.empty())
		{
			EXPECT_EQ(testEntity1, matchedEntities[0]);
		}
	}

	// use getEntitiesHavingComponents with one component matching two entities
	{
		std::vector<Entity> matchedEntities;
		entityManager.getEntitiesHavingComponents({TransformComponentId}, matchedEntities);
		EXPECT_EQ(static_cast<size_t>(2), matchedEntities.size());
		if (matchedEntities.size() >= 2)
		{
			EXPECT_NE(testEntity1, testEntity2);
		}
	}

	// use getEntitiesHavingComponents with two components matching one entity
	{
		std::vector<Entity> matchedEntities;
		entityManager.getEntitiesHavingComponents({EmptyComponentId, TransformComponentId}, matchedEntities);
		EXPECT_EQ(static_cast<size_t>(1), matchedEntities.size());
		if (!matchedEntities.empty())
		{
			EXPECT_EQ(testEntity2, matchedEntities[0]);
		}
	}
}

TEST(EntityManager, AllCopmonentsFromOneEntityCanBeCollected)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity);
	entityManager.addComponent<MovementComponent>(testEntity);

	std::vector<TypedComponent> componentsWithTypes;
	entityManager.getAllEntityComponents(testEntity, componentsWithTypes);
	EXPECT_EQ(static_cast<size_t>(2), componentsWithTypes.size());
	if (componentsWithTypes.size() >= 2)
	{
		if (componentsWithTypes[0].typeId == TransformComponentId)
		{
			EXPECT_EQ(MovementComponentId, componentsWithTypes[1].typeId);
		}
		else
		{
			EXPECT_EQ(MovementComponentId, componentsWithTypes[0].typeId);
			EXPECT_EQ(TransformComponentId, componentsWithTypes[1].typeId);
		}
	}
}

TEST(EntityManager, ComponentAdditionOrRemovementCanBeScheduled)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity);

	entityManager.forEachComponentSetWithEntity<TransformComponent>([&entityManager](Entity entity, TransformComponent*) {
		entityManager.scheduleRemoveComponent<TransformComponent>(entity);
		MovementComponent* movement = entityManager.scheduleAddComponent<MovementComponent>(entity);
		movement->move = TestVector2(2, 3);
	});

	entityManager.executeScheduledActions();

	EXPECT_FALSE(entityManager.doesEntityHaveComponent<TransformComponent>(testEntity));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent<MovementComponent>(testEntity));
	auto [movement] = entityManager.getEntityComponents<MovementComponent>(testEntity);
	EXPECT_NE(nullptr, movement);
	if (movement != nullptr)
	{
		EXPECT_EQ(TestVector2(2, 3), movement->move);
	}
}

TEST(EntityManager, EntitiesCanBeTransferedBetweenEntityManagers)
{
	using namespace TestEntityManager_Basic_Internal;

	EntityGenerator entityGenerator;
	ComponentFactory componentFactory;
	RegisterComponents(componentFactory);
	EntityManager entityManager1(componentFactory, entityGenerator);

	const Entity testEntity = entityManager1.addEntity();
	{
		TransformComponent* transform = entityManager1.addComponent<TransformComponent>(testEntity);
		transform->pos = TestVector2(10, 3);
		entityManager1.addComponent<MovementComponent>(testEntity);
	}

	EntityManager entityManager2(componentFactory, entityGenerator);
	entityManager1.transferEntityTo(entityManager2, testEntity);

	EXPECT_FALSE(entityManager1.hasEntity(testEntity));
	EXPECT_TRUE(entityManager2.hasEntity(testEntity));
	EXPECT_TRUE(entityManager2.doesEntityHaveComponent<TransformComponent>(testEntity));
	EXPECT_TRUE(entityManager2.doesEntityHaveComponent<MovementComponent>(testEntity));
	auto [transform] = entityManager2.getEntityComponents<TransformComponent>(testEntity);
	if (transform != nullptr)
	{
		EXPECT_EQ(TestVector2(10, 3), transform->pos);
	}
}

TEST(EntityManager, EntityCanBeAddedInTwoSteps)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	Entity testEntity = entityManager.generateNewEntityUnsafe();

	auto doRedoCommand = [testEntity, &entityManager]() {
		entityManager.addExistingEntityUnsafe(testEntity);
		entityManager.addComponent<TransformComponent>(testEntity);
	};

	auto undoCommand = [testEntity, &entityManager]() {
		entityManager.removeEntity(testEntity);
	};

	EXPECT_FALSE(entityManager.hasAnyEntities());
	EXPECT_FALSE(entityManager.hasEntity(testEntity));

	doRedoCommand();

	EXPECT_TRUE(entityManager.hasEntity(testEntity));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent<TransformComponent>(testEntity));

	undoCommand();

	EXPECT_FALSE(entityManager.hasEntity(testEntity));

	doRedoCommand();

	EXPECT_TRUE(entityManager.hasEntity(testEntity));
	EXPECT_TRUE(entityManager.doesEntityHaveComponent<TransformComponent>(testEntity));
}

TEST(EntityManager, MatchingEntityCountCanBeGathered)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity1);
	entityManager.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity2);
	entityManager.addComponent<EmptyComponent>(testEntity2);

	EXPECT_EQ(static_cast<size_t>(0), entityManager.getMatchingEntitiesCount<NotUsedComponent>());
	EXPECT_EQ(static_cast<size_t>(1), entityManager.getMatchingEntitiesCount<MovementComponent>());
	EXPECT_EQ(static_cast<size_t>(1), entityManager.getMatchingEntitiesCount<EmptyComponent>());
	EXPECT_EQ(static_cast<size_t>(2), entityManager.getMatchingEntitiesCount<TransformComponent>());
}

TEST(EntityManager, EntityManagerCanBeCloned)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	TransformComponent* transform1 = entityManager.addComponent<TransformComponent>(testEntity1);
	transform1->pos = TestVector2{10, 20};
	MovementComponent* movement1 = entityManager.addComponent<MovementComponent>(testEntity1);
	movement1->move = TestVector2{30, 40};

	const Entity testEntity2 = entityManager.addEntity();
	TransformComponent* transform2 = entityManager.addComponent<TransformComponent>(testEntity2);
	transform2->pos = TestVector2{50, 60};
	MovementComponent* movement2 = entityManager.addComponent<MovementComponent>(testEntity2);
	movement2->move = TestVector2{70, 80};

	EntityManager entityManagerCopy(entityManagerData->componentFactory, entityManagerData->entityGenerator);
	entityManagerCopy.overrideBy(entityManager);

	{
		ASSERT_TRUE(entityManagerCopy.hasEntity(testEntity1));
		auto [transform, movement] = entityManagerCopy.getEntityComponents<TransformComponent, MovementComponent>(testEntity1);
		EXPECT_EQ(transform->pos, TestVector2(10, 20));
		EXPECT_EQ(movement->move, TestVector2(30, 40));
		EXPECT_NE(transform1, transform);
		EXPECT_NE(movement1, movement);
	}
	{
		ASSERT_TRUE(entityManagerCopy.hasEntity(testEntity2));
		auto [transform, movement] = entityManagerCopy.getEntityComponents<TransformComponent, MovementComponent>(testEntity2);
		EXPECT_EQ(transform->pos, TestVector2(50, 60));
		EXPECT_EQ(movement->move, TestVector2(70, 80));
		EXPECT_NE(transform2, transform);
		EXPECT_NE(movement2, movement);
	}
}

TEST(EntityManager, CloningEntityManagerCopiesComponentsOnlyOnce)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;
	int destructionsCount = 0;
	int copiesCount = 0;
	int movesCount = 0;

	const auto destructionFn = [&destructionsCount]() { ++destructionsCount; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };
	const auto moveFn = [&movesCount]() { ++movesCount; };

	const Entity testEntity1 = entityManager.addEntity();
	{
		LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity1);
		lifetimeChecker->destructionCallback = destructionFn;
		lifetimeChecker->copyCallback = copyFn;
		lifetimeChecker->moveCallback = moveFn;
	}

	{
		EntityManager newEntityManager(entityManagerData->componentFactory, entityManagerData->entityGenerator);
		newEntityManager.overrideBy(entityManager);
		EXPECT_EQ(destructionsCount, 0);
		EXPECT_EQ(copiesCount, 1);
		EXPECT_EQ(movesCount, 0);
	}

	EXPECT_EQ(destructionsCount, 1);
	EXPECT_EQ(copiesCount, 1);
	EXPECT_EQ(movesCount, 0);
}

TEST(EntityManager, CloningEntityManagerKeepsOldInstanceUntouched)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* copiedFromMovement = entityManager.addComponent<MovementComponent>(testEntity1);
		copiedFromMovement->move.x = 100;
		copiedFromMovement->move.y = 200;
	}

	EntityManager newEntityManager(entityManagerData->componentFactory, entityManagerData->entityGenerator);

	const Entity testEntity2 = newEntityManager.addEntity();
	{
		MovementComponent* oldMovement = newEntityManager.addComponent<MovementComponent>(testEntity2);
		oldMovement->move.x = 40;
		oldMovement->move.y = 50;
	}

	newEntityManager.overrideBy(entityManager);

	ASSERT_TRUE(entityManager.hasEntity(testEntity1));
	EXPECT_FALSE(entityManager.hasEntity(testEntity2));

	auto [copiedFromMovement] = entityManager.getEntityComponents<MovementComponent>(testEntity1);
	EXPECT_EQ(copiedFromMovement->move.x, 100);
	EXPECT_EQ(copiedFromMovement->move.y, 200);
}

TEST(EntityManager, CloningEntityManagerOverridesPreviousEntities)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* copiedFromMovement = entityManager.addComponent<MovementComponent>(testEntity1);
		copiedFromMovement->move.x = 100;
		copiedFromMovement->move.y = 200;
	}

	EntityManager newEntityManager(entityManagerData->componentFactory, entityManagerData->entityGenerator);

	const Entity testEntity2 = newEntityManager.addEntity();
	{
		MovementComponent* oldMovement = newEntityManager.addComponent<MovementComponent>(testEntity2);
		oldMovement->move.x = 40;
		oldMovement->move.y = 50;
	}

	newEntityManager.overrideBy(entityManager);

	ASSERT_TRUE(newEntityManager.hasEntity(testEntity1));
	EXPECT_FALSE(newEntityManager.hasEntity(testEntity2));

	auto [newMovement] = newEntityManager.getEntityComponents<MovementComponent>(testEntity1);
	EXPECT_EQ(newMovement->move.x, 100);
	EXPECT_EQ(newMovement->move.y, 200);
}

TEST(EntityManager, CloningEntityManagerOverridesPreviousIndexes)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* copiedFromMovement = entityManager.addComponent<MovementComponent>(testEntity1);
		copiedFromMovement->move.x = 100;
		copiedFromMovement->move.y = 200;
	}

	EntityManager newEntityManager(entityManagerData->componentFactory, entityManagerData->entityGenerator);

	const Entity testEntity2 = newEntityManager.addEntity();
	{
		MovementComponent* oldMovement = newEntityManager.addComponent<MovementComponent>(testEntity2);
		oldMovement->move.x = 40;
		oldMovement->move.y = 50;
	}
	newEntityManager.initIndex<MovementComponent>();

	newEntityManager.overrideBy(entityManager);

	ASSERT_TRUE(newEntityManager.hasEntity(testEntity1));

	std::vector<std::tuple<const MovementComponent*>> resultComponents;
	newEntityManager.getComponents<const MovementComponent>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.x, 100);
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.y, 200);
}

TEST(EntityManager, EntityManagerCanBeMoveConstructed)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	TransformComponent* transform1 = entityManager.addComponent<TransformComponent>(testEntity1);
	transform1->pos = TestVector2{10, 20};
	MovementComponent* movement1 = entityManager.addComponent<MovementComponent>(testEntity1);
	movement1->move = TestVector2{30, 40};

	const Entity testEntity2 = entityManager.addEntity();
	TransformComponent* transform2 = entityManager.addComponent<TransformComponent>(testEntity2);
	transform2->pos = TestVector2{50, 60};
	MovementComponent* movement2 = entityManager.addComponent<MovementComponent>(testEntity2);
	movement2->move = TestVector2{70, 80};

	EntityManager newEntityManager(std::move(entityManager));

	{
		ASSERT_TRUE(newEntityManager.hasEntity(testEntity1));
		auto [transform, movement] = newEntityManager.getEntityComponents<TransformComponent, MovementComponent>(testEntity1);
		EXPECT_EQ(transform->pos, TestVector2(10, 20));
		EXPECT_EQ(movement->move, TestVector2(30, 40));
		EXPECT_EQ(transform1, transform);
		EXPECT_EQ(movement1, movement);
	}
	{
		ASSERT_TRUE(newEntityManager.hasEntity(testEntity2));
		auto [transform, movement] = newEntityManager.getEntityComponents<TransformComponent, MovementComponent>(testEntity2);
		EXPECT_EQ(transform->pos, TestVector2(50, 60));
		EXPECT_EQ(movement->move, TestVector2(70, 80));
		EXPECT_EQ(transform2, transform);
		EXPECT_EQ(movement2, movement);
	}
}

TEST(EntityManager, MoveConstructingEntityManagerDoesNotMoveComponentsIndividually)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;
	int destructionsCount = 0;
	int copiesCount = 0;
	int movesCount = 0;

	const auto destructionFn = [&destructionsCount]() { ++destructionsCount; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };
	const auto moveFn = [&movesCount]() { ++movesCount; };

	const Entity testEntity1 = entityManager.addEntity();
	{
		LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity1);
		lifetimeChecker->destructionCallback = destructionFn;
		lifetimeChecker->copyCallback = copyFn;
		lifetimeChecker->moveCallback = moveFn;
	}

	{
		EntityManager newEntityManager(std::move(entityManager));
		EXPECT_EQ(destructionsCount, 0);
		EXPECT_EQ(copiesCount, 0);
		EXPECT_EQ(movesCount, 0);
	}

	EXPECT_EQ(destructionsCount, 1);
	EXPECT_EQ(copiesCount, 0);
	EXPECT_EQ(movesCount, 0);
}

TEST(EntityManager, MoveConstructingEntityManagerClearsMovedFromEntity)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* copiedFromMovement = entityManager.addComponent<MovementComponent>(testEntity1);
		copiedFromMovement->move.x = 100;
		copiedFromMovement->move.y = 200;
	}

	EntityManager newEntityManager(std::move(entityManager));

	EXPECT_FALSE(entityManager.hasEntity(testEntity1));
	EXPECT_EQ(entityManager.getMatchingEntitiesCount<MovementComponent>(), static_cast<size_t>(0));
}

TEST(EntityManager, EntityManagerCanMoveAssigned)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	TransformComponent* transform1 = entityManager.addComponent<TransformComponent>(testEntity1);
	transform1->pos = TestVector2{10, 20};
	MovementComponent* movement1 = entityManager.addComponent<MovementComponent>(testEntity1);
	movement1->move = TestVector2{30, 40};

	const Entity testEntity2 = entityManager.addEntity();
	TransformComponent* transform2 = entityManager.addComponent<TransformComponent>(testEntity2);
	transform2->pos = TestVector2{50, 60};
	MovementComponent* movement2 = entityManager.addComponent<MovementComponent>(testEntity2);
	movement2->move = TestVector2{70, 80};

	EntityManager entityManagerCopy(entityManagerData->componentFactory, entityManagerData->entityGenerator);
	entityManagerCopy = std::move(entityManager);

	{
		ASSERT_TRUE(entityManagerCopy.hasEntity(testEntity1));
		auto [transform, movement] = entityManagerCopy.getEntityComponents<TransformComponent, MovementComponent>(testEntity1);
		EXPECT_EQ(transform->pos, TestVector2(10, 20));
		EXPECT_EQ(movement->move, TestVector2(30, 40));
		EXPECT_EQ(transform1, transform);
		EXPECT_EQ(movement1, movement);
	}
	{
		ASSERT_TRUE(entityManagerCopy.hasEntity(testEntity2));
		auto [transform, movement] = entityManagerCopy.getEntityComponents<TransformComponent, MovementComponent>(testEntity2);
		EXPECT_EQ(transform->pos, TestVector2(50, 60));
		EXPECT_EQ(movement->move, TestVector2(70, 80));
		EXPECT_EQ(transform2, transform);
		EXPECT_EQ(movement2, movement);
	}
}

TEST(EntityManager, MoveAssigningEntityManagerDoesNotMoveComponentsIndividually)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;
	int destructionsCount = 0;
	int copiesCount = 0;
	int movesCount = 0;

	const auto destructionFn = [&destructionsCount]() { ++destructionsCount; };
	const auto copyFn = [&copiesCount]() { ++copiesCount; };
	const auto moveFn = [&movesCount]() { ++movesCount; };

	const Entity testEntity1 = entityManager.addEntity();
	{
		LifetimeCheckerComponent* lifetimeChecker = entityManager.addComponent<LifetimeCheckerComponent>(testEntity1);
		lifetimeChecker->destructionCallback = destructionFn;
		lifetimeChecker->copyCallback = copyFn;
		lifetimeChecker->moveCallback = moveFn;
	}

	{
		EntityManager newEntityManager(entityManagerData->componentFactory, entityManagerData->entityGenerator);
		newEntityManager = std::move(entityManager);
		EXPECT_EQ(destructionsCount, 0);
		EXPECT_EQ(copiesCount, 0);
		EXPECT_EQ(movesCount, 0);
	}

	EXPECT_EQ(destructionsCount, 1);
	EXPECT_EQ(copiesCount, 0);
	EXPECT_EQ(movesCount, 0);
}

TEST(EntityManager, MoveAssigningEntityManagerClearsMovedFromEntity)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* copiedFromMovement = entityManager.addComponent<MovementComponent>(testEntity1);
		copiedFromMovement->move.x = 100;
		copiedFromMovement->move.y = 200;
	}

	EntityManager newEntityManager(entityManagerData->componentFactory, entityManagerData->entityGenerator);

	const Entity testEntity2 = newEntityManager.addEntity();
	{
		MovementComponent* oldMovement = newEntityManager.addComponent<MovementComponent>(testEntity2);
		oldMovement->move.x = 40;
		oldMovement->move.y = 50;
	}

	newEntityManager = std::move(entityManager);

	EXPECT_FALSE(entityManager.hasEntity(testEntity1));
	EXPECT_FALSE(entityManager.hasEntity(testEntity2));
	EXPECT_EQ(entityManager.getMatchingEntitiesCount<MovementComponent>(), static_cast<size_t>(0));
}

TEST(EntityManager, MoveAssigningEntityManagerOverridesPreviousEntities)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* copiedFromMovement = entityManager.addComponent<MovementComponent>(testEntity1);
		copiedFromMovement->move.x = 100;
		copiedFromMovement->move.y = 200;
	}

	EntityManager newEntityManager(entityManagerData->componentFactory, entityManagerData->entityGenerator);

	const Entity testEntity2 = newEntityManager.addEntity();
	{
		MovementComponent* oldMovement = newEntityManager.addComponent<MovementComponent>(testEntity2);
		oldMovement->move.x = 40;
		oldMovement->move.y = 50;
	}

	newEntityManager = std::move(entityManager);

	ASSERT_TRUE(newEntityManager.hasEntity(testEntity1));
	EXPECT_FALSE(newEntityManager.hasEntity(testEntity2));

	auto [newMovement] = newEntityManager.getEntityComponents<MovementComponent>(testEntity1);
	EXPECT_EQ(newMovement->move.x, 100);
	EXPECT_EQ(newMovement->move.y, 200);
}

TEST(EntityManager, MoveAssigningEntityManagerOverridesPreviousIndexes)
{
	using namespace TestEntityManager_Basic_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* copiedFromMovement = entityManager.addComponent<MovementComponent>(testEntity1);
		copiedFromMovement->move.x = 100;
		copiedFromMovement->move.y = 200;
	}

	EntityManager newEntityManager(entityManagerData->componentFactory, entityManagerData->entityGenerator);

	const Entity testEntity2 = newEntityManager.addEntity();
	{
		MovementComponent* oldMovement = newEntityManager.addComponent<MovementComponent>(testEntity2);
		oldMovement->move.x = 40;
		oldMovement->move.y = 50;
	}
	newEntityManager.initIndex<MovementComponent>();

	newEntityManager = std::move(entityManager);

	ASSERT_TRUE(newEntityManager.hasEntity(testEntity1));

	std::vector<std::tuple<const MovementComponent*>> resultComponents;
	newEntityManager.getComponents<const MovementComponent>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.x, 100);
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.y, 200);
}
