#include "gtest/gtest.h"

#include <coroutine>
#include <functional>
#include <iostream>
#include <deque>

#include "../JSLikePromise.hpp"

using namespace std;
using namespace JSLike;

namespace TestJSLikeVoidPromise
{
	//***************************************************************************************
	class VoidTest_co_await : public testing::Test {
	protected:
		Promise<bool> myCoAwaitingCoroutine(Promise<>& p) {
			co_await p;
			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatCatches(Promise<>& p) {
			try {
				co_await p;
			}
			catch (exception ex) {
				co_return true;
			}
			co_return false;
		}

	};
	namespace {
		TEST_F(VoidTest_co_await, Prereject_uncaught)
		{
			auto [p1, p1state] = Promise<>::getUnresolvedPromiseAndState();
			// Prereject p1.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			auto result = myCoAwaitingCoroutine(p1);
			EXPECT_FALSE(result.isResolved());
			EXPECT_TRUE(result.isRejected());
		}

		TEST_F(VoidTest_co_await, Preresolved)
		{
			Promise<> p1;

			auto result = myCoAwaitingCoroutine(p1);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(VoidTest_co_await, Reject_try_catch)
		{
			auto [p1, p1state] = Promise<>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutineThatCatches(p1);

			EXPECT_FALSE(result.isResolved());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_TRUE(result.isResolved());
			EXPECT_EQ(true, result.value());
		}

		TEST_F(VoidTest_co_await, Reject_uncaught)
		{
			auto [p1, p1state] = Promise<>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutine(p1);

			EXPECT_FALSE(result.isResolved());
			EXPECT_FALSE(result.isRejected());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_FALSE(result.isResolved());
			EXPECT_TRUE(result.isRejected());
		}

		TEST_F(VoidTest_co_await, ResolvedLater)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutine(p0);

			EXPECT_FALSE(result.isResolved());
			p0state->resolve();
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}
	}
	//***************************************************************************************
	class VoidTest_co_return_Explicit : public testing::Test {
	protected:
		Promise<> CoReturnPromise() {
			co_return;
		}

		Promise<bool> CoAwait() {
			co_await CoReturnPromise();
			co_return true;
		}

