#include <gtest/gtest.h>

#include "raccoon-ecs/error_handling.h"

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;
using ::testing::Environment;

class SGTestingEnvironment : public Environment
{
public:

	void SetUp() override;
	void TearDown() override;
};

class TestInfoLogger : public EmptyTestEventListener
{
	// Called before a test starts
	void OnTestStart(const TestInfo& test_info) override;
	// Called after a failed assertion or a SUCCEED() invocation
	void OnTestPartResult(const TestPartResult& test_part_result) override;
	// Called after a test ends
	void OnTestEnd(const TestInfo& test_info) override;
};

void SGTestingEnvironment::SetUp()
{
	RaccoonEcs::gErrorHandler = [](const std::string& error){ std::cerr << error << std::endl; GTEST_FAIL(); };
}

void SGTestingEnvironment::TearDown()
{
}

// Called before a test starts.
void TestInfoLogger::OnTestStart(const TestInfo& /*test_info*/)
{
//	std::cout << "======= Test " << test_info.test_case_name() << "." << test_info.name() << " starting." << std::endl;
}

// Called after a failed assertion or a SUCCEED() invocation.
void TestInfoLogger::OnTestPartResult(const TestPartResult& test_part_result)
{
	if (test_part_result.failed()) {
//		std::cerr << "======= " << (test_part_result.failed() ? " Failure in " : " Success in ")
//			<< test_part_result.file_name() << ":" << test_part_result.line_number() << " " << test_part_result.summary()
//			<< std::endl;
	}
}

// Called after a test ends.
void TestInfoLogger::OnTestEnd(const TestInfo& /*test_info*/)
{
//	std::cin << "======= Test " << test_info.test_case_name() << "." << test_info.name() << " is ending." << std::endl;
}

int main(int argc, char* argv[])
{
	InitGoogleTest(&argc, argv);

	AddGlobalTestEnvironment(new SGTestingEnvironment());

	TestEventListeners& listeners = UnitTest::GetInstance()->listeners();
	listeners.Append(new TestInfoLogger());

	int retVal = RUN_ALL_TESTS();

	return retVal;
}
