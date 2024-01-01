#include <gtest/gtest.h>

#include <functional>
#include <vector>
#include <thread>

#include "raccoon-ecs/entity_manager.h"

namespace TestEntityManager_Indexes_Internal
{
	enum ComponentType
	{
		EmptyComponentId,
		TransformComponentId,
		MovementComponentId,
		SingleIntComponentId,
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

	struct SingleIntComponent
	{
		int value;

		static ComponentType GetTypeId() { return SingleIntComponentId; };
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
		inOutFactory.registerComponent<SingleIntComponent>();
		inOutFactory.registerComponent<LifetimeCheckerComponent>();
	}

	static std::unique_ptr<EntityManagerData> PrepareEntityManager()
	{
		auto data = std::make_unique<EntityManagerData>();
		RegisterComponents(data->componentFactory);
		return data;
	}
} // namespace EntityManagerTestInternals

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityInIndexWithLastEntityInIndex)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity1);
		move->move.x = 100;
		move->move.y = 200;
	}

	const Entity testEntity2 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity2);
		move->move.x = 300;
		move->move.y = 400;
	}

	entityManager.initIndex<MovementComponent>();

	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const MovementComponent*>> resultComponents;
	entityManager.getComponents<const MovementComponent>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.x, 300);
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.y, 400);
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityInIndexWithLastEntityNotInIndex)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity1);
		move->move.x = 100;
		move->move.y = 200;
	}

	[[maybe_unused]] const Entity testEntity2 = entityManager.addEntity();

	entityManager.initIndex<MovementComponent>();

	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const MovementComponent*>> resultComponents;
	entityManager.getComponents<const MovementComponent>(resultComponents);
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
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity2);
		move->move.x = 300;
		move->move.y = 400;
	}

	entityManager.initIndex<MovementComponent>();

	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const MovementComponent*>> resultComponents;
	entityManager.getComponents<const MovementComponent>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.x, 300);
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.y, 400);
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityNotInIndexWithLastEntityNotInIndex)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	[[maybe_unused]] const Entity testEntity1 = entityManager.addEntity();
	[[maybe_unused]] const Entity testEntity2 = entityManager.addEntity();

	entityManager.initIndex<MovementComponent>();

	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const MovementComponent*>> resultComponents;
	entityManager.getComponents<const MovementComponent>(resultComponents);
	EXPECT_EQ(resultComponents.size(), static_cast<size_t>(0));
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityInIndexWithReversedDenseArray)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;
	entityManager.initIndex<MovementComponent>();

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity1);
		move->move.x = 100;
		move->move.y = 200;
	}

	const Entity testEntity2 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity2);
		move->move.x = 300;
		move->move.y = 400;
	}

	const Entity testEntity3 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity3);
		move->move.x = 500;
		move->move.y = 600;
	}

	const Entity testEntity4 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity4);
		move->move.x = 700;
		move->move.y = 800;
	}

	entityManager.removeEntity(testEntity2);
	entityManager.removeEntity(testEntity1);

	std::vector<std::tuple<const MovementComponent*>> resultComponents;
	entityManager.getComponents<const MovementComponent>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(2));
	std::sort(resultComponents.begin(), resultComponents.end(), [](auto& set1, auto& set2) { return std::get<0>(set1)->move.x < std::get<0>(set2)->move.x; });
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.x, 500);
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.y, 600);
	EXPECT_EQ(std::get<0>(resultComponents[1])->move.x, 700);
	EXPECT_EQ(std::get<0>(resultComponents[1])->move.y, 800);
}

TEST(EntityManager, CheckForCorruptingIndexes_RemoveEntityInIndexThenCopyEntityManager)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity1);
		move->move.x = 100;
		move->move.y = 200;
	}

	const Entity testEntity2 = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity2);
		move->move.x = 300;
		move->move.y = 400;
	}

	entityManager.initIndex<MovementComponent>();

	entityManager.removeEntity(testEntity1);

	EntityManager entityManagerCopy(entityManagerData->componentFactory, entityManagerData->entityGenerator);
	entityManagerCopy.overrideBy(entityManager);

	std::vector<std::tuple<const MovementComponent*>> resultComponents;
	entityManager.getComponents<const MovementComponent>(resultComponents);
	ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.x, 300);
	EXPECT_EQ(std::get<0>(resultComponents[0])->move.y, 400);
}

