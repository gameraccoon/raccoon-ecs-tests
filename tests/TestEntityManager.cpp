#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "raccoon-ecs/entity_manager.h"

namespace ComponentTests
{
	enum ComponentType { EmptyComponentId, TransformComponentId, MovementComponentId, LifetimeCheckerComponentId };
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<ComponentType>;
	using EntityGenerator = RaccoonEcs::EntityGenerator;
	using EntityManager = RaccoonEcs::EntityManagerImpl<ComponentType>;
	using Entity = RaccoonEcs::Entity;
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
}

TEST(EntityManager, EntitiesCanBeCreatedAndRemoved)
{
	using namespace ComponentTests;

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
	using namespace ComponentTests;

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
	using namespace ComponentTests;

	auto entityManagerData = PrepareEntityManager();
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
		auto entityManagerData = PrepareEntityManager();
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

TEST(EntityManager, ComponentSetsCanBeIteratedOver)
{
	using namespace ComponentTests;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity1);
	entityManager.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity2);
	entityManager.addComponent<EmptyComponent>(testEntity2);

	{
		int iterationsCount = 0;
		entityManager.forEachComponentSet<MovementComponent>(
			[&iterationsCount](MovementComponent*)
		{
			++iterationsCount;
		});
		EXPECT_EQ(1, iterationsCount);
	}

	{
		int iterationsCount = 0;
		auto transformPredicate =
		[&iterationsCount](TransformComponent*)
		{
			++iterationsCount;
		};
		entityManager.forEachComponentSet<TransformComponent>(transformPredicate);
		EXPECT_EQ(2, iterationsCount);

		// call the second time to check that cached data is valid
		entityManager.forEachComponentSet<TransformComponent>(transformPredicate);
		EXPECT_EQ(4, iterationsCount);
	}

	{
		int iterationsCount = 0;
		entityManager.forEachComponentSet<EmptyComponent, TransformComponent>(
			[&iterationsCount](EmptyComponent*, TransformComponent*)
		{
			++iterationsCount;
		});
		EXPECT_EQ(1, iterationsCount);
	}
}

TEST(EntityManager, ComponentSetsCanBeIteratedOverWithEntities)
{
	using namespace ComponentTests;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity1);
	entityManager.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity2);
	entityManager.addComponent<EmptyComponent>(testEntity2);

	{
		int iterationsCount = 0;
		entityManager.forEachComponentSetWithEntity<MovementComponent>(
			[&iterationsCount, testEntity1](Entity entity, MovementComponent*)
		{
			EXPECT_EQ(testEntity1, entity);
			++iterationsCount;
		});
		EXPECT_EQ(1, iterationsCount);
	}

	{
		int iterationsCount = 0;
		auto transformPredicate = [&iterationsCount](Entity, TransformComponent*)
		{
			++iterationsCount;
		};
		entityManager.forEachComponentSetWithEntity<TransformComponent>(transformPredicate);
		EXPECT_EQ(2, iterationsCount);

		// call the second time to check that cached data is valid
		entityManager.forEachComponentSetWithEntity<TransformComponent>(transformPredicate);
		EXPECT_EQ(4, iterationsCount);
	}

	{
		int iterationsCount = 0;
		entityManager.forEachComponentSetWithEntity<EmptyComponent, TransformComponent>(
			[&iterationsCount, testEntity2](Entity entity, EmptyComponent*, TransformComponent*)
		{
			EXPECT_EQ(testEntity2, entity);
			++iterationsCount;
		});
		EXPECT_EQ(1, iterationsCount);
	}
}

TEST(EntityManager, ComponentSetsCanBeCollected)
{
	using namespace ComponentTests;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity1);
	entityManager.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity2);
	entityManager.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<std::tuple<MovementComponent*>> components;
		entityManager.getComponents<MovementComponent>(components);
		EXPECT_EQ(static_cast<size_t>(1u), components.size());
	}

	{
		std::vector<std::tuple<TransformComponent*>> components;
		entityManager.getComponents<TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());

		// call the second time to check that cached data is valid
		entityManager.getComponents<TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(4u), components.size());
	}

	{
		std::vector<std::tuple<EmptyComponent*, TransformComponent*>> components;
		entityManager.getComponents<EmptyComponent, TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(1u), components.size());
	}
}

TEST(EntityManager, ComponentSetsWithEntitiesCanBeCollected)
{
	using namespace ComponentTests;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity1);
	entityManager.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity2);
	entityManager.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<std::tuple<Entity, MovementComponent*>> components;
		entityManager.getComponentsWithEntities<MovementComponent>(components);
		EXPECT_EQ(static_cast<size_t>(1u), components.size());
		if (!components.empty())
		{
			EXPECT_EQ(testEntity1, std::get<0>(components[0]));
		}
	}

	{
		std::vector<std::tuple<Entity, TransformComponent*>> components;
		entityManager.getComponentsWithEntities<TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());
		if (components.size() >= 2)
		{
			EXPECT_NE(std::get<0>(components[0]), std::get<0>(components[1]));
		}

		// call the second time to check that cached data is valid
		entityManager.getComponentsWithEntities<TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(4u), components.size());
	}

	{
		std::vector<std::tuple<Entity, EmptyComponent*, TransformComponent*>> components;
		entityManager.getComponentsWithEntities<EmptyComponent, TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(1u), components.size());
		if (!components.empty())
		{
			EXPECT_EQ(testEntity2, std::get<0>(components[0]));
		}
	}
}

