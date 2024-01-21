#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "raccoon-ecs/entity_manager.h"
#include "raccoon-ecs/utils/combined_entity_manager_view.h"

namespace TestCombinedEntityManagerView_Internal
{
	enum ComponentType
	{
		EmptyComponentId,
		TransformComponentId,
		MovementComponentId,
	};

	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<ComponentType>;
	using EntityManager = RaccoonEcs::EntityManagerImpl<ComponentType>;
	using EntityView = RaccoonEcs::EntityViewImpl<EntityManager>;
	using Entity = RaccoonEcs::Entity;
	using TypedComponent = RaccoonEcs::TypedComponentImpl<ComponentType>;
	using CombinedEntityManagerView = RaccoonEcs::CombinedEntityManagerView<EntityManager, int>;

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
		EntityManager entityManager1{componentFactory};
		EntityManager entityManager2{componentFactory};
		int data1 = 20;
		int data2 = 50;
		CombinedEntityManagerView combinedEntityManager{{{entityManager1, data1}, {entityManager2, data2}}};
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

TEST(CombinedEntityManagerView, ComponentSetsCanBeIteratedOver)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;
	CombinedEntityManagerView& combinedEntityManager = entityManagerData->combinedEntityManager;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		int iterationsCount = 0;
		combinedEntityManager.forEachComponentSet<MovementComponent>(
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
		combinedEntityManager.forEachComponentSet<TransformComponent>(transformPredicate);
		EXPECT_EQ(2, iterationsCount);

		// call the second time to check that cached data is valid
		combinedEntityManager.forEachComponentSet<TransformComponent>(transformPredicate);
		EXPECT_EQ(4, iterationsCount);
	}

	{
		int iterationsCount = 0;
		combinedEntityManager.forEachComponentSet<EmptyComponent, TransformComponent>(
			[&iterationsCount](EmptyComponent*, TransformComponent*) {
				++iterationsCount;
			}
		);
		EXPECT_EQ(1, iterationsCount);
	}
}

TEST(CombinedEntityManagerView, ComponentSetsCanBeIteratedOverWithEntities)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;
	CombinedEntityManagerView& combinedEntityManager = entityManagerData->combinedEntityManager;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		int iterationsCount = 0;
		combinedEntityManager.forEachComponentSetWithEntity<MovementComponent>(
			[&iterationsCount, testEntity1](EntityView entityView, MovementComponent*) {
				EXPECT_EQ(testEntity1, entityView.getEntity());
				++iterationsCount;
			}
		);
		EXPECT_EQ(1, iterationsCount);
	}

	{
		int iterationsCount = 0;
		auto transformPredicate = [&iterationsCount](EntityView, TransformComponent*) {
			++iterationsCount;
		};
		combinedEntityManager.forEachComponentSetWithEntity<TransformComponent>(transformPredicate);
		EXPECT_EQ(2, iterationsCount);

		// call the second time to check that cached data is valid
		combinedEntityManager.forEachComponentSetWithEntity<TransformComponent>(transformPredicate);
		EXPECT_EQ(4, iterationsCount);
	}

	{
		int iterationsCount = 0;
		combinedEntityManager.forEachComponentSetWithEntity<EmptyComponent, TransformComponent>(
			[&iterationsCount, testEntity2](EntityView entityView, EmptyComponent*, TransformComponent*) {
				EXPECT_EQ(testEntity2, entityView.getEntity());
				++iterationsCount;
			}
		);
		EXPECT_EQ(1, iterationsCount);
	}
}

TEST(CombinedEntityManagerView, ComponentSetsCanBeCollected)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;
	CombinedEntityManagerView& combinedEntityManager = entityManagerData->combinedEntityManager;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<std::tuple<MovementComponent*>> components;
		combinedEntityManager.getComponents<MovementComponent>(components);
		EXPECT_EQ(static_cast<size_t>(1u), components.size());
	}

	{
		std::vector<std::tuple<TransformComponent*>> components;
		combinedEntityManager.getComponents<TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());

		// call the second time to check that cached data is valid
		combinedEntityManager.getComponents<TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(4u), components.size());
	}

	{
		std::vector<std::tuple<EmptyComponent*, TransformComponent*>> components;
		combinedEntityManager.getComponents<EmptyComponent, TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(1u), components.size());
	}
}

TEST(CombinedEntityManagerView, ComponentSetsWithEntitiesCanBeCollected)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;
	CombinedEntityManagerView& combinedEntityManager = entityManagerData->combinedEntityManager;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<MovementComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<std::tuple<Entity, MovementComponent*>> components;
		combinedEntityManager.getComponentsWithEntities<MovementComponent>(components);
		ASSERT_EQ(static_cast<size_t>(1u), components.size());
		EXPECT_EQ(testEntity1, std::get<0>(components[0]));
	}

	{
		std::vector<std::tuple<Entity, TransformComponent*>> components;
		combinedEntityManager.getComponentsWithEntities<TransformComponent>(components);
		ASSERT_EQ(static_cast<size_t>(2u), components.size());
		EXPECT_EQ(testEntity1, std::get<0>(components[0]));
		EXPECT_EQ(testEntity2, std::get<0>(components[1]));

		// call the second time to check that cached data is valid
		combinedEntityManager.getComponentsWithEntities<TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(4u), components.size());
	}

	{
		std::vector<std::tuple<Entity, EmptyComponent*, TransformComponent*>> components;
		combinedEntityManager.getComponentsWithEntities<EmptyComponent, TransformComponent>(components);
		ASSERT_EQ(static_cast<size_t>(1u), components.size());
		EXPECT_EQ(testEntity2, std::get<0>(components[0]));
	}
}

