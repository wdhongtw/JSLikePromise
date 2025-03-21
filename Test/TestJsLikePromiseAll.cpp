#include "gtest/gtest.h"

#include <string>
#include <coroutine>
#include <functional>
#include <iostream>
#include <vector>
#include <utility>  // for std::pair

#include "../JSLikePromise.hpp"
#include "../JSLikePromiseAll.hpp"

using namespace std;
using namespace JSLike;

namespace TestJSLikePromiseAll
{
	//***************************************************************************************
	class AllTest_co_await : public testing::Test {
	protected:
		Promise<bool> myCoAwaitingCoroutine(PromiseAll& p) {

			auto result = co_await p;

			EXPECT_EQ(1, result[0]->value<int>());
			EXPECT_EQ(string("Hello"), result[1]->value<string>());
			EXPECT_EQ(3.3, result[2]->value<double>());

			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatCatches(PromiseAll& p) {
			try {
				auto result = co_await p;
			}
			catch (exception ex) {
				co_return true;
			}
			co_return false;
		}
	};

	namespace {
		TEST_F(AllTest_co_await, Preresolved)
		{
			Promise<int> p1(1);
			Promise<string> p2("Hello");
			Promise<double> p3(3.3);
			PromiseAll pa = PromiseAll({ p1, p2, p3 });  // Preresolved

			auto result = myCoAwaitingCoroutine(pa);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(AllTest_co_await, ResolvedLater)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto pa = PromiseAll({ p0, p1, p2 });

			auto result = myCoAwaitingCoroutine(pa);

			EXPECT_FALSE(result.isResolved());
			p0state->resolve(1);
			EXPECT_FALSE(result.isResolved());
			p1state->resolve("Hello");
			EXPECT_FALSE(result.isResolved());
			p2state->resolve(3.3);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(AllTest_co_await, Reject_try_catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto pa = PromiseAll({ p0, p1, p2 });

			auto result = myCoAwaitingCoroutineThatCatches(pa);

			EXPECT_FALSE(result.isResolved());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_TRUE(result.isResolved());
			EXPECT_EQ(true, result.value());
		}

		TEST_F(AllTest_co_await, Reject_uncaught)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto pa = PromiseAll({ p0, p1, p2 });

			auto result = myCoAwaitingCoroutine(pa);

			EXPECT_FALSE(result.isResolved());
			EXPECT_FALSE(result.isRejected());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_FALSE(result.isResolved());
			EXPECT_TRUE(result.isRejected());
		}
	}
	//***************************************************************************************
	class AllTest_co_return : public testing::Test {
	protected:
		PromiseAll CoReturnPromiseAll(PromiseAll& p) {
			co_return p;
		}

		Promise<bool> CoAwait(PromiseAll& p) {
			co_await CoReturnPromiseAll(p);
			co_return true;
		}

		PromiseAll CoroutineThatThrows() {
			char c = std::string().at(1); // this throws a std::out_of_range
			co_return PromiseAll(vector<BasePromise>{});
		}
	};

	namespace {
		TEST_F(AllTest_co_return, Preresolved_Then)
		{
			auto p1 = Promise<int>(1);
			auto p2 = Promise<string>("Hello");
			auto p3 = Promise<double>(3.3);
			auto p = PromiseAll({ p1, p2, p3 });


			bool wasThenCalled = false;
			CoReturnPromiseAll(p).Then([&](auto result)
				{
					EXPECT_EQ(1, result[0]->value<int>());
					EXPECT_EQ(string("Hello"), result[1]->value<string>());
					EXPECT_EQ(3.3, result[2]->value<double>());
					wasThenCalled = true;
				});

			EXPECT_TRUE(wasThenCalled);
		}

		TEST_F(AllTest_co_return, ResolvedLater_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAll({ p0, p1, p2 });

			bool wasThenCalled = false;
			CoReturnPromiseAll(p).Then([&](auto result)
				{
					EXPECT_EQ(1, result[0]->value<int>());
					EXPECT_EQ(string("Hello"), result[1]->value<string>());
					EXPECT_EQ(3.3, result[2]->value<double>());
					wasThenCalled = true;
				});

			EXPECT_FALSE(wasThenCalled);
			p0state->resolve(1);
			EXPECT_FALSE(wasThenCalled);
			p1state->resolve("Hello");
			EXPECT_FALSE(wasThenCalled);
			p2state->resolve(3.3);
			EXPECT_TRUE(wasThenCalled);
		}

