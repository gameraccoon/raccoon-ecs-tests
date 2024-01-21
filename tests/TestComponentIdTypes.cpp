#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "raccoon-ecs/entity_manager.h"
#include "raccoon-ecs/component_set_holder.h"

namespace TestComponentIdTypes_Internal
{
	struct ComponentWithStringId
	{
		static std::string GetTypeId() { return "ComponentWithStringId"; };
	};

	struct ComponentWithConstCharId
	{
		static const char* GetTypeId() { return "ComponentWithConstCharId"; };
	};

	class CustomString : public std::string
	{
		using std::string::string;
	};

	std::string to_string(const CustomString& value)
	{
		return value;
	}

	struct ComponentWithIntegerId
	{
		static int GetTypeId() { return 1; };
	};

	struct ComponentWithCustomStringId
	{
		static CustomString GetTypeId() { return CustomString("WithCustomStringId"); };
	};

	enum class EnumComponentId
	{
		ComponentWithEnumId
	};

	std::string to_string(const EnumComponentId& value)
	{
		switch (value)
		{
		case EnumComponentId::ComponentWithEnumId:
			return "ComponentWithEnumId";
		default:
			return "Unknown";
		}
	}

	struct ComponentWithEnumId
	{
		static EnumComponentId GetTypeId() { return EnumComponentId::ComponentWithEnumId; };
	};

	using Entity = RaccoonEcs::Entity;
}

namespace std
{
	template<>
	struct hash<TestComponentIdTypes_Internal::CustomString>
	{
		size_t operator()(const TestComponentIdTypes_Internal::CustomString& value) const
		{
			return std::hash<std::string>()(value);
		}
	};
}

TEST(ComponentIdTypes, EntityManagerWithStringComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using EntityManager = RaccoonEcs::EntityManagerImpl<std::string>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<std::string>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithStringId>();
	EntityManager entityManager(componentFactory);

	const Entity testEntity = entityManager.addEntity();
	entityManager.addComponent<ComponentWithStringId>(testEntity);

	EXPECT_TRUE(entityManager.doesEntityHaveComponent<ComponentWithStringId>(testEntity));
}

TEST(ComponentIdTypes, ComponentSetHolderWithStringComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using ComponentSetHolder = RaccoonEcs::ComponentSetHolderImpl<std::string>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<std::string>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithStringId>();
	ComponentSetHolder componentSetHolder(componentFactory);

	componentSetHolder.addComponent<ComponentWithStringId>();

	EXPECT_NE(std::get<0>(componentSetHolder.getComponents<ComponentWithStringId>()), nullptr);
}

TEST(ComponentIdTypes, EntityManagerWithConstCharComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using EntityManager = RaccoonEcs::EntityManagerImpl<const char*>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<const char*>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithConstCharId>();
	EntityManager entityManager(componentFactory);

	const Entity testEntity = entityManager.addEntity();
	entityManager.addComponent<ComponentWithConstCharId>(testEntity);

	EXPECT_TRUE(entityManager.doesEntityHaveComponent<ComponentWithConstCharId>(testEntity));
}

TEST(ComponentIdTypes, ComponentSetHolderWithConstCharComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using ComponentSetHolder = RaccoonEcs::ComponentSetHolderImpl<const char*>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<const char*>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithConstCharId>();
	ComponentSetHolder componentSetHolder(componentFactory);

	componentSetHolder.addComponent<ComponentWithConstCharId>();

	EXPECT_NE(std::get<0>(componentSetHolder.getComponents<ComponentWithConstCharId>()), nullptr);
}

TEST(ComponentIdTypes, EntityManagerWithCustomStringComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using EntityManager = RaccoonEcs::EntityManagerImpl<CustomString>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<CustomString>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithCustomStringId>();
	EntityManager entityManager(componentFactory);

	const Entity testEntity = entityManager.addEntity();
	entityManager.addComponent<ComponentWithCustomStringId>(testEntity);

	EXPECT_TRUE(entityManager.doesEntityHaveComponent<ComponentWithCustomStringId>(testEntity));
}

TEST(ComponentIdTypes, ComponentSetHolderWithCustomStringComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using ComponentSetHolder = RaccoonEcs::ComponentSetHolderImpl<CustomString>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<CustomString>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithCustomStringId>();
	ComponentSetHolder componentSetHolder(componentFactory);

	componentSetHolder.addComponent<ComponentWithCustomStringId>();

	EXPECT_NE(std::get<0>(componentSetHolder.getComponents<ComponentWithCustomStringId>()), nullptr);
}

TEST(ComponentIdTypes, EntityManagerWithIntegerComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using EntityManager = RaccoonEcs::EntityManagerImpl<int>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<int>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithIntegerId>();
	EntityManager entityManager(componentFactory);

	const Entity testEntity = entityManager.addEntity();
	entityManager.addComponent<ComponentWithIntegerId>(testEntity);

	EXPECT_TRUE(entityManager.doesEntityHaveComponent<ComponentWithIntegerId>(testEntity));
}

TEST(ComponentIdTypes, ComponentSetHolderWithIntegerComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using ComponentSetHolder = RaccoonEcs::ComponentSetHolderImpl<int>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<int>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithIntegerId>();
	ComponentSetHolder componentSetHolder(componentFactory);

	componentSetHolder.addComponent<ComponentWithIntegerId>();

	EXPECT_NE(std::get<0>(componentSetHolder.getComponents<ComponentWithIntegerId>()), nullptr);
}

TEST(ComponentIdTypes, EntityManagerWithEnumComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using EntityManager = RaccoonEcs::EntityManagerImpl<EnumComponentId>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<EnumComponentId>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithEnumId>();
	EntityManager entityManager(componentFactory);

	const Entity testEntity = entityManager.addEntity();
	entityManager.addComponent<ComponentWithEnumId>(testEntity);

	EXPECT_TRUE(entityManager.doesEntityHaveComponent<ComponentWithEnumId>(testEntity));
}

TEST(ComponentIdTypes, ComponentSetHolderWithEnumComponentIdTypes_TryToCreateAndUse_CanBeCreatedAndUsed)
{
	using namespace TestComponentIdTypes_Internal;
	using ComponentSetHolder = RaccoonEcs::ComponentSetHolderImpl<EnumComponentId>;
	using ComponentFactory = RaccoonEcs::ComponentFactoryImpl<EnumComponentId>;

	ComponentFactory componentFactory;
	componentFactory.registerComponent<ComponentWithEnumId>();
	ComponentSetHolder componentSetHolder(componentFactory);

	componentSetHolder.addComponent<ComponentWithEnumId>();

	EXPECT_NE(std::get<0>(componentSetHolder.getComponents<ComponentWithEnumId>()), nullptr);
}
