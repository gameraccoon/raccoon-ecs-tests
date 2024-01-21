#include <gtest/gtest.h>

#include <functional>
#include <vector>
#include <thread>

#include "raccoon-ecs/entity_manager.h"

namespace TestEntityManager_Indexes_Internal
{
	enum ComponentType
	{
		ComponentTypeA,
		ComponentTypeB,
		ComponentTypeC,
		ComponentTypeD,
		ComponentTypeE,
		ComponentTypeF,
		ComponentTypeG,
		ComponentTypeH,
	};

	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<ComponentType>;
	using EntityManager = RaccoonEcs::EntityManagerImpl<ComponentType>;
	using Entity = RaccoonEcs::Entity;
	using TypedComponent = RaccoonEcs::TypedComponentImpl<ComponentType>;

	struct ComponentA
	{
		int value;

		static ComponentType GetTypeId() { return ComponentTypeA; };
	};

	struct ComponentB
	{
		int value;

		static ComponentType GetTypeId() { return ComponentTypeB; };
	};

	struct ComponentC
	{
		int value;

		static ComponentType GetTypeId() { return ComponentTypeC; };
	};

	struct ComponentD
	{
		int value;

		static ComponentType GetTypeId() { return ComponentTypeD; };
	};

	struct ComponentE
	{
		int value;

		static ComponentType GetTypeId() { return ComponentTypeE; };
	};

	struct ComponentF
	{
		int value;

		static ComponentType GetTypeId() { return ComponentTypeF; };
	};

	struct ComponentG
	{
		int value;

		static ComponentType GetTypeId() { return ComponentTypeG; };
	};

	struct ComponentH
	{
		int value;

		static ComponentType GetTypeId() { return ComponentTypeH; };
	};

	struct EntityManagerData
	{
		ComponentFactory componentFactory;
		EntityManager entityManager{componentFactory};
	};

	static void RegisterComponents(ComponentFactory& inOutFactory)
	{
		inOutFactory.registerComponent<ComponentA>();
		inOutFactory.registerComponent<ComponentB>();
		inOutFactory.registerComponent<ComponentC>();
		inOutFactory.registerComponent<ComponentD>();
		inOutFactory.registerComponent<ComponentE>();
		inOutFactory.registerComponent<ComponentF>();
		inOutFactory.registerComponent<ComponentG>();
		inOutFactory.registerComponent<ComponentH>();
	}

	static std::unique_ptr<EntityManagerData> PrepareEntityManager()
	{
		auto data = std::make_unique<EntityManagerData>();
		RegisterComponents(data->componentFactory);
		return data;
	}

	static std::tuple<Entity, Entity, Entity> setUpComponentPermutationsFor3Entities(EntityManager& entityManager) {
		//   1 2 3
		// A x
		// B   x
		// C x x
		// D     x
		// E x   x
		// F   x x
		// G x x x
		// H

		const Entity entity1 = entityManager.addEntity();
		entityManager.addComponent<ComponentA>(entity1)->value = 1;
		entityManager.addComponent<ComponentC>(entity1)->value = 3;
		entityManager.addComponent<ComponentE>(entity1)->value = 5;
		entityManager.addComponent<ComponentG>(entity1)->value = 7;

		const Entity entity2 = entityManager.addEntity();
		entityManager.addComponent<ComponentB>(entity2)->value = 20;
		entityManager.addComponent<ComponentC>(entity2)->value = 30;
		entityManager.addComponent<ComponentF>(entity2)->value = 60;
		entityManager.addComponent<ComponentG>(entity2)->value = 70;

		const Entity entity3 = entityManager.addEntity();
		entityManager.addComponent<ComponentD>(entity3)->value = 400;
		entityManager.addComponent<ComponentE>(entity3)->value = 500;
		entityManager.addComponent<ComponentF>(entity3)->value = 600;
		entityManager.addComponent<ComponentG>(entity3)->value = 700;

		entityManager.initIndex<ComponentA>();
		entityManager.initIndex<ComponentB>();
		entityManager.initIndex<ComponentC>();
		entityManager.initIndex<ComponentD>();
		entityManager.initIndex<ComponentE>();
		entityManager.initIndex<ComponentF>();
		entityManager.initIndex<ComponentG>();
		entityManager.initIndex<ComponentH>();

		return {entity1, entity2, entity3};
	}