		TEST_F(AllTest_co_return, Reject_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAll({ p0, p1, p2 });

			int nThenCalls = 0;
			int nCatchCalls = 0;
			bool wasExceptionThrown = false;
			PromiseAll pa = CoReturnPromiseAll(p);
			pa.Then([&](auto result) { nThenCalls++; });
			pa.Catch([&](auto ex) {
				if (!ex) FAIL();

				try {
					std::rethrow_exception(ex);
				}
				catch (std::exception& e) {
					if (e.what() == string("invalid string position"))
						wasExceptionThrown = true;
				}

				nCatchCalls++;
				});

			EXPECT_FALSE(pa.isRejected());

			// Resolve 1 Promises and reject 2.
			p0state->resolve(1);
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			p2state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p2 again to verify that Catch isn't called multiple times.
			p2state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_FALSE(pa.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
			EXPECT_TRUE(wasExceptionThrown);
		}

		TEST_F(AllTest_co_return, ResolvedLater_co_await)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAll({ p0, p1, p2 });


			auto result = CoAwait(p);

			EXPECT_FALSE(result.isResolved());
			p0state->resolve(1);
			EXPECT_FALSE(result.isResolved());
			p1state->resolve("Hello");
			EXPECT_FALSE(result.isResolved());
			p2state->resolve(3.3);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(AllTest_co_return, throw_Catch)
		{
			bool wasExceptionThrown = false;

			CoroutineThatThrows().Catch([&](std::exception_ptr eptr)
				{
					if (!eptr) FAIL();

					try {
						std::rethrow_exception(eptr);
					}
					catch (std::exception& e) {
						if (e.what() == string("invalid string position"))
							wasExceptionThrown = true;
					}
				});

			EXPECT_TRUE(wasExceptionThrown);
		}
	}
	//***************************************************************************************
	namespace {
		TEST(AllTest_constructors, Assign)
		{
			PromiseAll pa1;
			PromiseAll pa2 = pa1;

			EXPECT_TRUE(pa1.state() == pa2.state());
		}

		TEST(AllTest_constructors, Copy)
		{
			PromiseAll pa1;
			PromiseAll pa2(pa1);

			EXPECT_TRUE(pa1.state() == pa2.state());
		}

		TEST(AllTest_constructors, Default)
		{
			PromiseAll pa;
			EXPECT_TRUE(pa.isResolved());
		}

		TEST(AllTest_constructors, EmptyVector)
		{
			PromiseAll pa(vector<BasePromise>{});
			EXPECT_TRUE(pa.isResolved());
		}
	}
	//***************************************************************************************
	namespace {
		TEST(AllTestResolution, Preresolved_Catch_Then)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ Promise<>(), p1, p2, p3 });
			pa
				.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto states)
					{
						EXPECT_EQ(1, states[1]->value<int>());
						EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
						EXPECT_EQ(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AllTestResolution, Preresolved_Then)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			bool areAllResolved = false;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then([&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					areAllResolved = true;
				});

			EXPECT_TRUE(areAllResolved);
		}

