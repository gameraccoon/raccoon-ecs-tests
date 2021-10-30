#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "raccoon-ecs/async_entity_manager.h"
#include "raccoon-ecs/async_operations.h"
#include "raccoon-ecs/async_systems_manager.h"
#include "raccoon-ecs/entity_manager.h"

namespace AsyncSystemsTestInternal
{
	enum class ComponentType { ComponentA, ComponentB, ComponentC, ComponentD };

	std::string to_string(AsyncSystemsTestInternal::ComponentType componentType)
	{
		return componentType == AsyncSystemsTestInternal::ComponentType::ComponentA ? "ComponentA" :
			componentType == AsyncSystemsTestInternal::ComponentType::ComponentB ? "ComponentB" :
			componentType == AsyncSystemsTestInternal::ComponentType::ComponentC ? "ComponentC" :
			componentType == AsyncSystemsTestInternal::ComponentType::ComponentD ? "ComponentD" :
			"error";
	}

	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<ComponentType>;
	using EntityGenerator = RaccoonEcs::EntityGenerator;
	using EntityManager = RaccoonEcs::EntityManagerImpl<ComponentType>;
	using AsyncEntityManager = RaccoonEcs::AsyncEntityManagerImpl<ComponentType>;
	using TypedComponent = RaccoonEcs::TypedComponentImpl<ComponentType>;

	struct ComponentA
	{
		int data = 0;
		static ComponentType GetTypeId() { return ComponentType::ComponentA; };
	};

	struct ComponentB
	{
		int data = 0;
		static ComponentType GetTypeId() { return ComponentType::ComponentB; };
	};

	struct ComponentC
	{
		float data = 0.0f;
		static ComponentType GetTypeId() { return ComponentType::ComponentC; };
	};

	struct ComponentD
	{
		float data = 0.0f;
		static ComponentType GetTypeId() { return ComponentType::ComponentD; };
	};

	struct EntityManagerData
	{
		ComponentFactory componentFactory;
		EntityGenerator entityGenerator;
		EntityManager entityManager{componentFactory, entityGenerator};
		AsyncEntityManager asyncEntityManager{entityManager};
	};

	static void RegisterComponents(ComponentFactory& inOutFactory)
	{
		inOutFactory.registerComponent<ComponentA>();
		inOutFactory.registerComponent<ComponentB>();
		inOutFactory.registerComponent<ComponentC>();
		inOutFactory.registerComponent<ComponentD>();
	}

	std::unique_ptr<EntityManagerData> PrepareEntityManager()
	{
		auto data = std::make_unique<EntityManagerData>();
		RegisterComponents(data->componentFactory);
		return data;
		return nullptr;
	}

	class ComponentDataProducerSystem : public RaccoonEcs::System
	{
	public:
		ComponentDataProducerSystem(
			RaccoonEcs::ComponentFilter<ComponentA, ComponentB>&& abFilter,
			AsyncEntityManager& asyncEntityManager
		)
			: mABFilter(std::move(abFilter))
			, mAsyncEntityManager(asyncEntityManager)
		{}

		void update() override
		{
			mABFilter.forEachComponentSet(mAsyncEntityManager, [](ComponentA* componentA, ComponentB* componentB){
				componentA->data += 10;
				componentB->data += 20;
			});
		}

		static std::string GetSystemId() { return "ComponentDataProducerSystem"; }

	private:
		RaccoonEcs::ComponentFilter<ComponentA, ComponentB> mABFilter;
		AsyncEntityManager& mAsyncEntityManager;
	};

	class ComponentAtoCTransformSystem : public RaccoonEcs::System
	{
	public:
		ComponentAtoCTransformSystem(
			RaccoonEcs::ComponentFilter<const ComponentA, ComponentC>&& acFilter,
			AsyncEntityManager& asyncEntityManager
		)
			: mACFilter(std::move(acFilter))
			, mAsyncEntityManager(asyncEntityManager)
		{}

		void update() override
		{
			mACFilter.forEachComponentSet(mAsyncEntityManager, [](const ComponentA* componentA, ComponentC* componentC){
				componentC->data += static_cast<float>(componentA->data);
			});
		}

		static std::string GetSystemId() { return "ComponentAtoCTransformSystem"; }

	private:
		RaccoonEcs::ComponentFilter<const ComponentA, ComponentC> mACFilter;
		AsyncEntityManager& mAsyncEntityManager;
	};