	template<typename Component>
	static void checkComponentEntities(EntityManager& entityManager, std::vector<std::pair<Entity, int>> entities) {
		entityManager.forEachComponentSetWithEntity<Component>([&entities](Entity entity, const Component* component) mutable {
			auto it = std::find_if(entities.begin(), entities.end(), [entity](auto& pair) { return pair.first == entity; });
			ASSERT_NE(it, entities.end());
			EXPECT_EQ(component->value, it->second);
			entities.erase(it);
		});

		EXPECT_TRUE(entities.empty());
	}
} // namespace EntityManagerTestInternals

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityInIndexWithLastEntityInIndex)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity1);
		a->value = 100;
	}

	const Entity testEntity2 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity2);
		a->value = 200;
	}

	entityManager.initIndex<ComponentA>();

	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const ComponentA*>> resultComponents;
	entityManager.getComponents<const ComponentA>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
	EXPECT_EQ(std::get<0>(resultComponents[0])->value, 200);
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityInIndexWithLastEntityNotInIndex)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity1);
		a->value = 100;
	}

	entityManager.addEntity();

	entityManager.initIndex<ComponentA>();

	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const ComponentA*>> resultComponents;
	entityManager.getComponents<const ComponentA>(resultComponents);
	EXPECT_EQ(resultComponents.size(), static_cast<size_t>(0));
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityNotInIndexWithLastEntityInIndex)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	[[maybe_unused]] const Entity testEntity1 = entityManager.addEntity();

	const Entity testEntity2 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity2);
		a->value = 200;
	}

	entityManager.initIndex<ComponentA>();

	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const ComponentA*>> resultComponents;
	entityManager.getComponents<const ComponentA>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
	EXPECT_EQ(std::get<0>(resultComponents[0])->value, 200);
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityNotInIndexWithLastEntityNotInIndex)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	[[maybe_unused]] const Entity testEntity1 = entityManager.addEntity();
	[[maybe_unused]] const Entity testEntity2 = entityManager.addEntity();

	entityManager.initIndex<ComponentA>();

	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const ComponentA*>> resultComponents;
	entityManager.getComponents<const ComponentA>(resultComponents);
	EXPECT_EQ(resultComponents.size(), static_cast<size_t>(0));
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityInIndexWithReversedDenseArray)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;
	entityManager.initIndex<ComponentA>();

	const Entity testEntity1 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity1);
		a->value = 100;
	}

	const Entity testEntity2 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity2);
		a->value = 200;
	}

	const Entity testEntity3 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity3);
		a->value = 300;
	}

	const Entity testEntity4 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity4);
		a->value = 400;
	}

	entityManager.removeEntity(testEntity2);
	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const ComponentA*>> resultComponents;
	entityManager.getComponents<const ComponentA>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(2));
	std::sort(resultComponents.begin(), resultComponents.end(), [](auto& set1, auto& set2) { return std::get<0>(set1)->value < std::get<0>(set2)->value; });
	EXPECT_EQ(std::get<0>(resultComponents[0])->value, 300);
	EXPECT_EQ(std::get<0>(resultComponents[1])->value, 400);
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityInIndexThenCopyEntityManager)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity1);
		a->value = 100;
	}

	const Entity testEntity2 = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity2);
		a->value = 200;
	}

	entityManager.initIndex<ComponentA>();

	entityManager.removeEntity(testEntity1);

	EntityManager entityManagerCopy(entityManagerData->componentFactory);
	entityManagerCopy.overrideBy(entityManager);

	std::vector<std::tuple<const ComponentA*>> resultComponents;
	entityManager.getComponents<const ComponentA>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
	EXPECT_EQ(std::get<0>(resultComponents[0])->value, 200);
}

// regression test for a bug introduced in 00fad90
TEST(EntityManager, EntityManager_TransferOwnershipToAnotherThread_CanStillAccessEntities)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity = entityManager.addEntity();
	{
		ComponentA* a = entityManager.addComponent<ComponentA>(testEntity);
		a->value = 100;
	}

	entityManager.initIndex<ComponentA>();

	auto thread = std::thread([&entityManager]() {
		std::vector<std::tuple<const ComponentA*>> resultComponents;
		entityManager.getComponents<const ComponentA>(resultComponents);
		ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
		EXPECT_EQ(std::get<0>(resultComponents[0])->value, 100);
	});
	thread.join();
}

// regression test for a bug introduced in 7ecad63
TEST(EntityManager, TwoEntityMangersCreatedInDifferentThreads_AddAndRemoveIndexes_NoDataRaceOccurs)
{
	using namespace TestEntityManager_Indexes_Internal;
	auto thread1 = std::thread([]() {
		for (int i = 0; i < 1000; ++i)
		{
			auto entityManagerData = PrepareEntityManager();
			EntityManager& entityManager = entityManagerData->entityManager;
			entityManager.initIndex<ComponentA>();
		}
	});

	auto thread2 = std::thread([]() {
		for (int i = 0; i < 1000; ++i)
		{
			auto entityManagerData = PrepareEntityManager();
			EntityManager& entityManager = entityManagerData->entityManager;
			entityManager.initIndex<ComponentA>();
		}
	});

	thread1.join();
	thread2.join();
}

