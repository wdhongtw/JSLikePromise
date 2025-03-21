#include "gtest/gtest.h"

#include <string>
#include <coroutine>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>
#include <utility>  // for std::pair

#include "../JSLikePromise.hpp"
#include "../JSLikePromiseAny.hpp"

using namespace std;
using namespace JSLike;

namespace TestJSLikePromiseAny
{
	//***************************************************************************************
	namespace {
		TEST(AnyTestResolution, NonePreresolved_Catch_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto state)
					{
						EXPECT_EQ(1, state->value<int>());
						nThenCalls++;
					});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p1state->resolve(1);  // Resolve p1
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AnyTestResolution, NonePreresolved_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // resolved later
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // resolved later
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // resolved later

			bool areSomeResolved = false;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then([&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					areSomeResolved = true;
				});

			EXPECT_FALSE(areSomeResolved);
			p1state->resolve(1);
			EXPECT_TRUE(areSomeResolved);
		}

		TEST(AnyTestResolution, NonePreresolved_Then_Catch)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then([&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					nThenCalls++;
				})
				.Catch([&](auto ex) { nCatchCalls++; });

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p1state->resolve(1);  // Resolve p1
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AnyTestResolution, NonePreresolved_Then_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then([&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					nThenCalls++;
				}).Then([&](auto state)
					{
						EXPECT_EQ(1, state->value<int>());
						nThenCalls++;
					});

				EXPECT_EQ(0, nThenCalls);
				p1state->resolve(1);  // Resolve p1
				EXPECT_EQ(2, nThenCalls);
		}

		TEST(AnyTestResolution, NonePreresolved_ThenCatch)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then(
				[&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p1state->resolve(1);  // Resolve p1
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AnyTestResolution, NonePreresolved_ThenCatch_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then(
				[&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
					[&](auto state)
					{
						EXPECT_EQ(1, state->value<int>());
						nThenCalls++;
					});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p1state->resolve(1);  // Resolve p1
			EXPECT_EQ(2, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AnyTestResolution, SomePreresolved_Catch_Then2)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto state)
					{
						EXPECT_EQ(1, state->value<int>());
						nThenCalls++;
					});

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AnyTestResolution, SomePreresolved_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			bool areSomeResolved = false;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then([&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					areSomeResolved = true;
				});

			EXPECT_TRUE(areSomeResolved);
		}

		TEST(AnyTestResolution, SomePreresolved_Then_Catch)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then([&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					nThenCalls++;
				})
				.Catch([&](auto state) { nCatchCalls++; });

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AnyTestResolution, SomePreresolved_Then_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then([&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					nThenCalls++;
				}).Then([&](auto state)
					{
						EXPECT_EQ(1, state->value<int>());
						nThenCalls++;
					});

				EXPECT_EQ(2, nThenCalls);
		}

		TEST(AnyTestResolution, SomePreresolved_ThenCatch)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then(
				[&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					nThenCalls++;
				},
				[&](auto state) { nCatchCalls++; });

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(AnyTestResolution, SomePreresolved_ThenCatch_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then(
				[&](auto state)
				{
					EXPECT_EQ(1, state->value<int>());
					nThenCalls++;
				},
				[&](auto state) { nCatchCalls++; }).Then(
					[&](auto state)
					{
						EXPECT_EQ(1, state->value<int>());
						nThenCalls++;
					});

			EXPECT_EQ(2, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}
	}
	//***************************************************************************************
	namespace {
		TEST(AnyTestHierarchyOfPromiseAny, Test)
		{
			// Get 2 pre-resolved Promises, and 1 onresolved Promise.
			auto p1 = Promise<int>([](auto junk) {});  // won't resolve
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // will resolve later
			auto p3 = Promise<double>([](auto junk) {});  // won't resolve
			PromiseAny pa1 = PromiseAny({ p1, p2, p3 });

			auto p0 = BasePromise([](auto junk) {});  // won't resolve

			bool areAnyResolved = false;
			PromiseAny pa2({ pa1, p0 });
			pa2.Then([&](shared_ptr<BasePromiseState> result)
				{
					//// pa2 was resolved, because pa1 was resolved, becasue p2 was resolved.
					EXPECT_EQ(std::string("Hello"), result->value<string>());
					areAnyResolved = true;
				});

			EXPECT_FALSE(areAnyResolved);
			p2state->resolve("Hello");  // Resolve p2 --> resolves pa1 --> resolves pa2
			EXPECT_TRUE(areAnyResolved);
		}
	}
	//***************************************************************************************
	class AnyTest_co_await : public testing::Test {
	protected:
		Promise<bool> myCoAwaitingCoroutine(PromiseAny& p) {

			std::shared_ptr<BasePromiseState> result = co_await p;
			EXPECT_EQ(1, result->value<int>());

			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatCatches(PromiseAny& p) {
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
		TEST_F(AnyTest_co_await, Preresolved)
		{
			Promise<int> p1(1);
			Promise<string> p2("Hello");
			Promise<double> p3(3.3);
			PromiseAny pa({ p1, p2, p3 });  // Preresolved

			auto result = myCoAwaitingCoroutine(pa);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(AnyTest_co_await, ResolvedLater)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAny({ p0, p1, p2 });
			PromiseAny pa({ p0, p1, p2 });  // Not yet resolved

			auto result = myCoAwaitingCoroutine(pa);
			EXPECT_FALSE(result.isResolved());

			p0state->resolve(1);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(AnyTest_co_await, Reject_try_catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			PromiseAny pa({ p0, p1, p2 });

			auto result = myCoAwaitingCoroutineThatCatches(pa);

			EXPECT_FALSE(result.isResolved());

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_TRUE(result.isResolved());
			EXPECT_EQ(true, result.value());
		}

		TEST_F(AnyTest_co_await, Reject_uncaught)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			PromiseAny pa({ p0, p1, p2 });

			auto result = myCoAwaitingCoroutine(pa);

			EXPECT_FALSE(result.isResolved());
			EXPECT_FALSE(result.isRejected());

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_FALSE(result.isResolved());
			EXPECT_TRUE(result.isRejected());
		}
	}
	//***************************************************************************************
	class AnyTest_co_return : public testing::Test {
	protected:
		static PromiseAny CoReturnPromiseAny(PromiseAny& p) {
			co_return p;
		}

		static Promise<bool> CoAwait(PromiseAny& p) {
			co_await CoReturnPromiseAny(p);
			co_return true;
		}

		static PromiseAny CoroutineThatThrows() {
			char c = std::string().at(1); // this throws a std::out_of_range
			co_return PromiseAny(vector<BasePromise>{});
		}
	};

	namespace {
		TEST_F(AnyTest_co_return, Preresolved_Then)
		{
			auto p1 = Promise<int>(1);
			auto p2 = Promise<string>("Hello");
			auto p3 = Promise<double>(3.3);
			PromiseAny p({ p1, p2, p3 });


			bool wasThenCalled = false;
			CoReturnPromiseAny(p).Then([&](auto result)
				{
					EXPECT_EQ(1, result->value<int>());
					wasThenCalled = true;
				});

			EXPECT_TRUE(wasThenCalled);
		}

		TEST_F(AnyTest_co_return, Reject_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			PromiseAny p({ p0, p1, p2 });

			int nThenCalls = 0;
			int nCatchCalls = 0;
			bool wasExceptionThrown = false;
			PromiseAny pa = CoReturnPromiseAny(p);
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

		TEST_F(AnyTest_co_return, ResolvedLater_co_await)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			PromiseAny p({ p0, p1, p2 });

			auto result = CoAwait(p);

			EXPECT_FALSE(result.isResolved());
			p0state->resolve(1);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		};

		TEST_F(AnyTest_co_return, ResolvedLater_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			PromiseAny p({ p0, p1, p2 });

			bool wasThenCalled = false;
			CoReturnPromiseAny(p).Then([&](auto result)
				{
					EXPECT_EQ(1, result->value<int>());
					wasThenCalled = true;
				});

			p0state->resolve(1);
			EXPECT_TRUE(wasThenCalled);
		}

		TEST_F(AnyTest_co_return, throw_Catch)
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
		TEST(AnyTest_constructors, Assign)
		{
			PromiseAny pa1;
			PromiseAny pa2 = pa1;

			EXPECT_TRUE(pa1.state() == pa2.state());
		}

		TEST(AnyTest_constructors, Copy)
		{
			PromiseAny pa1;
			PromiseAny pa2(pa1);

			EXPECT_TRUE(pa1.state() == pa2.state());
		}

		TEST(AnyTest_constructors, Default)
		{
			PromiseAny pa;
			EXPECT_TRUE(pa.isRejected());
		}

		TEST(AnyTest_constructors, EmptyVector)
		{
			PromiseAny pa(vector<BasePromise>{});
			EXPECT_TRUE(pa.isResolved());
		}
	}
	//***************************************************************************************
	namespace {
		TEST(AnyTestRejection, Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			PromiseAny pa({ p0, p1, p2 });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_EQ(1, nCatchCalls);
		}

		TEST(AnyTestRejection, Catch_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			PromiseAny pa({ p0, p1, p2 });
			pa.Catch([&](auto ex) { nCatchCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(pa.isRejected());
			EXPECT_EQ(2, nCatchCalls);
		}

		TEST(AnyTestRejection, Catch_Then)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa({ p0, p1, p2 });
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

		TEST(AnyTestRejection, Then_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa({ p0, p1, p2 });
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

		TEST(AnyTestRejection, ThenCatch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa({ p0, p1, p2 });
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

		TEST(AnyTestRejection, ThenCatch_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa({ p0, p1, p2 });
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

		TEST(AnyTestRejection, ThenCatch_Then)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa({ p0, p1, p2 });
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


}