		TEST(AllTestResolution, Preresolved_Then_Catch)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then([&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					nThenCalls++;
				})
				.Catch([&](auto ex) { nCatchCalls++; });

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AllTestResolution, Preresolved_Then_Then)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then([&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					nThenCalls++;
				})
				.Then([&](auto states)
					{
						EXPECT_EQ(1, states[1]->value<int>());
						EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
						EXPECT_EQ(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			EXPECT_EQ(2, nThenCalls);
		}

		TEST(AllTestResolution, Preresolved_ThenCatch)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then(
				[&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AllTestResolution, Preresolved_ThenCatch_Then)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then(
				[&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
					[&](auto states)
					{
						EXPECT_EQ(1, states[1]->value<int>());
						EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
						EXPECT_EQ(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			EXPECT_EQ(2, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AllTestResolution, SomePreresolved_Catch_Then)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto states)
					{
						EXPECT_EQ(1, states[1]->value<int>());
						EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
						EXPECT_EQ(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve();  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AllTestResolution, SomePreresolved_Then)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			bool areAllResolved = false;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then([&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					areAllResolved = true;
				});

			EXPECT_FALSE(areAllResolved);
			p0state->resolve();  // Resolve
			EXPECT_TRUE(areAllResolved);
		}

		TEST(AllTestResolution, SomePreresolved_Then_Catch)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then([&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					nThenCalls++;
				})
				.Catch([&](auto ex) { nCatchCalls++; });

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve();  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AllTestResolution, SomePreresolved_Then_Then)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then([&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					nThenCalls++;
				})
				.Then([&](auto states)
					{
						EXPECT_EQ(1, states[1]->value<int>());
						EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
						EXPECT_EQ(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			EXPECT_EQ(0, nThenCalls);
			p0state->resolve();  // Resolve
			EXPECT_EQ(2, nThenCalls);
		}

		TEST(AllTestResolution, SomePreresolved_ThenCatch)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then(
				[&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve();  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AllTestResolution, SomePreresolved_ThenCatch_Then)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then(
				[&](auto states)
				{
					EXPECT_EQ(1, states[1]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
					EXPECT_EQ(3.3, states[3]->value<double>());
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
					[&](auto states)
					{
						EXPECT_EQ(1, states[1]->value<int>());
						EXPECT_EQ(std::string("Hello"), states[2]->value<std::string>());
						EXPECT_EQ(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve();  // Resolve
			EXPECT_EQ(2, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}
	}
	//***************************************************************************************
	namespace {
		TEST(AllTestRejection, Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_EQ(1, nCatchCalls);
		}

		TEST(AllTestRejection, Catch_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Catch([&](auto ex) { nCatchCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_EQ(2, nCatchCalls);
		}

		TEST(AllTestRejection, Catch_Then)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto result) { nThenCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p1 again to verify that Catch isn't called multiple times.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_FALSE(pa.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
		}

		TEST(AllTestRejection, Then_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Then([&](auto result) { nThenCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p1 again to verify that Catch isn't called multiple times.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_FALSE(pa.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
		}

		TEST(AllTestRejection, ThenCatch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Then(
				[&](auto result) { nThenCalls++; },
				[&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p1 again to verify that Catch isn't called multiple times.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_FALSE(pa.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
		}

		TEST(AllTestRejection, ThenCatch_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Then(
				[&](auto result) { nThenCalls++; },
				[&](auto ex) { nCatchCalls++; }).Catch(
					[&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p1 again to verify that Catch isn't called multiple times.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_FALSE(pa.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(2, nCatchCalls);
		}

		TEST(AllTestRejection, ThenCatch_Then)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Then(
				[&](auto result) { nThenCalls++; },
				[&](auto ex) { nCatchCalls++; }).Then(
					[&](auto result) { nThenCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p1 again to verify that Catch isn't called multiple times.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_FALSE(pa.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
		}
	}
	//***************************************************************************************
	namespace {
		TEST(AllTestHierarchicalPromiseAll, Test)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();    // resolve later
			Promise<int> p1(1);                                                // preresolved
			Promise<string> p2("Hello");                                       // preresolved
			Promise<double> p3(3.3);                                           // preresolved

			bool areAllResolved = false;

			// Top of the hierarchy (should be preresolved)
			PromiseAll pa1({ p1, p2, p3 });
			pa1.Then([&](auto states)
				{
					EXPECT_EQ(1, states[0]->value<int>());
					EXPECT_EQ(std::string("Hello"), states[1]->value<std::string>());
					EXPECT_EQ(3.3, states[2]->value<double>());
				});

			// Bottom of the hierarchy (should resolve when p0 resolves)
			PromiseAll pa2({ pa1, p0 });
			pa2.Then([&](auto states)
				{
					areAllResolved = true;
				});

			EXPECT_FALSE(areAllResolved);
			p0state->resolve();  // Resolve
			EXPECT_TRUE(areAllResolved);
		}
	}
	//***************************************************************************************
};