TEST(EntityManager, EntitiesCanBeMatchedByHavingComponents)
{
	using namespace ComponentTests;

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
	using namespace ComponentTests;

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
	using namespace ComponentTests;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity = entityManager.addEntity();
	entityManager.addComponent<TransformComponent>(testEntity);

	entityManager.forEachComponentSetWithEntity<TransformComponent>([&entityManager](Entity entity,TransformComponent*)
	{
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

TEST(EntityManager, EntitiesCanBeTransferedBetweenCopmonents)
{
	using namespace ComponentTests;

	EntityGenerator entityGenerator(42);
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
	using namespace ComponentTests;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	Entity testEntity = entityManager.getNonExistentEntity();

	auto doRedoCommand = [testEntity, &entityManager]()
	{
		entityManager.tryInsertEntity(testEntity);
		entityManager.addComponent<TransformComponent>(testEntity);
	};

	auto undoCommand = [testEntity, &entityManager]()
	{
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

TEST(EntityManager, ComponentSetsCanBeIteratedOverWithAdditionalData)
{
	using namespace ComponentTests;

	EntityGenerator entityGenerator(42);
	ComponentFactory componentFactory;
	RegisterComponents(componentFactory);
	EntityManager entityManager1(componentFactory, entityGenerator);
	EntityManager entityManager2(componentFactory, entityGenerator);

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		int sum = 0;
		auto iterationFunction = [&sum](int data, EmptyComponent*, TransformComponent*)
		{
			sum += data;
		};
		entityManager1.forEachComponentSet<EmptyComponent, TransformComponent>(iterationFunction, 20);
		entityManager2.forEachComponentSet<EmptyComponent, TransformComponent>(iterationFunction, 50);
		EXPECT_EQ(70, sum);
	}
}

TEST(EntityManager, ComponentSetsCanBeIteratedOverWithEntitiesAndAdditionalData)
{
	using namespace ComponentTests;

	EntityGenerator entityGenerator(42);
	ComponentFactory componentFactory;
	RegisterComponents(componentFactory);
	EntityManager entityManager1(componentFactory, entityGenerator);
	EntityManager entityManager2(componentFactory, entityGenerator);

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		int sum = 0;
		auto iterationFunction = [&sum](int data, Entity, EmptyComponent*, TransformComponent*)
		{
			sum += data;
		};
		entityManager1.forEachComponentSetWithEntity<EmptyComponent, TransformComponent>(iterationFunction, 20);
		entityManager2.forEachComponentSetWithEntity<EmptyComponent, TransformComponent>(iterationFunction, 50);
		EXPECT_EQ(70, sum);
	}
}

TEST(EntityManager, ComponentSetsWithAdditionalDataCanBeCollected)
{
	using namespace ComponentTests;

	EntityGenerator entityGenerator(42);
	ComponentFactory componentFactory;
	RegisterComponents(componentFactory);
	EntityManager entityManager1(componentFactory, entityGenerator);
	EntityManager entityManager2(componentFactory, entityGenerator);

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<std::tuple<int, EmptyComponent*, TransformComponent*>> components;
		entityManager1.getComponents<EmptyComponent, TransformComponent>(components, 10);
		entityManager2.getComponents<EmptyComponent, TransformComponent>(components, 20);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());

		if (components.size() >= 2)
		{
			if (std::get<0>(components[0]) == 10)
			{
				EXPECT_EQ(20, std::get<0>(components[1]));
			}
			else
			{
				EXPECT_EQ(20, std::get<0>(components[0]));
				EXPECT_EQ(10, std::get<0>(components[1]));
			}
		}
	}
}

TEST(EntityManager, ComponentSetsWithEntitiesAndAdditionalDataCanBeCollected)
{
	using namespace ComponentTests;

	EntityGenerator entityGenerator(42);
	ComponentFactory componentFactory;
	RegisterComponents(componentFactory);
	EntityManager entityManager1(componentFactory, entityGenerator);
	EntityManager entityManager2(componentFactory, entityGenerator);

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<std::tuple<int, Entity, EmptyComponent*, TransformComponent*>> components;
		entityManager1.getComponentsWithEntities<EmptyComponent, TransformComponent>(components, 10);
		entityManager2.getComponentsWithEntities<EmptyComponent, TransformComponent>(components, 20);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());

		if (components.size() >= 2)
		{
			if (std::get<0>(components[0]) == 10)
			{
				EXPECT_EQ(testEntity1, std::get<1>(components[0]));
				EXPECT_EQ(20, std::get<0>(components[1]));
				EXPECT_EQ(testEntity2, std::get<1>(components[1]));
			}
			else
			{
				EXPECT_EQ(20, std::get<0>(components[0]));
				EXPECT_EQ(testEntity2, std::get<1>(components[0]));
				EXPECT_EQ(10, std::get<0>(components[1]));
				EXPECT_EQ(testEntity1, std::get<1>(components[1]));
			}
		}
	}
}