// regression test for a bug introduced in 00fad90
TEST(EntityManager, EntityManager_TransferOwnershipToAnotherThread_CanStillAccessEntities)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity = entityManager.addEntity();
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity);
		move->move.x = 100;
		move->move.y = 200;
	}

	entityManager.initIndex<MovementComponent>();

	auto thread = std::thread([&entityManager]() {
		std::vector<std::tuple<const MovementComponent*>> resultComponents;
		entityManager.getComponents<const MovementComponent>(resultComponents);
		ASSERT_EQ(resultComponents.size(), static_cast<size_t>(1));
		EXPECT_EQ(std::get<0>(resultComponents[0])->move.x, 100);
		EXPECT_EQ(std::get<0>(resultComponents[0])->move.y, 200);
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
			entityManager.initIndex<MovementComponent>();
		}
	});

	auto thread2 = std::thread([]() {
		for (int i = 0; i < 1000; ++i)
		{
			auto entityManagerData = PrepareEntityManager();
			EntityManager& entityManager = entityManagerData->entityManager;
			entityManager.initIndex<MovementComponent>();
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
		TransformComponent* transform = entityManager.addComponent<TransformComponent>(testEntity2);
		transform->pos.x = 500;
		transform->pos.y = 600;
	}
	entityManager.initIndex<TransformComponent>();

	entityManager.removeEntity(testEntity);

	entityManager.forEachComponentSetWithEntity<const TransformComponent>([testEntity2](Entity entity, const TransformComponent* transform) {
		EXPECT_EQ(entity, testEntity2);
		EXPECT_EQ(transform->pos.x, 500);
		EXPECT_EQ(transform->pos.y, 600);
	});
}

TEST(EntityManager, EntityManagerWithIndexes_RemoveFirstEntityInMultupleIndexes_IndexesAreNotCorrupted)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		TransformComponent* transform = entityManager.addComponent<TransformComponent>(testEntity1);
		transform->pos.x = 100;
		transform->pos.y = 200;
	}
	{
		SingleIntComponent* singleInt = entityManager.addComponent<SingleIntComponent>(testEntity1);
		singleInt->value = 300;
	}
	const Entity testEntity2 = entityManager.addEntity();
	{
		TransformComponent* transform = entityManager.addComponent<TransformComponent>(testEntity2);
		transform->pos.x = 400;
		transform->pos.y = 500;
	}
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity2);
		move->move.x = 600;
		move->move.y = 700;
	}
	entityManager.initIndex<TransformComponent>();
	entityManager.initIndex<MovementComponent>();
	entityManager.initIndex<SingleIntComponent>();

	entityManager.removeEntity(testEntity1);

	entityManager.forEachComponentSetWithEntity<const TransformComponent>([testEntity2](Entity entity, const TransformComponent* transform) {
		EXPECT_EQ(entity, testEntity2);
		EXPECT_EQ(transform->pos.x, 400);
		EXPECT_EQ(transform->pos.y, 500);
	});
	entityManager.forEachComponentSetWithEntity<const MovementComponent>([testEntity2](Entity entity, const MovementComponent* move) {
		EXPECT_EQ(entity, testEntity2);
		EXPECT_EQ(move->move.x, 600);
		EXPECT_EQ(move->move.y, 700);
	});
	entityManager.forEachComponentSetWithEntity<const SingleIntComponent>([](Entity, const SingleIntComponent*) {
		FAIL();
	});

	entityManager.removeComponent<TransformComponent>(testEntity2);
	entityManager.removeComponent<MovementComponent>(testEntity2);

	entityManager.forEachComponentSetWithEntity<const TransformComponent>([](Entity, const TransformComponent*) {
		FAIL();
	});
	entityManager.forEachComponentSetWithEntity<const MovementComponent>([](Entity, const MovementComponent*) {
		FAIL();
	});
}