// regression test for a bug introduced in 00fad90
TEST(EntityManager, EntityManagerWithIndex_RemoveEntityNotInIndex_EntityDoesNotAppearInIndex)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity = entityManager.addEntity();
	const Entity testEntity2 = entityManager.addEntity();
	{
		ComponentB* b = entityManager.addComponent<ComponentB>(testEntity2);
		b->value = 500;
	}
	entityManager.initIndex<ComponentB>();

	entityManager.removeEntity(testEntity);

	entityManager.forEachComponentSetWithEntity<const ComponentB>([testEntity2](Entity entity, const ComponentB* b) {
		EXPECT_EQ(entity, testEntity2);
		EXPECT_EQ(b->value, 500);
	});
}

TEST(EntityManager, EntityManagerWithIndexes_RemoveFirstEntityInMultupleIndexes_IndexesAreNotCorrupted)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	auto [entity1, entity2, entity3] = setUpComponentPermutationsFor3Entities(entityManager);

	entityManager.removeEntity(entity1);

	checkComponentEntities<ComponentA>(entityManager, {});
	checkComponentEntities<ComponentB>(entityManager, {{entity2, 20}});
	checkComponentEntities<ComponentC>(entityManager, {{entity2, 30}});
	checkComponentEntities<ComponentD>(entityManager, {{entity3, 400}});
	checkComponentEntities<ComponentE>(entityManager, {{entity3, 500}});
	checkComponentEntities<ComponentF>(entityManager, {{entity2, 60}, {entity3, 600}});
	checkComponentEntities<ComponentG>(entityManager, {{entity2, 70}, {entity3, 700}});
	checkComponentEntities<ComponentH>(entityManager, {});

	Entity entity4 = entityManager.addEntity();
	entityManager.addComponent<ComponentA>(entity4)->value = 10000;
	entityManager.addComponent<ComponentB>(entity4)->value = 20000;
	entityManager.addComponent<ComponentC>(entity4)->value = 30000;
	entityManager.addComponent<ComponentD>(entity4)->value = 40000;
	entityManager.addComponent<ComponentE>(entity4)->value = 50000;
	entityManager.addComponent<ComponentF>(entity4)->value = 60000;
	entityManager.addComponent<ComponentG>(entity4)->value = 70000;
	entityManager.addComponent<ComponentH>(entity4)->value = 80000;

	checkComponentEntities<ComponentA>(entityManager, {{entity4, 10000}});
	checkComponentEntities<ComponentB>(entityManager, {{entity2, 20}, {entity4, 20000}});
	checkComponentEntities<ComponentC>(entityManager, {{entity2, 30}, {entity4, 30000}});
	checkComponentEntities<ComponentD>(entityManager, {{entity3, 400}, {entity4, 40000}});
	checkComponentEntities<ComponentE>(entityManager, {{entity3, 500}, {entity4, 50000}});
	checkComponentEntities<ComponentF>(entityManager, {{entity2, 60}, {entity3, 600}, {entity4, 60000}});
	checkComponentEntities<ComponentG>(entityManager, {{entity2, 70}, {entity3, 700}, {entity4, 70000}});
	checkComponentEntities<ComponentH>(entityManager, {{entity4, 80000}});
}

