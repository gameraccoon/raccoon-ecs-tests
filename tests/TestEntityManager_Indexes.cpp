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