TEST(EntityManager, EntityManagerWithIndexes_RemoveLastEntityInMultupleIndexes_IndexesAreNotCorrupted)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager = entityManagerData->entityManager;

	const Entity testEntity1 = entityManager.addEntity();
	{
		TransformComponent* transform = entityManager.addComponent<TransformComponent>(testEntity1);
		transform->pos.x = 100;
		transform->pos.y = 200;
	}
	{
		SingleIntComponent* singleInt = entityManager.addComponent<SingleIntComponent>(testEntity1);
		singleInt->value = 300;
	}
	const Entity testEntity2 = entityManager.addEntity();
	{
		TransformComponent* transform = entityManager.addComponent<TransformComponent>(testEntity2);
		transform->pos.x = 400;
		transform->pos.y = 500;
	}
	{
		MovementComponent* move = entityManager.addComponent<MovementComponent>(testEntity2);
		move->move.x = 600;
		move->move.y = 700;
	}
	entityManager.initIndex<TransformComponent>();
	entityManager.initIndex<MovementComponent>();
	entityManager.initIndex<SingleIntComponent>();

	entityManager.removeEntity(testEntity2);

	entityManager.forEachComponentSetWithEntity<const TransformComponent>([testEntity1](Entity entity, const TransformComponent* transform) {
		EXPECT_EQ(entity, testEntity1);
		EXPECT_EQ(transform->pos.x, 100);
		EXPECT_EQ(transform->pos.y, 200);
	});
	entityManager.forEachComponentSetWithEntity<const MovementComponent>([](Entity, const MovementComponent*) {
		FAIL();
	});
	entityManager.forEachComponentSetWithEntity<const SingleIntComponent>([testEntity1](Entity entity, const SingleIntComponent* singleInt) {
		EXPECT_EQ(entity, testEntity1);
		EXPECT_EQ(singleInt->value, 300);
	});

	entityManager.removeComponent<TransformComponent>(testEntity1);
	entityManager.removeComponent<SingleIntComponent>(testEntity1);

	entityManager.forEachComponentSetWithEntity<const TransformComponent>([](Entity, const TransformComponent*) {
		FAIL();
	});
	entityManager.forEachComponentSetWithEntity<const SingleIntComponent>([](Entity, const SingleIntComponent*) {
		FAIL();
	});
}

TEST(EntityManager, EntityManager_TransferFirstEntityToAnotherManager_CanStillAccessEntities)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager;
	EntityManager entityManager2{entityManagerData->componentFactory, entityManagerData->entityGenerator};

	const Entity testEntity = entityManager1.addEntity();
	{
		MovementComponent* move = entityManager1.addComponent<MovementComponent>(testEntity);
		move->move.x = 100;
		move->move.y = 200;
	}
	{
		SingleIntComponent* singleInt = entityManager1.addComponent<SingleIntComponent>(testEntity);
		singleInt->value = 300;
	}

	const Entity testEntity2 = entityManager1.addEntity();
	{
		MovementComponent* move = entityManager1.addComponent<MovementComponent>(testEntity2);
		move->move.x = 400;
		move->move.y = 500;
	}
	{
		TransformComponent* transform = entityManager1.addComponent<TransformComponent>(testEntity2);
		transform->pos.x = 600;
		transform->pos.y = 700;
	}

	entityManager1.initIndex<MovementComponent>();
	entityManager1.initIndex<TransformComponent>();
	entityManager1.initIndex<SingleIntComponent>();
	entityManager2.initIndex<MovementComponent>();
	entityManager2.initIndex<TransformComponent>();
	entityManager2.initIndex<SingleIntComponent>();

	entityManager1.transferEntityTo(entityManager2, testEntity);

	entityManager1.forEachComponentSetWithEntity<const MovementComponent>([testEntity2](Entity entity, const MovementComponent* move) {
		EXPECT_EQ(entity, testEntity2);
		EXPECT_EQ(move->move.x, 400);
		EXPECT_EQ(move->move.y, 500);
	});
	entityManager1.forEachComponentSetWithEntity<const TransformComponent>([testEntity2](Entity entity, const TransformComponent* transform) {
		EXPECT_EQ(entity, testEntity2);
		EXPECT_EQ(transform->pos.x, 600);
		EXPECT_EQ(transform->pos.y, 700);
	});
	entityManager1.forEachComponentSetWithEntity<const SingleIntComponent>([](Entity, const SingleIntComponent*) {
		FAIL();
	});

	entityManager2.forEachComponentSetWithEntity<const MovementComponent>([testEntity](Entity entity, const MovementComponent* move) {
		EXPECT_EQ(entity, testEntity);
		EXPECT_EQ(move->move.x, 100);
		EXPECT_EQ(move->move.y, 200);
	});
	entityManager2.forEachComponentSetWithEntity<const TransformComponent>([](Entity, const TransformComponent*) {
		FAIL();
	});
	entityManager2.forEachComponentSetWithEntity<const SingleIntComponent>([testEntity](Entity entity, const SingleIntComponent* singleInt) {
		EXPECT_EQ(entity, testEntity);
		EXPECT_EQ(singleInt->value, 300);
	});
}