TEST(EntityManager, EntityManagerWithIndexes_RemoveMiddleEntityInMultupleIndexes_IndexesAreNotCorrupted)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	auto [entity1, entity2, entity3] = setUpComponentPermutationsFor3Entities(entityManager);

	entityManager.removeEntity(entity2);

	checkComponentEntities<ComponentA>(entityManager, {{entity1, 1}});
	checkComponentEntities<ComponentB>(entityManager, {});
	checkComponentEntities<ComponentC>(entityManager, {{entity1, 3}});
	checkComponentEntities<ComponentD>(entityManager, {{entity3, 400}});
	checkComponentEntities<ComponentE>(entityManager, {{entity1, 5}, {entity3, 500}});
	checkComponentEntities<ComponentF>(entityManager, {{entity3, 600}});
	checkComponentEntities<ComponentG>(entityManager, {{entity1, 7}, {entity3, 700}});
	checkComponentEntities<ComponentH>(entityManager, {});

	Entity entity4 = entityManager.addEntity();
	entityManager.addComponent<ComponentA>(entity4)->value = 10000;
	entityManager.addComponent<ComponentB>(entity4)->value = 20000;
	entityManager.addComponent<ComponentC>(entity4)->value = 30000;
	entityManager.addComponent<ComponentD>(entity4)->value = 40000;
	entityManager.addComponent<ComponentE>(entity4)->value = 50000;
	entityManager.addComponent<ComponentF>(entity4)->value = 60000;
	entityManager.addComponent<ComponentG>(entity4)->value = 70000;
	entityManager.addComponent<ComponentH>(entity4)->value = 80000;

	checkComponentEntities<ComponentA>(entityManager, {{entity1, 1}, {entity4, 10000}});
	checkComponentEntities<ComponentB>(entityManager, {{entity4, 20000}});
	checkComponentEntities<ComponentC>(entityManager, {{entity1, 3}, {entity4, 30000}});
	checkComponentEntities<ComponentD>(entityManager, {{entity3, 400}, {entity4, 40000}});
	checkComponentEntities<ComponentE>(entityManager, {{entity1, 5}, {entity3, 500}, {entity4, 50000}});
	checkComponentEntities<ComponentF>(entityManager, {{entity3, 600}, {entity4, 60000}});
	checkComponentEntities<ComponentG>(entityManager, {{entity1, 7}, {entity3, 700}, {entity4, 70000}});
	checkComponentEntities<ComponentH>(entityManager, {{entity4, 80000}});
}

TEST(EntityManager, EntityManagerWithIndexes_RemoveLastEntityInMultupleIndexes_IndexesAreNotCorrupted)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	auto [entity1, entity2, entity3] = setUpComponentPermutationsFor3Entities(entityManager);

	entityManager.removeEntity(entity3);

	checkComponentEntities<ComponentA>(entityManager, {{entity1, 1}});
	checkComponentEntities<ComponentB>(entityManager, {{entity2, 20}});
	checkComponentEntities<ComponentC>(entityManager, {{entity1, 3}, {entity2, 30}});
	checkComponentEntities<ComponentD>(entityManager, {});
	checkComponentEntities<ComponentE>(entityManager, {{entity1, 5}});
	checkComponentEntities<ComponentF>(entityManager, {{entity2, 60}});
	checkComponentEntities<ComponentG>(entityManager, {{entity1, 7}, {entity2, 70}});
	checkComponentEntities<ComponentH>(entityManager, {});

	Entity entity4 = entityManager.addEntity();
	entityManager.addComponent<ComponentA>(entity4)->value = 10000;
	entityManager.addComponent<ComponentB>(entity4)->value = 20000;
	entityManager.addComponent<ComponentC>(entity4)->value = 30000;
	entityManager.addComponent<ComponentD>(entity4)->value = 40000;
	entityManager.addComponent<ComponentE>(entity4)->value = 50000;
	entityManager.addComponent<ComponentF>(entity4)->value = 60000;
	entityManager.addComponent<ComponentG>(entity4)->value = 70000;
	entityManager.addComponent<ComponentH>(entity4)->value = 80000;

	checkComponentEntities<ComponentA>(entityManager, {{entity1, 1}, {entity4, 10000}});
	checkComponentEntities<ComponentB>(entityManager, {{entity2, 20}, {entity4, 20000}});
	checkComponentEntities<ComponentC>(entityManager, {{entity1, 3}, {entity2, 30}, {entity4, 30000}});
	checkComponentEntities<ComponentD>(entityManager, {{entity4, 40000}});
	checkComponentEntities<ComponentE>(entityManager, {{entity1, 5}, {entity4, 50000}});
	checkComponentEntities<ComponentF>(entityManager, {{entity2, 60}, {entity4, 60000}});
	checkComponentEntities<ComponentG>(entityManager, {{entity1, 7}, {entity2, 70}, {entity4, 70000}});
	checkComponentEntities<ComponentH>(entityManager, {{entity4, 80000}});
}

