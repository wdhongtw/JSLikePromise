#include "gtest/gtest.h"

#include <coroutine>
#include <functional>
#include <iostream>
#include <deque>

#include "../JSLikePromise.hpp"
#include "TranscriptionCounter.hpp"

using namespace std;
using namespace JSLike;

namespace TestReadmeCodeSnippets
{
	//***************************************************************************************
	class ConsumerProducer : public testing::Test {
	protected:
		class DeepThought {
		public:
			DeepThought() = default;

			void cogitate(function<void(error_code errorCode, int theAnswer)> callback) {
				callback(error_code(0, generic_category()), 42);
			}
		};

		DeepThought deepThoughtAPI;

#include "../docs/producer.hpp"
	};

	namespace {
		TEST_F(ConsumerProducer, DeepThoughtCogitate)
		{
#include "../docs/consumer.hpp"
		}
	}
	//***************************************************************************************
	class CoroutineIntegration_co_await_resolved : public testing::Test {
	protected:
		Promise<int> taskThatReturnsAPromise() {
			co_return Promise<int>(1);
		}

		Promise<> coroutine1() {
			Promise<int> x = taskThatReturnsAPromise();
			int result = co_await x;  // Suspend here if x is not resolved.  Resume after x is resolved.
			cout << "result=" << result << "\n";
		}
	};
	namespace {
		TEST_F(CoroutineIntegration_co_await_resolved, DeepThoughtCogitate)
		{
			coroutine1();
		}
	}
	//***************************************************************************************
	class CoroutineIntegration_co_await_rejected : public testing::Test {
	protected:
		Promise<int> taskThatReturnsAPromise() {
			auto p = Promise<int>([](auto promiseState) {
				promiseState->reject(make_exception_ptr(out_of_range("invalid string position")));
				});
			co_return p;
		}

		Promise<> coroutine1() {
			Promise<int> x = taskThatReturnsAPromise();
			int result;
			try {
				result = co_await x;  // Resume and throw if x is rejected.
				cout << "result=" << result << "\n";
			}
			catch (exception& ex) {
				cout << "ex=" << ex.what() << "\n";
			}
		}
	};
	namespace {
		TEST_F(CoroutineIntegration_co_await_rejected, DeepThoughtCogitate)
		{
			coroutine1();
		}
	}
	//***************************************************************************************
	class CoroutineIntegration_ObtainResolvedValue : public testing::Test {
	protected:
		Promise<int> taskThatReturnsAnIntPromise() {
			co_return 1;
		}

		Promise<> coroutine1() {
			int& result = co_await taskThatReturnsAnIntPromise();
			cout << "result=" << result << "\n";
		}
	};
	namespace {
		TEST_F(CoroutineIntegration_ObtainResolvedValue, GetValueVia_Then)
		{
			taskThatReturnsAnIntPromise().Then([](int& result) {
				cout << "result=" << result << "\n";
				});
		}

		TEST_F(CoroutineIntegration_ObtainResolvedValue, GetValueVia_co_await)
		{
			coroutine1();
		}
	}
	//***************************************************************************************
	class CoroutineIntegration_ResolvedValueLifecycle : public testing::Test {
	protected:
		class MovableType {
		public:
			MovableType() = delete;
			MovableType(int i) : v(i) {}

			MovableType(const MovableType&) = delete;
			MovableType(MovableType&& other) noexcept { v = other.v; other.v = -1; }; // move constructor
			MovableType& operator=(const MovableType&) = delete;
			MovableType& operator=(MovableType&&) = delete;

			int internalValue() { return v; }
			int v;
		};

		class CopyableType {
		public:
			CopyableType() = delete;
			CopyableType(int i) : v(i) {}

			CopyableType(const CopyableType& other) { v = other.v; };  // copy constructor
			//CopyableType(CopyableType&& other) = delete;  // Don't delete this.  It breaks e.g. "co_return CopyableType(1);"
			CopyableType& operator=(const CopyableType&) = delete;
			CopyableType& operator=(CopyableType&&) = delete;

			int internalValue() { return v; }
			int v;
		};

		class ShareableFromThisType : public enable_shared_from_this<ShareableFromThisType> {
		public:
			ShareableFromThisType() = delete;
			ShareableFromThisType(int i) : v(i) {}

			ShareableFromThisType(const ShareableFromThisType&) = default;
			ShareableFromThisType(ShareableFromThisType&& other) = default;
			ShareableFromThisType& operator=(const ShareableFromThisType&) = default;
			ShareableFromThisType& operator=(ShareableFromThisType&&) = default;

			int internalValue() { return v; }
			int v;
		};

		Promise<MovableType> taskThatReturnsAMovableResultTypePromise() {
			co_return MovableType(1);
		}