TEST(CombinedEntityManagerView, ComponentSetsCanBeIteratedOverWithAdditionalData)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;
	CombinedEntityManagerView& combinedEntityManager = entityManagerData->combinedEntityManager;

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
		combinedEntityManager.forEachComponentSetWithExtraData<EmptyComponent, TransformComponent>(iterationFunction);
		EXPECT_EQ(70, sum);
	}
}

TEST(CombinedEntityManagerView, ComponentSetsCanBeIteratedOverWithEntitiesAndAdditionalData)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;
	CombinedEntityManagerView& combinedEntityManager = entityManagerData->combinedEntityManager;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		int sum = 0;
		auto iterationFunction = [&sum](int data, EntityView, EmptyComponent*, TransformComponent*) {
			sum += data;
		};
		combinedEntityManager.forEachComponentSetWithEntityAndExtraData<EmptyComponent, TransformComponent>(iterationFunction);
		EXPECT_EQ(70, sum);
	}
}

TEST(CombinedEntityManagerView, ComponentSetsWithAdditionalDataCanBeCollected)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;
	CombinedEntityManagerView& combinedEntityManager = entityManagerData->combinedEntityManager;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<std::tuple<int, EmptyComponent*, TransformComponent*>> components;
		combinedEntityManager.getComponentsWithExtraData<EmptyComponent, TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());

		if (components.size() >= 2)
		{
			if (std::get<0>(components[0]) == 20)
			{
				EXPECT_EQ(50, std::get<0>(components[1]));
			}
			else
			{
				EXPECT_EQ(50, std::get<0>(components[0]));
				EXPECT_EQ(20, std::get<0>(components[1]));
			}
		}
	}
}

TEST(CombinedEntityManagerView, ComponentSetsWithEntitiesAndAdditionalDataCanBeCollected)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;
	CombinedEntityManagerView& combinedEntityManager = entityManagerData->combinedEntityManager;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<std::tuple<int, Entity, EmptyComponent*, TransformComponent*>> components;
		combinedEntityManager.getComponentsWithEntitiesAndExtraData<EmptyComponent, TransformComponent>(components);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());

		if (components.size() >= 2)
		{
			if (std::get<0>(components[0]) == 20)
			{
				EXPECT_EQ(testEntity1, std::get<1>(components[0]));
				EXPECT_EQ(50, std::get<0>(components[1]));
				EXPECT_EQ(testEntity2, std::get<1>(components[1]));
			}
			else
			{
				EXPECT_EQ(50, std::get<0>(components[0]));
				EXPECT_EQ(testEntity2, std::get<1>(components[0]));
				EXPECT_EQ(20, std::get<0>(components[1]));
				EXPECT_EQ(testEntity1, std::get<1>(components[1]));
			}
		}
	}
}


TEST(CombinedEntityManagerView, CombinedEntityManagerView_GetAllEntityComponents_ReturnsAllComponents)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	{
		std::vector<TypedComponent> components;
		entityManagerData->combinedEntityManager.getAllEntityComponents(testEntity1, components);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());
	}
	{
		std::vector<TypedComponent> components;
		entityManagerData->combinedEntityManager.getAllEntityComponents(testEntity2, components);
		EXPECT_EQ(static_cast<size_t>(2u), components.size());
	}
}

TEST(CombinedEntityManagerView, CombinedEntityManagerView_ExecuteScheduledActions_ExecuteActionsAcrossAllEntityManagers)
{
	using namespace TestCombinedEntityManagerView_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager1;
	EntityManager& entityManager2 = entityManagerData->entityManager2;

	const Entity testEntity1 = entityManager1.addEntity();
	entityManager1.addComponent<TransformComponent>(testEntity1);
	entityManager1.addComponent<EmptyComponent>(testEntity1);

	const Entity testEntity2 = entityManager2.addEntity();
	entityManager2.addComponent<TransformComponent>(testEntity2);
	entityManager2.addComponent<EmptyComponent>(testEntity2);

	entityManager1.scheduleRemoveComponent<EmptyComponent>(testEntity1);
	entityManager2.scheduleRemoveComponent<TransformComponent>(testEntity2);

	entityManagerData->combinedEntityManager.executeScheduledActions();

	{
		std::vector<TypedComponent> components;
		entityManagerData->combinedEntityManager.getAllEntityComponents(testEntity1, components);
		EXPECT_EQ(static_cast<size_t>(1u), components.size());
	}
	{
		std::vector<TypedComponent> components;
		entityManagerData->combinedEntityManager.getAllEntityComponents(testEntity2, components);
		EXPECT_EQ(static_cast<size_t>(1u), components.size());
	}
}