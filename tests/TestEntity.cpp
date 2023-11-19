#include <gtest/gtest.h>

#include "raccoon-ecs/entity.h"

TEST(Entity, Entity_CreateWithId_ExpectIdIsSet)
{
	RaccoonEcs::Entity entity(1);
	EXPECT_EQ(entity.getId(), RaccoonEcs::Entity::EntityId(1));
}

TEST(Entity, OptionalEntity_CreateWithId_ExpectIdIsSetAndValid)
{
	RaccoonEcs::OptionalEntity entity(1);
	EXPECT_TRUE(entity.isValid());
	EXPECT_EQ(entity.getId(), RaccoonEcs::Entity::EntityId(1));
}

TEST(Entity, OptionalEntity_CreateWithEntity_ExpectIdIsSetAndValid)
{
	RaccoonEcs::Entity entity(1);
	RaccoonEcs::OptionalEntity optionalEntity(entity);
	EXPECT_TRUE(optionalEntity.isValid());
	EXPECT_EQ(optionalEntity.getId(), RaccoonEcs::Entity::EntityId(1));
}

TEST(Entity, Entity_ConvertToOptionalEntity_PreservesId)
{
	RaccoonEcs::Entity entity1(1);
	RaccoonEcs::OptionalEntity entity2 = entity1;
	EXPECT_TRUE(entity2.isValid());
	EXPECT_EQ(entity2.getId(), RaccoonEcs::Entity::EntityId(1));
}

TEST(Entity, OptionalEntity_CreateDefault_ExpectInvalid)
{
	RaccoonEcs::OptionalEntity entity;
	EXPECT_FALSE(entity.isValid());
}

TEST(Entity, TwoEntitiesWithDifferentId_Compare_NotEqual)
{
	RaccoonEcs::Entity entity1(1);
	RaccoonEcs::Entity entity2(2);
	EXPECT_NE(entity1, entity2);
	EXPECT_TRUE(entity1 < entity2);
	EXPECT_FALSE(entity2 < entity1);
	EXPECT_FALSE(entity1 == entity2);
}

TEST(Entity, OneEntityAndOneOptionalEntityWithSameId_Compare_Equal)
{
	RaccoonEcs::Entity entity1(1);
	RaccoonEcs::OptionalEntity entity2(1);
	EXPECT_EQ(entity1, entity2);
	EXPECT_FALSE(entity1 != entity2);
}

TEST(Entity, OneEntityAndOneOptionalEntityWithDifferentId_Compare_NotEqual)
{
	RaccoonEcs::Entity entity1(1);
	RaccoonEcs::OptionalEntity entity2(2);
	EXPECT_NE(entity1, entity2);
	EXPECT_FALSE(entity1 == entity2);
}

TEST(Entity, OneEntityAndOneInvalidOptionalEntity_Compare_NotEqual)
{
	RaccoonEcs::Entity entity1(1);
	RaccoonEcs::OptionalEntity entity2;
	EXPECT_NE(entity1, entity2);
	EXPECT_FALSE(entity1 == entity2);
}
