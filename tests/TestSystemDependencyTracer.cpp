#include <gtest/gtest.h>

#include <functional>
#include <vector>
#include <span>

#include "raccoon-ecs/async_systems_manager.h"

namespace TestSystemDependencyTracerInternal
{
	void expectSystemsToRunEq(std::vector<size_t>&& expectedSystems, const RaccoonEcs::SystemDependencyTracer& tracer)
	{
		std::vector<size_t> systemsToRun = tracer.getNextSystemsToRun();
		ASSERT_EQ(expectedSystems.size(), systemsToRun.size());
		std::sort(systemsToRun.begin(), systemsToRun.end());
		std::sort(expectedSystems.begin(), expectedSystems.end());

		for (size_t i = 0; i < expectedSystems.size(); ++i)
		{
			EXPECT_EQ(expectedSystems[i], systemsToRun[i]);
		}
	}
}

TEST(SystemDependencyTracer, TwoSystemsIndependent)
{
	using namespace TestSystemDependencyTracerInternal;

	RaccoonEcs::DependencyGraph dependencies;

	dependencies.initNodes(2);
	dependencies.finalize();
	RaccoonEcs::SystemDependencyTracer tracer(dependencies);

	expectSystemsToRunEq({0, 1}, tracer);

	tracer.runSystem(1);
	expectSystemsToRunEq({0}, tracer);
	tracer.finishSystem(1);

	expectSystemsToRunEq({0}, tracer);

	tracer.runSystem(0);
	expectSystemsToRunEq({}, tracer);
	tracer.finishSystem(0);

	expectSystemsToRunEq({}, tracer);
}

TEST(SystemDependencyTracer, TwoSystemsChain)
{
	using namespace TestSystemDependencyTracerInternal;

	RaccoonEcs::DependencyGraph dependencies;

	dependencies.initNodes(2);
	dependencies.addDependency(0, 1);
	dependencies.finalize();
	RaccoonEcs::SystemDependencyTracer tracer(dependencies);

	expectSystemsToRunEq({0}, tracer);

	tracer.runSystem(0);
	expectSystemsToRunEq({}, tracer);
	tracer.finishSystem(0);

	expectSystemsToRunEq({1}, tracer);

	tracer.runSystem(1);
	expectSystemsToRunEq({}, tracer);
	tracer.finishSystem(1);

	expectSystemsToRunEq({}, tracer);
}

TEST(SystemDependencyTracer, TwoSystemsIndependentRunInParallel)
{
	using namespace TestSystemDependencyTracerInternal;

	RaccoonEcs::DependencyGraph dependencies;

	dependencies.initNodes(2);
	dependencies.finalize();
	RaccoonEcs::SystemDependencyTracer tracer(dependencies);

	tracer.runSystem(1);
	tracer.runSystem(0);
	expectSystemsToRunEq({}, tracer);
	tracer.finishSystem(1);
	tracer.finishSystem(0);

	expectSystemsToRunEq({}, tracer);
}

TEST(SystemDependencyTracer, FourSystemsTwoParallelChains)
{
	using namespace TestSystemDependencyTracerInternal;

	RaccoonEcs::DependencyGraph dependencies;

	dependencies.initNodes(4);
	dependencies.addDependency(0, 1);
	dependencies.addDependency(2, 3);
	dependencies.finalize();
	RaccoonEcs::SystemDependencyTracer tracer(dependencies);

	expectSystemsToRunEq({0, 2}, tracer);

	tracer.runSystem(2);
	expectSystemsToRunEq({0}, tracer);
	tracer.runSystem(0);
	expectSystemsToRunEq({}, tracer);
	tracer.finishSystem(2);
	expectSystemsToRunEq({3}, tracer);
	tracer.finishSystem(0);
	expectSystemsToRunEq({1, 3}, tracer);

	tracer.runSystem(3);
	expectSystemsToRunEq({1}, tracer);
	tracer.finishSystem(3);
	expectSystemsToRunEq({1}, tracer);
}