	class ComponentBtoDTransformSystem : public RaccoonEcs::System
	{
	public:
		ComponentBtoDTransformSystem(
			RaccoonEcs::ComponentFilter<const ComponentB, ComponentD>&& bdFilter,
			AsyncEntityManager& asyncEntityManager
		)
			: mBDFilter(std::move(bdFilter))
			, mAsyncEntityManager(asyncEntityManager)
		{}

		void update() override
		{
			mBDFilter.forEachComponentSet(mAsyncEntityManager, [](const ComponentB* componentB, ComponentD* componentD){
				componentD->data += static_cast<float>(componentB->data);
			});
		}

		static std::string GetSystemId() { return "ComponentBtoDTransformSystem"; }

	private:
		RaccoonEcs::ComponentFilter<const ComponentB, ComponentD> mBDFilter;
		AsyncEntityManager& mAsyncEntityManager;
	};

	class ComponentConsumerSystem : public RaccoonEcs::System
	{
	public:
		ComponentConsumerSystem(
			RaccoonEcs::ComponentFilter<const ComponentC, const ComponentD>&& bdFilter,
			AsyncEntityManager& asyncEntityManager,
			std::function<void(float)>&& resultFn
		)
			: mBDFilter(std::move(bdFilter))
			, mAsyncEntityManager(asyncEntityManager)
			, mResultFn(std::move(resultFn))
		{}

		void update() override
		{
			float sum = 0.0f;
			mBDFilter.forEachComponentSet(mAsyncEntityManager, [&sum](const ComponentC* componentC, const ComponentD* componentD){
				sum += componentC->data + componentD->data;
			});
			mResultFn(sum);
		}

		static std::string GetSystemId() { return "ComponentConsumerSystem"; }

	private:
		RaccoonEcs::ComponentFilter<const ComponentC, const ComponentD> mBDFilter;
		AsyncEntityManager& mAsyncEntityManager;
		std::function<void(float)> mResultFn;
	};
} // namespace AsyncSystemsTestInternal

TEST(AsyncSystems, ExplicitOrder)
{
	using namespace AsyncSystemsTestInternal;

	auto componentSetHolderData = PrepareEntityManager();

	auto resultFn = [](float result) { EXPECT_FLOAT_EQ(93.0f, result); };

	RaccoonEcs::AsyncSystemsManager<ComponentType> systemManager;

	systemManager.registerSystem<ComponentDataProducerSystem
		, RaccoonEcs::ComponentFilter<ComponentA, ComponentB>
	>(RaccoonEcs::SystemDependencies()
		, componentSetHolderData->asyncEntityManager
	);

	systemManager.registerSystem<ComponentAtoCTransformSystem
		, RaccoonEcs::ComponentFilter<const ComponentA, ComponentC>
	>(RaccoonEcs::SystemDependencies().goesAfter<ComponentDataProducerSystem>()
		, componentSetHolderData->asyncEntityManager
	);

	systemManager.registerSystem<ComponentBtoDTransformSystem
		, RaccoonEcs::ComponentFilter<const ComponentB, ComponentD>
	>(RaccoonEcs::SystemDependencies().goesAfter<ComponentDataProducerSystem>()
		, componentSetHolderData->asyncEntityManager
	);

	systemManager.registerSystem<ComponentConsumerSystem
		, RaccoonEcs::ComponentFilter<const ComponentC, const ComponentD>
	>(RaccoonEcs::SystemDependencies().goesAfter<ComponentAtoCTransformSystem, ComponentBtoDTransformSystem>()
		, componentSetHolderData->asyncEntityManager
		, std::move(resultFn)
	);

	{
		{
			RaccoonEcs::Entity entity1 = componentSetHolderData->entityManager.addEntity();
			ComponentA* componentA1 = componentSetHolderData->entityManager.addComponent<ComponentA>(entity1);
			componentA1->data = 10;
			ComponentB* componentB1 = componentSetHolderData->entityManager.addComponent<ComponentB>(entity1);
			componentB1->data = 20;
			componentSetHolderData->entityManager.addComponent<ComponentC>(entity1);
			componentSetHolderData->entityManager.addComponent<ComponentD>(entity1);
		}

		{
			RaccoonEcs::Entity entity2 = componentSetHolderData->entityManager.addEntity();
			ComponentA* componentA2 = componentSetHolderData->entityManager.addComponent<ComponentA>(entity2);
			componentA2->data = 1;
			ComponentB* componentB2 = componentSetHolderData->entityManager.addComponent<ComponentB>(entity2);
			componentB2->data = 2;
			componentSetHolderData->entityManager.addComponent<ComponentC>(entity2);
			componentSetHolderData->entityManager.addComponent<ComponentD>(entity2);
		}
	}

	systemManager.init(2);

	systemManager.update();
}