		Promise<> CoroutineThatThrows() {
			char c = std::string().at(1); // this throws a std::out_of_range
			co_return;
		}
	};
	namespace {
		TEST_F(VoidTest_co_return_Explicit, Co_await)
		{
			auto result = CoAwait();

			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(VoidTest_co_return_Explicit, Then)
		{
			bool wasThenCalled = false;
			CoReturnPromise().Then(
				[&]()
				{
					wasThenCalled = true;
				});

			EXPECT_TRUE(wasThenCalled);
		}

		TEST_F(VoidTest_co_return_Explicit, throw_Catch)
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
	class VoidTest_co_return_Implicit : public testing::Test {
	protected:
		Promise<> CoReturnPromise() {
			co_await suspend_never{};
			// Implicit co_return
		}

		Promise<bool> CoAwait() {
			co_await CoReturnPromise();
			co_return true;
		}
	};
	namespace {
		TEST_F(VoidTest_co_return_Implicit, Co_await)
		{
			auto result = CoAwait();

			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(VoidTest_co_return_Implicit, Test)
		{
			auto p = CoReturnPromise();
			EXPECT_TRUE(p.isResolved());
		}

		TEST_F(VoidTest_co_return_Implicit, Then)
		{
			bool wasThenCalled = false;
			CoReturnPromise().Then(
				[&]()
				{
					wasThenCalled = true;
				});

			EXPECT_TRUE(wasThenCalled);
		}
	}
	//***************************************************************************************
	namespace {
		TEST(VoidTest_constructors, Assign)
		{
			Promise<> pa1;
			Promise<> pa2 = pa1;

			EXPECT_TRUE(pa1.state() == pa2.state());
		}

		TEST(VoidTest_constructors, Copy)
		{
			Promise<> pa1;
			Promise<> pa2(pa1);

			EXPECT_TRUE(pa1.state() == pa2.state());
		}

		TEST(VoidTest_constructors, Default)
		{
			Promise<> p;
			EXPECT_TRUE(p.isResolved());
		}

		TEST(VoidTest_constructors, InitializerThatResolves)
		{
			Promise<> p0(
				[](auto state) {
					state->resolve();
				});
			EXPECT_TRUE(p0.isResolved());
		}

		TEST(VoidTest_constructors, InitializerThatRejects)
		{
			Promise<> p0(
				[](auto state) {
					state->reject(make_exception_ptr(out_of_range("invalid string position")));
				});
			EXPECT_TRUE(p0.isRejected());
		}

		TEST(VoidTest_constructors, InitializerThatThrows)
		{
			Promise<> p0(
				[](auto state) {
					int i = std::string().at(1); // this generates an std::out_of_range
				});
			EXPECT_TRUE(p0.isRejected());
		}
	}
	//***************************************************************************************
	namespace {
		TEST(VoidTestRejection, Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			bool wasExpectedExceptionThrown = false;
			p0.Catch(
				[&](auto ex) {
					if (!ex) FAIL();

					try {
						std::rethrow_exception(ex);
					}
					catch (std::exception& e) {
						if (e.what() == string("invalid string position"))
							wasExpectedExceptionThrown = true;
					}

					nCatchCalls++;
				});

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_EQ(1, nCatchCalls);
			EXPECT_TRUE(wasExpectedExceptionThrown);
		}

		TEST(VoidTestRejection, Catch_Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			bool wasExpectedException1Thrown = false;
			bool wasExpectedException2Thrown = false;
			p0
				.Catch(
					[&](auto ex) {
						if (!ex) FAIL();

						try {
							std::rethrow_exception(ex);
						}
						catch (std::exception& e) {
							if (e.what() == string("invalid string position"))
								wasExpectedException1Thrown = true;
						}

						nCatchCalls++;
					})
				.Catch(
					[&](auto ex) {
						if (!ex) FAIL();

						try {
							std::rethrow_exception(ex);
						}
						catch (std::exception& e) {
							if (e.what() == string("invalid string position"))
								wasExpectedException2Thrown = true;
						}

						nCatchCalls++;
					});

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_EQ(2, nCatchCalls);
			EXPECT_TRUE(wasExpectedException1Thrown);
			EXPECT_TRUE(wasExpectedException2Thrown);
		}

		TEST(VoidTestRejection, Catch_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			bool wasExpectedExceptionThrown = false;
			p0
				.Catch(
					[&](auto ex) {
						if (!ex) FAIL();

						try {
							std::rethrow_exception(ex);
						}
						catch (std::exception& e) {
							if (e.what() == string("invalid string position"))
								wasExpectedExceptionThrown = true;
						}

						nCatchCalls++;
					})
				.Then([&]() { nThenCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_FALSE(p0.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
			EXPECT_TRUE(wasExpectedExceptionThrown);
		}

		TEST(VoidTestRejection, Then_Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then([&]() { nThenCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_FALSE(p0.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
		}

		TEST(VoidTestRejection, ThenCatch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&]() { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_FALSE(p0.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
		}

		TEST(VoidTestRejection, ThenCatch_Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&]() { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; })
				.Catch(
					[&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_FALSE(p0.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(2, nCatchCalls);
		}

		TEST(VoidTestRejection, ThenCatch_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&]() { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; })
				.Then(
					[&]() { nThenCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_FALSE(p0.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
		}
	}
	//***************************************************************************************
	namespace {
		TEST(VoidTestResolution, Preresolved_Catch_Then)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
					[&]() {
						nThenCalls++;
					});

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Preresolved_Then)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				});

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Preresolved_Then_Catch)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				}).Catch(
					[&](auto ex) { nCatchCalls++; });

				EXPECT_EQ(1, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Preresolved_Then_Then)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				}).Then(
					[&]() {
						nThenCalls++;
					});

				EXPECT_EQ(2, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Preresolved_ThenCatch)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Preresolved_ThenCatch_Then)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
					[&]() {
						nThenCalls++;
					});

			EXPECT_EQ(2, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Unresolved_Catch_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
					[&]() {
						nThenCalls++;
					});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve();  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Unresolved_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve();  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Unresolved_Then_Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				}).Catch(
					[&](auto ex) {
						nCatchCalls++;
					});

				EXPECT_EQ(0, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
				p0state->resolve();  // Resolve
				EXPECT_EQ(1, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Unresolved_Then_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				}).Then(
					[&]() {
						nThenCalls++;
					});

				EXPECT_EQ(0, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
				p0state->resolve();  // Resolve
				EXPECT_EQ(2, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Unresolved_ThenCatch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve();  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(VoidTestResolution, Unresolved_ThenCatch_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				}).Then(
					[&]() {
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
}