TEST(EntityManager, EntityManager_TransferLastEntityToAnotherManager_CanStillAccessEntities)
{
	using namespace TestEntityManager_Indexes_Internal;

	auto entityManagerData = PrepareEntityManager();
	EntityManager& entityManager1 = entityManagerData->entityManager;
	EntityManager entityManager2{entityManagerData->componentFactory, entityManagerData->entityGenerator};

	const Entity testEntity1 = entityManager1.addEntity();
	{
		MovementComponent* move = entityManager1.addComponent<MovementComponent>(testEntity1);
		move->move.x = 100;
		move->move.y = 200;
	}
	{
		SingleIntComponent* singleInt = entityManager1.addComponent<SingleIntComponent>(testEntity1);
		singleInt->value = 300;
	}

	const Entity testEntity2 = entityManager1.addEntity();
	{
		MovementComponent* move = entityManager1.addComponent<MovementComponent>(testEntity2);
		move->move.x = 400;
		move->move.y = 500;
	}
	{
		TransformComponent* transform = entityManager1.addComponent<TransformComponent>(testEntity2);
		transform->pos.x = 600;
		transform->pos.y = 700;
	}

	entityManager1.initIndex<MovementComponent>();
	entityManager1.initIndex<TransformComponent>();
	entityManager1.initIndex<SingleIntComponent>();
	entityManager2.initIndex<MovementComponent>();
	entityManager2.initIndex<TransformComponent>();
	entityManager2.initIndex<SingleIntComponent>();

	entityManager1.transferEntityTo(entityManager2, testEntity2);

	entityManager1.forEachComponentSetWithEntity<const MovementComponent>([testEntity1](Entity entity, const MovementComponent* move) {
		EXPECT_EQ(entity, testEntity1);
		EXPECT_EQ(move->move.x, 100);
		EXPECT_EQ(move->move.y, 200);
	});
	entityManager1.forEachComponentSetWithEntity<const TransformComponent>([](Entity, const TransformComponent*) {
		FAIL();
	});
	entityManager1.forEachComponentSetWithEntity<const SingleIntComponent>([testEntity1](Entity entity, const SingleIntComponent* singleInt) {
		EXPECT_EQ(entity, testEntity1);
		EXPECT_EQ(singleInt->value, 300);
	});

	entityManager2.forEachComponentSetWithEntity<const MovementComponent>([testEntity2](Entity entity, const MovementComponent* move) {
		EXPECT_EQ(entity, testEntity2);
		EXPECT_EQ(move->move.x, 400);
		EXPECT_EQ(move->move.y, 500);
	});
	entityManager2.forEachComponentSetWithEntity<const TransformComponent>([testEntity2](Entity entity, const TransformComponent* transform) {
		EXPECT_EQ(entity, testEntity2);
		EXPECT_EQ(transform->pos.x, 600);
		EXPECT_EQ(transform->pos.y, 700);
	});
	entityManager2.forEachComponentSetWithEntity<const SingleIntComponent>([](Entity, const SingleIntComponent*) {
		FAIL();
	});
}
