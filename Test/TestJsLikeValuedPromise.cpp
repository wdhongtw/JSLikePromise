#include "gtest/gtest.h"

#include <coroutine>
#include <functional>
#include <iostream>
#include <deque>

#include "../JSLikePromise.hpp"
#include "TranscriptionCounter.hpp"

using namespace std;
using namespace JSLike;

namespace TestJSLikeValuedPromise
{
	//***************************************************************************************
	class ValuedTest_co_await : public testing::Test {
	protected:
		Promise<bool> myCoAwaitingCoroutine(Promise<int>& p) {

			auto result = co_await p;

			EXPECT_EQ(1, result);

			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatMoves(Promise<TranscriptionCounter>& p) {
			TranscriptionCounter r;
			r = move(co_await p);   // should invoke the move assignment operator

			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatCatches(Promise<int>& p) {
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
		TEST_F(ValuedTest_co_await, Prereject_uncaught)
		{
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();
			// Prereject p1.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			auto result = myCoAwaitingCoroutine(p1);
			EXPECT_FALSE(result.isResolved());
			EXPECT_TRUE(result.isRejected());
		}

		TEST_F(ValuedTest_co_await, Preresolved)
		{
			Promise<int> p1(1);

			auto result = myCoAwaitingCoroutine(p1);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(ValuedTest_co_await, Reject_try_catch)
		{
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutineThatCatches(p1);

			EXPECT_FALSE(result.isResolved());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_TRUE(result.isResolved());
			EXPECT_EQ(true, result.value());
		}

		TEST_F(ValuedTest_co_await, Reject_uncaught)
		{
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutine(p1);

			EXPECT_FALSE(result.isResolved());
			EXPECT_FALSE(result.isRejected());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_FALSE(result.isResolved());
			EXPECT_TRUE(result.isRejected());
		}

		TEST_F(ValuedTest_co_await, ResolvedLater)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutine(p0);

			EXPECT_FALSE(result.isResolved());
			p0state->resolve(1);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(ValuedTest_co_await, ResolvedLater_move)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutineThatMoves(p0);

			EXPECT_FALSE(result.isResolved());

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(move(*obj));  // Resolve

			EXPECT_EQ(0, nCopyCtor);
			EXPECT_EQ(0, nCopyAssign);
			EXPECT_EQ(1, nMoveCtor);    // performed by resolve()
			EXPECT_EQ(1, nMoveAssign);  // performed inside the coroutine


			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}
	}
	//***************************************************************************************
	class ValuedTest_co_return_Value : public testing::Test {
	protected:
		Promise<int> CoReturnPromise(int val) {
			co_return val;
		}

		Promise<bool> CoAwait(int val) {
			auto result = co_await CoReturnPromise(val);
			EXPECT_EQ(result, val);
			co_return true;
		}
	};
	namespace {
		TEST_F(ValuedTest_co_return_Value, Co_await)
		{
			auto result = CoAwait(1);

			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(ValuedTest_co_return_Value, Then)
		{
			bool wasThenCalled = false;
			CoReturnPromise(1).Then(
				[&](int& result)
				{
					EXPECT_EQ(1, result);
					wasThenCalled = true;
				});

			EXPECT_TRUE(wasThenCalled);
		}
	}
	//***************************************************************************************
	class ValuedTest_co_return_ValuedPromise : public testing::Test {
	protected:
		Promise<int> CoReturnPromise(Promise<int>& p) {
			co_return p;
		}

		Promise<bool> CoAwait(Promise<int>& p) {
			co_await CoReturnPromise(p);
			co_return true;
		}

		Promise<int> CoroutineThatThrows() {
			char c = std::string().at(1); // this throws a std::out_of_range
			co_return 1;
		}
	};
	namespace {
		TEST_F(ValuedTest_co_return_ValuedPromise, Preresolved_Then)
		{
			auto p1 = Promise<int>(1);

			bool wasThenCalled = false;
			CoReturnPromise(p1).Then(
				[&](int& result)
				{
					EXPECT_EQ(1, result);
					wasThenCalled = true;
				});

			EXPECT_TRUE(wasThenCalled);
		}

		TEST_F(ValuedTest_co_return_ValuedPromise, Reject_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			int nThenCalls = 0;
			int nCatchCalls = 0;
			bool wasExceptionThrown = false;
			Promise<int> pa = CoReturnPromise(p0);
			pa.Then([&](int& result) { nThenCalls++; });
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

			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			EXPECT_TRUE(pa.isRejected());
			EXPECT_FALSE(pa.isResolved());
			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(1, nCatchCalls);
			EXPECT_TRUE(wasExceptionThrown);
		}

		TEST_F(ValuedTest_co_return_ValuedPromise, ResolvedLater)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			bool wasThenCalled = false;
			auto p = CoReturnPromise(p0);

			EXPECT_FALSE(p.isResolved());
			p0state->resolve(1);
			EXPECT_TRUE(p.isResolved());
			EXPECT_EQ(1, p.value());
		}

		TEST_F(ValuedTest_co_return_ValuedPromise, ResolvedLater_co_await)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			auto result = CoAwait(p0);

			EXPECT_FALSE(result.isResolved());
			p0state->resolve(1);
			EXPECT_TRUE(result.isResolved());
			EXPECT_TRUE(result.value() == true);
		}

		TEST_F(ValuedTest_co_return_ValuedPromise, ResolvedLater_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			bool wasThenCalled = false;
			CoReturnPromise(p0).Then([&](int& result)
				{
					EXPECT_EQ(1, result);
					wasThenCalled = true;
				});

			EXPECT_FALSE(wasThenCalled);
			p0state->resolve(1);
			EXPECT_TRUE(wasThenCalled);
		}

		TEST_F(ValuedTest_co_return_ValuedPromise, throw_Catch)
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
		TEST(ValuedTest_constructors, Assign)
		{
			Promise<int> pa1(1);
			Promise<int> pa2 = pa1;

			EXPECT_TRUE(pa1.state() == pa2.state());
		}

		TEST(ValuedTest_constructors, Copy)
		{
			Promise<int> pa1(1);
			Promise<int> pa2(pa1);

			EXPECT_TRUE(pa1.state() == pa2.state());
		}

		TEST(ValuedTest_constructors, InitializerThatResolves)
		{
			Promise<int> p0(
				[](auto state) {
					state->resolve(1);
				});
			EXPECT_TRUE(p0.isResolved());
			EXPECT_TRUE(p0.value() == 1);
		}

		TEST(ValuedTest_constructors, InitializerThatRejects)
		{
			Promise<int> p0(
				[](auto state) {
					state->reject(make_exception_ptr(out_of_range("invalid string position")));
				});
			EXPECT_TRUE(p0.isRejected());
		}

		TEST(ValuedTest_constructors, InitializerThatThrows)
		{
			Promise<int> p0(
				[](auto state) {
					int i = std::string().at(1); // this generates an std::out_of_range
				});
			EXPECT_TRUE(p0.isRejected());
		}

		TEST(ValuedTest_constructors, WithValue)
		{
			Promise<int> p(1);
			EXPECT_TRUE(p.isResolved());
			EXPECT_TRUE(p.value() == 1);
		}
	}
	//***************************************************************************************
	namespace {
		TEST(ValuedTestRejection, Catch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			p0.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_EQ(1, nCatchCalls);
		}

		TEST(ValuedTestRejection, Catch_Catch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			p0
				.Catch([&](auto ex) { nCatchCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			EXPECT_TRUE(p0.isRejected());
			EXPECT_EQ(2, nCatchCalls);
		}

		TEST(ValuedTestRejection, Catch_Then)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](bool& result) { nThenCalls++; });

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

		TEST(ValuedTestRejection, Then_Catch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then([&](bool& result) { nThenCalls++; })
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

		TEST(ValuedTestRejection, ThenCatch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&](bool& result) { nThenCalls++; },
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

		TEST(ValuedTestRejection, ThenCatch_Catch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&](bool& result) { nThenCalls++; },
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

		TEST(ValuedTestRejection, ThenCatch_Then)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&](bool& result) { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; })
				.Then(
					[&](bool& result) { nThenCalls++; });

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
		TEST(ValuedTestResolution, PostresolvedCopy_ThenCatchMove)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](TranscriptionCounter& result) {
					TranscriptionCounter r; // This should invoke the default constructor
					r = move(result);       // This should invoke the move assignment operator
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(*obj);             // This should invoke the copy assignment operator

			EXPECT_EQ(0, nMoveCtor);
			EXPECT_EQ(1, nMoveAssign);  // performed inside the Then Lambda above
			EXPECT_EQ(1, nCopyCtor);    // performed by resolve(T&)
			EXPECT_EQ(0, nCopyAssign);

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedCopy_ThenMove)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](TranscriptionCounter& result) {
					TranscriptionCounter r; // This should invoke the default constructor
					r = move(result);       // This should invoke the move assignment operator
					nThenCalls++;
				});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(*obj);  // This should invoke the move assignment operator

			EXPECT_EQ(0, nMoveCtor);
			EXPECT_EQ(1, nMoveAssign);  // performed inside the Then Lambda above
			EXPECT_EQ(1, nCopyCtor);    // performed by resolve(T&)
			EXPECT_EQ(0, nCopyAssign);

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedLiteral_Catch_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
					[&](int& result) {
						EXPECT_EQ(1, result);
						nThenCalls++;
					});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedLiteral_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedLiteral_Then_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				}).Catch(
					[&](auto ex) {
						nCatchCalls++;
					});

				EXPECT_EQ(0, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
				p0state->resolve(1);  // Resolve
				EXPECT_EQ(1, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedLiteral_Then_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				}).Then(
					[&](int& result) {
						EXPECT_EQ(1, result);
						nThenCalls++;
					});

				EXPECT_EQ(0, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
				p0state->resolve(1);  // Resolve
				EXPECT_EQ(2, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedLiteral_ThenCatch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedLiteral_ThenCatch_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				}).Then(
					[&](int& result) {
						EXPECT_EQ(1, result);
						nThenCalls++;
					});

				EXPECT_EQ(0, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
				p0state->resolve(1);  // Resolve
				EXPECT_EQ(2, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedMove_ThenCatchMove)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](TranscriptionCounter& result) {
					TranscriptionCounter r; // This should invoke the default constructor
					r = move(result);       // This should invoke the move assignment operator
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(move(*obj));      // This should invoke the move assignment operator

			EXPECT_EQ(1, nMoveCtor);    // performed by resolve(T&&)
			EXPECT_EQ(1, nMoveAssign);  // performed inside the Then Lambda above
			EXPECT_EQ(0, nCopyCtor);
			EXPECT_EQ(0, nCopyAssign);

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PostresolvedMove_ThenMove)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](TranscriptionCounter& result) {
					TranscriptionCounter r; // This should invoke the default constructor
					r = move(result);       // This should invoke the move assignment operator
					nThenCalls++;
				});

			EXPECT_EQ(0, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(move(*obj));  // This should invoke the move assignment operator

			EXPECT_EQ(1, nMoveCtor);    // performed by resolve(T&&)
			EXPECT_EQ(1, nMoveAssign);  // performed inside the Then Lambda above
			EXPECT_EQ(0, nCopyCtor);
			EXPECT_EQ(0, nCopyAssign);

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PreresolvedCopy_Then)
		{
			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			Promise<TranscriptionCounter> p1(*obj);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](TranscriptionCounter& result) {
					nThenCalls++;
				});

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);

			EXPECT_EQ(0, nMoveCtor);
			EXPECT_EQ(0, nMoveAssign);
			EXPECT_EQ(1, nCopyCtor);    // performed by Promise(T &)
			EXPECT_EQ(0, nCopyAssign);
		}

		TEST(ValuedTestResolution, PreresolvedLiteral_Catch_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
					[&](int& result) {
						EXPECT_EQ(1, result);
						nThenCalls++;
					});

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PreresolvedLiteral_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				});

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PreresolvedLiteral_Then_Catch)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				}).Catch(
					[&](auto ex) { nCatchCalls++; });

				EXPECT_EQ(1, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PreresolvedLiteral_Then_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				}).Then(
					[&](int& result) {
						EXPECT_EQ(1, result);
						nThenCalls++;
					});

				EXPECT_EQ(2, nThenCalls);
				EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PreresolvedLiteral_ThenCatch)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PreresolvedLiteral_ThenCatch_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int& result) {
					EXPECT_EQ(1, result);
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
					[&](int& result) {
						EXPECT_EQ(1, result);
						nThenCalls++;
					});

			EXPECT_EQ(2, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);
		}

		TEST(ValuedTestResolution, PreresolvedMove_Then)
		{
			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			Promise<TranscriptionCounter> p1(move(*obj));                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](TranscriptionCounter& result) {
					nThenCalls++;
				});

			EXPECT_EQ(1, nThenCalls);
			EXPECT_EQ(0, nCatchCalls);

			EXPECT_EQ(1, nMoveCtor);    // performed by Promise(T &&)
			EXPECT_EQ(0, nMoveAssign);
			EXPECT_EQ(0, nCopyCtor);
			EXPECT_EQ(0, nCopyAssign);
		}
		//***************************************************************************************
	}
}