TEST(EntityManager, EntityManager_TransferFirstEntityToAnotherManager_CanStillAccessEntities)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager;
	EntityManager entityManager2{entityManagerData->componentFactory};

	auto [entity1, entity2, entity3] = setUpComponentPermutationsFor3Entities(entityManager1);

	const Entity transferredEntity = entityManager1.transferEntityTo(entityManager2, entity1);

	checkComponentEntities<ComponentA>(entityManager1, {});
	checkComponentEntities<ComponentB>(entityManager1, {{entity2, 20}});
	checkComponentEntities<ComponentC>(entityManager1, {{entity2, 30}});
	checkComponentEntities<ComponentD>(entityManager1, {{entity3, 400}});
	checkComponentEntities<ComponentE>(entityManager1, {{entity3, 500}});
	checkComponentEntities<ComponentF>(entityManager1, {{entity2, 60}, {entity3, 600}});
	checkComponentEntities<ComponentG>(entityManager1, {{entity2, 70}, {entity3, 700}});
	checkComponentEntities<ComponentH>(entityManager1, {});

	checkComponentEntities<ComponentA>(entityManager2, {{transferredEntity, 1}});
	checkComponentEntities<ComponentB>(entityManager2, {});
	checkComponentEntities<ComponentC>(entityManager2, {{transferredEntity, 3}});
	checkComponentEntities<ComponentD>(entityManager2, {});
	checkComponentEntities<ComponentE>(entityManager2, {{transferredEntity, 5}});
	checkComponentEntities<ComponentF>(entityManager2, {});
	checkComponentEntities<ComponentG>(entityManager2, {{transferredEntity, 7}});
	checkComponentEntities<ComponentH>(entityManager2, {});
}

TEST(EntityManager, EntityManager_TransferMiddleEntityToAnotherManager_CanStillAccessEntities)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager;
	EntityManager entityManager2{entityManagerData->componentFactory};

	auto [entity1, entity2, entity3] = setUpComponentPermutationsFor3Entities(entityManager1);

	const Entity transferredEntity = entityManager1.transferEntityTo(entityManager2, entity2);

	checkComponentEntities<ComponentA>(entityManager1, {{entity1, 1}});
	checkComponentEntities<ComponentB>(entityManager1, {});
	checkComponentEntities<ComponentC>(entityManager1, {{entity1, 3}});
	checkComponentEntities<ComponentD>(entityManager1, {{entity3, 400}});
	checkComponentEntities<ComponentE>(entityManager1, {{entity1, 5}, {entity3, 500}});
	checkComponentEntities<ComponentF>(entityManager1, {{entity3, 600}});
	checkComponentEntities<ComponentG>(entityManager1, {{entity1, 7}, {entity3, 700}});
	checkComponentEntities<ComponentH>(entityManager1, {});

	checkComponentEntities<ComponentA>(entityManager2, {});
	checkComponentEntities<ComponentB>(entityManager2, {{transferredEntity, 20}});
	checkComponentEntities<ComponentC>(entityManager2, {{transferredEntity, 30}});
	checkComponentEntities<ComponentD>(entityManager2, {});
	checkComponentEntities<ComponentE>(entityManager2, {});
	checkComponentEntities<ComponentF>(entityManager2, {{transferredEntity, 60}});
	checkComponentEntities<ComponentG>(entityManager2, {{transferredEntity, 70}});
	checkComponentEntities<ComponentH>(entityManager2, {});
}

TEST(EntityManager, EntityManager_TransferLastEntityToAnotherManager_CanStillAccessEntities)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager;
	EntityManager entityManager2{entityManagerData->componentFactory};

	auto [entity1, entity2, entity3] = setUpComponentPermutationsFor3Entities(entityManager1);

	const Entity transferredEntity = entityManager1.transferEntityTo(entityManager2, entity3);

	checkComponentEntities<ComponentA>(entityManager1, {{entity1, 1}});
	checkComponentEntities<ComponentB>(entityManager1, {{entity2, 20}});
	checkComponentEntities<ComponentC>(entityManager1, {{entity1, 3}, {entity2, 30}});
	checkComponentEntities<ComponentD>(entityManager1, {});
	checkComponentEntities<ComponentE>(entityManager1, {{entity1, 5}});
	checkComponentEntities<ComponentF>(entityManager1, {{entity2, 60}});
	checkComponentEntities<ComponentG>(entityManager1, {{entity1, 7}, {entity2, 70}});
	checkComponentEntities<ComponentH>(entityManager1, {});

	checkComponentEntities<ComponentA>(entityManager2, {});
	checkComponentEntities<ComponentB>(entityManager2, {});
	checkComponentEntities<ComponentC>(entityManager2, {});
	checkComponentEntities<ComponentD>(entityManager2, {{transferredEntity, 400}});
	checkComponentEntities<ComponentE>(entityManager2, {{transferredEntity, 500}});
	checkComponentEntities<ComponentF>(entityManager2, {{transferredEntity, 600}});
	checkComponentEntities<ComponentG>(entityManager2, {{transferredEntity, 700}});
	checkComponentEntities<ComponentH>(entityManager2, {});
}
