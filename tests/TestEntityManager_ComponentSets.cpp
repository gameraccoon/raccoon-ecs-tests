#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "raccoon-ecs/entity_manager.h"

namespace TestEntityManager_ComponentSets_Internal
{
	enum ComponentType
	{
		EmptyComponentId,
		TransformComponentId,
		MovementComponentId,
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
	}

	static std::unique_ptr<EntityManagerData> PrepareEntityManager()
	{
		auto data = std::make_unique<EntityManagerData>();
		RegisterComponents(data->componentFactory);
		return data;
	}
} // namespace EntityManagerTestInternals

TEST(EntityManager, ComponentSetsCanBeIteratedOver)
{
	using namespace TestEntityManager_ComponentSets_Internal;

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
			[&iterationsCount](MovementComponent*) {
				++iterationsCount;
			}
		);
		EXPECT_EQ(1, iterationsCount);
	}

	{
		int iterationsCount = 0;
		auto transformPredicate =
			[&iterationsCount](TransformComponent*) {
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
			[&iterationsCount](EmptyComponent*, TransformComponent*) {
				++iterationsCount;
			}
		);
		EXPECT_EQ(1, iterationsCount);
	}
}

TEST(EntityManager, ComponentSetsCanBeIteratedOverWithEntities)
{
	using namespace TestEntityManager_ComponentSets_Internal;

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
			[&iterationsCount, testEntity1](Entity entity, MovementComponent*) {
				EXPECT_EQ(testEntity1, entity);
				++iterationsCount;
			}
		);
		EXPECT_EQ(1, iterationsCount);
	}

	{
		int iterationsCount = 0;
		auto transformPredicate = [&iterationsCount](Entity, TransformComponent*) {
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
			[&iterationsCount, testEntity2](Entity entity, EmptyComponent*, TransformComponent*) {
				EXPECT_EQ(testEntity2, entity);
				++iterationsCount;
			}
		);
		EXPECT_EQ(1, iterationsCount);
	}
}

TEST(EntityManager, ComponentSetsCanBeCollected)
{
	using namespace TestEntityManager_ComponentSets_Internal;

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
	using namespace TestEntityManager_ComponentSets_Internal;

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

TEST(EntityManager, ComponentSetsCanBeIteratedOverWithAdditionalData)
{
	using namespace TestEntityManager_ComponentSets_Internal;

	EntityGenerator entityGenerator;
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
		auto iterationFunction = [&sum](int data, EmptyComponent*, TransformComponent*) {
			sum += data;
		};
		entityManager1.forEachComponentSet<EmptyComponent, TransformComponent>(iterationFunction, 20);
		entityManager2.forEachComponentSet<EmptyComponent, TransformComponent>(iterationFunction, 50);
		EXPECT_EQ(70, sum);
	}
}

TEST(EntityManager, ComponentSetsCanBeIteratedOverWithEntitiesAndAdditionalData)
{
	using namespace TestEntityManager_ComponentSets_Internal;

	EntityGenerator entityGenerator;
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
		auto iterationFunction = [&sum](int data, Entity, EmptyComponent*, TransformComponent*) {
			sum += data;
		};
		entityManager1.forEachComponentSetWithEntity<EmptyComponent, TransformComponent>(iterationFunction, 20);
		entityManager2.forEachComponentSetWithEntity<EmptyComponent, TransformComponent>(iterationFunction, 50);
		EXPECT_EQ(70, sum);
	}
}

TEST(EntityManager, ComponentSetsWithAdditionalDataCanBeCollected)
{
	using namespace TestEntityManager_ComponentSets_Internal;

	EntityGenerator entityGenerator;
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
	using namespace TestEntityManager_ComponentSets_Internal;

	EntityGenerator entityGenerator;
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
