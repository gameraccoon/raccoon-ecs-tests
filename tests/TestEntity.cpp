#include <gtest/gtest.h>

#include "raccoon-ecs/entity.h"

TEST(Entity, Entity_CreateWithRawIdAndVersion_ExpectIdAndVersionSet)
{
	RaccoonEcs::Entity entity = RaccoonEcs::Entity{1, 2};
	EXPECT_EQ(entity.getRawId(), RaccoonEcs::Entity::RawId(1));
	EXPECT_EQ(entity.getVersion(), RaccoonEcs::Entity::Version(2));
}

TEST(Entity, OptionalEntity_CreateWithId_ExpectIdIsSetAndValid)
{
	RaccoonEcs::OptionalEntity entity = RaccoonEcs::Entity{1, 0};
	EXPECT_TRUE(entity.isValid());
	EXPECT_EQ(entity.getRawId(), RaccoonEcs::Entity::RawId(1));
	EXPECT_EQ(entity.getVersion(), RaccoonEcs::Entity::Version(0));
}

TEST(Entity, OptionalEntity_CreateWithEntity_ExpectIdIsSetAndValid)
{
	RaccoonEcs::Entity entity = RaccoonEcs::Entity{1, 0};
	RaccoonEcs::OptionalEntity optionalEntity(entity);
	EXPECT_TRUE(optionalEntity.isValid());
	EXPECT_EQ(optionalEntity.getRawId(), RaccoonEcs::Entity::RawId(1));
	EXPECT_EQ(optionalEntity.getVersion(), RaccoonEcs::Entity::Version(0));
}

TEST(Entity, Entity_ConvertToOptionalEntity_PreservesId)
{
	RaccoonEcs::Entity entity1 = RaccoonEcs::Entity{1, 0};
	RaccoonEcs::OptionalEntity entity2 = entity1;
	EXPECT_TRUE(entity2.isValid());
	EXPECT_EQ(entity2.getRawId(), RaccoonEcs::Entity::RawId(1));
	EXPECT_EQ(entity2.getVersion(), RaccoonEcs::Entity::Version(0));
}

TEST(Entity, OptionalEntity_CreateDefault_ExpectInvalid)
{
	RaccoonEcs::OptionalEntity entity;
	EXPECT_FALSE(entity.isValid());
}

TEST(Entity, TwoEntitiesWithDifferentId_Compare_NotEqual)
{
	RaccoonEcs::Entity entity1 = RaccoonEcs::Entity{1, 0};
	RaccoonEcs::Entity entity2 = RaccoonEcs::Entity{2, 0};
	EXPECT_NE(entity1, entity2);
	EXPECT_TRUE(entity1 < entity2);
	EXPECT_FALSE(entity2 < entity1);
	EXPECT_FALSE(entity1 == entity2);
}

TEST(Entity, OneEntityAndOneOptionalEntityWithSameId_Compare_Equal)
{
	RaccoonEcs::Entity entity1 = RaccoonEcs::Entity{1, 0};
	RaccoonEcs::OptionalEntity entity2 = RaccoonEcs::Entity{1, 0};
	EXPECT_EQ(entity1, entity2);
	EXPECT_FALSE(entity1 != entity2);
}

TEST(Entity, OneEntityAndOneOptionalEntityWithDifferentId_Compare_NotEqual)
{
	RaccoonEcs::Entity entity1 = RaccoonEcs::Entity{1, 0};
	RaccoonEcs::OptionalEntity entity2 = RaccoonEcs::Entity{2, 0};
	EXPECT_NE(entity1, entity2);
	EXPECT_FALSE(entity1 == entity2);
}

TEST(Entity, OneEntityAndOneInvalidOptionalEntity_Compare_NotEqual)
{
	RaccoonEcs::Entity entity1{1, 0};
	RaccoonEcs::OptionalEntity entity2;
	EXPECT_NE(entity1, entity2);
	EXPECT_FALSE(entity1 == entity2);
}