		Promise<CopyableType> taskThatReturnsACopyableResultTypePromise() {
			co_return CopyableType(1);
		}

		Promise<ShareableFromThisType> taskThatReturnsAShareableResultTypePromise() {
			co_return ShareableFromThisType(1);
		}

		Promise<> coroutineThatMovesTheValue() {
			// Get the lvalue reference.
			MovableType& result = co_await taskThatReturnsAMovableResultTypePromise();

			// Move the result into a new instance of MovableType.
			MovableType rr(move(result));  // cast v to an rvalue to invoke the move constructor

			cout << "rr=" << rr.internalValue() << "\n";
		}

		Promise<> coroutineThatCopiesTheValue() {
			// Get the lvalue reference.
			CopyableType& result = co_await taskThatReturnsACopyableResultTypePromise();

			// Copy the result to a new instance of CopyableType.
			CopyableType rr(result);  // use v as an lavalue to invoke the copy constructor

			cout << "rr=" << rr.internalValue() << "\n";
		}

		Promise<> coroutineThatGetsASharedPointerToTheValue() {
			// Copy the lvalue refrence to the resolved value.
			ShareableFromThisType& result = co_await taskThatReturnsAShareableResultTypePromise();

			// Get a shared_ptr to the resolved value.
			shared_ptr<ShareableFromThisType> rr(result.shared_from_this());

			cout << "rr=" << rr->internalValue() << "\n";
		}

	};
	namespace {
		TEST_F(CoroutineIntegration_ResolvedValueLifecycle, Move)
		{
			coroutineThatMovesTheValue();
		}

		TEST_F(CoroutineIntegration_ResolvedValueLifecycle, MoveCaveat)
		{
			Promise<MovableType> p = taskThatReturnsAMovableResultTypePromise();
			p.Then([](MovableType& result) {
				// This Lambda will be called first.

				MovableType rr(move(result));  // cast v to an rvalue to invoke the move constructor
				cout << "rr=" << rr.internalValue() << "\n";

				}).Then([](MovableType& result) {
					// This Lambda will be called next.

					MovableType rr(move(result));  // BUG: v was moved already
					cout << "rr=" << rr.internalValue() << "\n";
					});
		}

		TEST_F(CoroutineIntegration_ResolvedValueLifecycle, Copy)
		{
			coroutineThatCopiesTheValue();
		}

		TEST_F(CoroutineIntegration_ResolvedValueLifecycle, Share)
		{
			coroutineThatGetsASharedPointerToTheValue();
		}

	}
	//***************************************************************************************
	class NittyGritty_coroutines : public testing::Test {
	protected:
		Promise<TranscriptionCounter> coReturnFromValue(TranscriptionCounter val) {
			co_return val;
		}

		Promise<TranscriptionCounter> coReturnFromReferenceByCopying(TranscriptionCounter& ref) {
			co_return ref;
		}

		Promise<TranscriptionCounter> coReturnFromReferenceByMoving(TranscriptionCounter& ref) {
			co_return move(ref);   // use move() to cast to an rvalue
		}

	};
	namespace {
		TEST_F(NittyGritty_coroutines, CallAndResolveWithReferenceByCopying)
		{
			// Construct a TranscriptionCounter
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);

			auto p = coReturnFromReferenceByCopying(*obj);

			EXPECT_EQ(0, nMoveCtor);
			EXPECT_EQ(0, nMoveAssign);
			EXPECT_EQ(1, nCopyCtor);   // (1) done by co_return
			EXPECT_EQ(0, nCopyAssign);
		}

		TEST_F(NittyGritty_coroutines, CallAndResolveWithReferenceByMoving)
		{
			// Construct a TranscriptionCounter
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);

			auto p = coReturnFromReferenceByMoving(*obj);

			EXPECT_EQ(1, nMoveCtor);   // (1) done by co_return
			EXPECT_EQ(0, nMoveAssign);
			EXPECT_EQ(0, nCopyCtor);
			EXPECT_EQ(0, nCopyAssign);
		}

		TEST_F(NittyGritty_coroutines, CallAndResolveWithValue)
		{
			// Construct a TranscriptionCounter
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);

			Promise<TranscriptionCounter> p = coReturnFromValue(*obj);

			EXPECT_EQ(2, nMoveCtor);    // (2) done to move parameter val into the coroutine's frame; (3) done by co_return
			EXPECT_EQ(0, nMoveAssign);
			EXPECT_EQ(1, nCopyCtor);    // (1) done to call coReturnFromValue() with val by value
			EXPECT_EQ(0, nCopyAssign);
		}
	}
	//***************************************************************************************
}
