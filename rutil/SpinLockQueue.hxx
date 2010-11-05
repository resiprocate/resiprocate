// Copyright (C) 2010 Dan Weber <dan@blinkmind.com>
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SPINLOCKQUEUE_HXX
#define SPINLOCKQUEUE_HXX 1
#include <cstddef>
#include <algorithm>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <cassert>
#include <vector>
#include <type_traits>
#include <iostream>
#include <chrono>
#define TEST_MAIN 0
#if (TEST_MAIN == 1)
#define HAS_BOOST 1
#endif
#ifdef HAS_BOOST
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
#endif
#define NO_PRODUCER_SPINLOCK 1
#define NO_CONSUMER_SPINLOCK 1
#define REUSE_NODES 0
namespace resip {
template <typename T, typename Enable = void>
class SpinLockQueue
{
public:
	typedef T  value_type;
	typedef T* pointer_type;
	typedef T const & param_type;
	typedef T & reference_type;

private:
        struct Node {
                Node( pointer_type val ) : value(val), next(0) { }
                Node( Node && rhs) {
                        *this = std::move(rhs);
                }
                Node( Node const &) = delete;
                Node& operator=(Node const &) = delete;
                Node& operator=(Node && rhs) {
                        if (this != &rhs) {
                                value = rhs.value;
                                rhs.value = 0;
                                next = rhs.next;
                                rhs.next = 0;
                        }

                        return *this;
                }

                ~Node() {
                        delete value;
                }
                pointer_type value;
#if 0 && (NO_PRODUCER_SPINLOCK == 1)
                std::atomic<Node*> next;
#else
                Node* next;
#endif
        } __attribute__((aligned(64)));



        std::mutex mMutex;
        std::condition_variable mCondition;

#if (NO_CONSUMER_SPINLOCK == 0)
        Node* first __attribute__((aligned(64)));

        // shared among consumers
        atomic<long> consumerLock __attribute__((aligned(64)));
        //char pad[ 64 - sizeof(consumerLock) - sizeof(first)];
        // for one producer at a time
#else
	std::atomic<Node*> first __attribute__((aligned(64)));
#endif
#if (NO_PRODUCER_SPINLOCK == 1)
        atomic<Node*> last __attribute__((aligned(64)));
#else
        Node *last __attribute__((aligned(32)));
#endif

        // shared among producers
#if (NO_PRODUCER_SPINLOCK == 0)
        atomic<long> producerLock __attribute__((aligned(32)));
#endif

#if (REUSE_NODES == 1)
        atomic<Node*> nodeList; // __attribute__((aligned(64)));

        Node* getNewNode(param_type t) {
                Node* n = nodeList;
                if (n) {
                        while (n && !nodeList.compare_exchange_strong(n,n->next)) {
                                n = nodeList;
                        }

                        if (n) {
                                n->value = new value_type(t);
                                n->next = 0;
                                return n;
                        } else {
                                return new Node(new value_type(t));
                        }

                } else {
                        return new Node(new value_type(t));
                }
        }

        Node* getNewNode(pointer_type t) {
                Node * n = nodeList;
                if (n) {
                        Node* next = n->next;
                        while (!nodeList.compare_exchange_strong(n,next)) {
                                n = nodeList;
                                if (n) {
                                        next = n->next;
                                }
                        }

                        if (n) {
                                n->value = t;
                                n->next = 0;
                                return n;
                        } else {
                                return new Node(n);
                        }
                } else {
                        return new Node(n);
                }
        }

        void addNodeToList(Node* node) {
                Node *n(0);
                do {
                        n = nodeList;
                        node->next = n;
                } while (!nodeList.compare_exchange_strong(n,node));

        }
#else
        Node* getNewNode(param_type t) {
                return new Node(new value_type(t));
        }
        Node* getNewNode(pointer_type t) {
                return new Node (t);
        }
        void addNodeToList(Node* node) {
                delete node;
        }
#endif

	std::atomic<std::size_t> mWaiters __attribute__((aligned(16)));
	std::atomic<ssize_t> mSize __attribute__((aligned(16)));



public:
        SpinLockQueue() : mWaiters(0) {
		mSize = 0;
#if (REUSE_NODES == 1)
                nodeList = new Node(0);
#endif
                first  = last = new Node(0);           // add dummy separator
#if (NO_PRODUCER_SPINLOCK == 0)
                producerLock = false;
#endif
#if (NO_CONSUMER_SPINLOCK == 0)

                consumerLock = false;
#endif
        }

	SpinLockQueue(const SpinLockQueue&) = delete;
	SpinLockQueue& operator=(const SpinLockQueue&) = delete;
        ~SpinLockQueue() {
                Node* tmp;
                while( (tmp = first) != 0 ) {   // release the list
                        first = tmp->next;
                        delete tmp;
                }

#if (REUSE_NODES == 1)
                while ( (tmp = nodeList) != 0) {
                        nodeList.store(tmp->next);
                        delete tmp;
                }
#endif
        }


        void push( param_type t ) {
                Node* newlast = getNewNode(t);
                assert(newlast);
                assert(newlast->value);

#if (NO_PRODUCER_SPINLOCK == 1)
                // what prevents oldLast from being deleted while I'm doing this compare and swap?
                while(true) {
                        Node *oldLast = last.load();
                        if (last.compare_exchange_strong(oldLast,newlast)) {
                                oldLast->next = newlast;
                                break;
                        } else {
                                oldLast = last.load();
                        }
                }

#else
                long oldvalue = false;
                while (!producerLock.compare_exchange_strong(oldvalue,true)) {
                        oldvalue = false;
                }
                assert(producerLock);
// own producer lock

                last->next = newlast;
                last = newlast;
                long exchange;
                exchange = producerLock.exchange(false);
                assert(exchange);
#endif
		++mSize;
                if (mWaiters) {
                        mCondition.notify_one();
                }
        }
#if (NO_CONSUMER_SPINLOCK == 1)
	bool try_pop( reference_type result )
	{
		while((Node*)first != (Node*)last)
		{
			Node * volatile f = first; // Required volatile because of f->next updates
			while(!first.compare_exchange_strong(const_cast<Node*&>(f),NULL))
			{
				f = first;
			}

			if (f == NULL)
			{
				continue; // Didn't acquire spin lock, rerun loops..
			}

			Node * next;
			while ((next = f->next)  == NULL) {} // Wait for producer to finish setting next

			while (next)
			{
				Node* fold = f;
				if (next->value == 0)
				{
				//	f->value = 0; // Is this really necessary?
					f = next;
					next = f->next;
					--mSize;
					addNodeToList(f);
				}
				else 
				{
					pointer_type val = next->value;
					next->value = 0;
					f = next;
					// no need to update next, but just in case
					next = 0;
					first.store(f); // reset first back to a real pointer again...
					--mSize;
					addNodeToList(fold);
					result = std::move(*val);
					delete val;
					return true;
				}
			}
			
			
		}
		
		return false;
	}
#else
        bool try_pop( reference_type result ) {
                // No nodes point to front
                long oldvalue = false;
                while (!consumerLock.compare_exchange_strong(oldvalue,true)) {
                        oldvalue = false;
                }


                while (true) {
                        Node * volatile theFirst = first;
                        Node * next;

                        while ((next = theFirst->next) == NULL) {}
                        if (next) {
                                if (next->value == 0) {

                                        first = next;
                                        theFirst->value = 0;
                                        addNodeToList(theFirst);
					--mSize;
                                        continue;
                                } else {
                                        pointer_type val = next->value;
                                        next->value = 0;
                                        first = next;
                                        theFirst->value = 0;
                                        addNodeToList(theFirst);
                                        long exchangedVal = consumerLock.exchange(false);
					--mSize;
                                        assert(exchangedVal);
                                        assert(val);
                                        result = std::move(*val);
                                        delete val;

                                }

                                return true;
                        }

                        consumerLock.exchange(false);
                        return false;
                }
        }
#endif

        void pop(reference_type result) {

                while (!try_pop(result)) {
			++mWaiters;
			--mSize; // Allow size to drop below zero
			std::unique_lock<std::mutex> lk(mMutex);
                        mCondition.wait(lk);
			++mSize; // Don't allow size to double decrement (try_pop and here)
			--mWaiters;
                }
        }

        bool pop(reference_type result, std::chrono::system_clock::duration wait)
        {
                using namespace std::chrono;
                system_clock::time_point pt = system_clock::now() + wait;
                while (!try_pop(result)) {
                        ++mWaiters;
                        --mSize; // Allow size to drop below zero
                        std::unique_lock<std::mutex> lk(mMutex);
                        if (mCondition.wait_until(lk,pt) == std::cv_status::timeout)
                        {
                                ++mSize;
                                --mWaiters;
                                return false;
                        }
                        ++mSize; // Don't allow size to double decrement (try_pop and here)
                        --mWaiters;
                }

                return true;
        }


        std::size_t size() const { return mSize; }
        bool empty() const { return mSize <= 0; }


};


template <typename T>
class SpinLockQueue<T, typename std::enable_if<std::is_pointer<T>::value >::type >
{
public:
	typedef T value_type;
	typedef T pointer_type;
	typedef T param_type;
	typedef T & reference_type;

private:
        struct Node {
                Node( pointer_type val ) : value(val), next(0) { }
                Node( Node && rhs) {
                        *this = std::move(rhs);
                }
                Node( Node const &) = delete;
                Node& operator=(Node const &) = delete;
                Node& operator=(Node && rhs) {
                        if (this != &rhs) {
                                value = rhs.value;
                                rhs.value = 0;
                                next = rhs.next;
                                rhs.next = 0;
                        }

                        return *this;
                }

                ~Node() {
		  // .dw. if passed a pointer, don't delete it because we may not own that object.
                    //    delete value;
                }
                pointer_type value;
#if 0 && (NO_PRODUCER_SPINLOCK == 1)
                std::atomic<Node*> next;
#else
                Node* next;
#endif
        } __attribute__((aligned(64)));



        std::mutex mMutex;
        std::condition_variable mCondition;

#if (NO_CONSUMER_SPINLOCK == 0)
        Node* first __attribute__((aligned(64)));

        // shared among consumers
        atomic<long> consumerLock __attribute__((aligned(64)));
        //char pad[ 64 - sizeof(consumerLock) - sizeof(first)];
        // for one producer at a time
#else
	std::atomic<Node*> first __attribute__((aligned(64)));
#endif
#if (NO_PRODUCER_SPINLOCK == 1)
        atomic<Node*> last __attribute__((aligned(64)));
#else
        Node *last __attribute__((aligned(32)));
#endif

        // shared among producers
#if (NO_PRODUCER_SPINLOCK == 0)
        atomic<long> producerLock __attribute__((aligned(32)));
#endif

#if (REUSE_NODES == 1)
        atomic<Node*> nodeList; // __attribute__((aligned(64)));


        Node* getNewNode(pointer_type t) {
                Node * n = nodeList;
                if (n) {
                        Node* next = n->next;
                        while (!nodeList.compare_exchange_strong(n,next)) {
                                n = nodeList;
                                if (n) {
                                        next = n->next;
                                }
                        }

                        if (n) {
                                n->value = t;
                                n->next = 0;
                                return n;
                        } else {
                                return new Node(n);
                        }
                } else {
                        return new Node(n);
                }
        }

        void addNodeToList(Node* node) {
                Node *n(0);
                do {
                        n = nodeList;
                        node->next = n;
                } while (!nodeList.compare_exchange_strong(n,node));

        }
#else
        Node* getNewNode(pointer_type t) {
                return new Node (t);
        }
        void addNodeToList(Node* node) {
                delete node;
        }
#endif

	std::atomic<std::size_t> mWaiters __attribute__((aligned(16)));
	std::atomic<ssize_t> mSize __attribute__((aligned(16)));

public:
        SpinLockQueue() : mWaiters(0) {
		mSize  = 0;
#if (REUSE_NODES == 1)
                nodeList = new Node(0);
#endif
                first  = last = new Node(0);           // add dummy separator
#if (NO_PRODUCER_SPINLOCK == 0)
                producerLock = false;
#endif
#if (NO_CONSUMER_SPINLOCK == 0)

                consumerLock = false;
#endif
        }

        SpinLockQueue(const SpinLockQueue&) = delete;
        SpinLockQueue& operator=(const SpinLockQueue&) = delete;

        ~SpinLockQueue() {
                Node* tmp;
                while( (tmp = first) != 0 ) {   // release the list
                        first = tmp->next;
                        delete tmp;
                }

#if (REUSE_NODES == 1)
                while ( (tmp = nodeList) != 0) {
                        nodeList.store(tmp->next);
                        delete tmp;
                }
#endif
        }


        void push( param_type t ) {
                Node* newlast = getNewNode(t);
                assert(newlast);
                assert(newlast->value);

#if (NO_PRODUCER_SPINLOCK == 1)
                // what prevents oldLast from being deleted while I'm doing this compare and swap?
                while(true) {
                        Node *oldLast = last.load();
                        if (last.compare_exchange_strong(oldLast,newlast)) {
                                oldLast->next = newlast;
                                break;
                        } else {
                                oldLast = last.load();
                        }
                }

#else
                long oldvalue = false;
                while (!producerLock.compare_exchange_strong(oldvalue,true)) {
                        oldvalue = false;
                }
                assert(producerLock);
// own producer lock

                last->next = newlast;
                last = newlast;
                long exchange;
                exchange = producerLock.exchange(false);
                assert(exchange);
#endif
		++mSize;
                if (mWaiters) {
                        mCondition.notify_one();
                }
        }
#if (NO_CONSUMER_SPINLOCK == 1)
	bool try_pop( reference_type result )
	{
		while((Node*)first != (Node*)last)
		{
			Node * volatile f = first; // Required volatile because of f->next updates
			while(!first.compare_exchange_strong(const_cast<Node*&>(f),NULL))
			{
				f = first;
			}

			if (f == NULL)
			{
				continue; // Didn't acquire spin lock, rerun loops..
			}

			Node * next;
			while ((next = f->next)  == NULL) {} // Wait for producer to finish setting next

			while (next)
			{
				Node* fold = f;
				if (next->value == 0)
				{
				//	f->value = 0; // Is this really necessary?
					f = next;
					next = f->next;
					addNodeToList(f);
					--mSize;
				}
				else 
				{
					pointer_type val = next->value;
					next->value = 0;
					f = next;
					// no need to update next, but just in case
					next = 0;
					first.store(f); // reset first back to a real pointer again...
					addNodeToList(fold);
					result = std::move(val);
					--mSize;
					return true;
				}
			}
			
			
		}
		
		return false;
	}
#else
        bool try_pop( reference_type result ) {
                // No nodes point to front
                long oldvalue = false;
                while (!consumerLock.compare_exchange_strong(oldvalue,true)) {
                        oldvalue = false;
                }


                while (true) {
                        Node * volatile theFirst = first;
                        Node * next;

                        while ((next = theFirst->next) == NULL) {}
                        if (next) {
                                if (next->value == 0) {

                                        first = next;
                                        theFirst->value = 0;
                                        addNodeToList(theFirst);
					--mSize;
                                        continue;
                                } else {
                                        pointer_type val = next->value;
                                        next->value = 0;
                                        first = next;
                                        theFirst->value = 0;
                                        addNodeToList(theFirst);
                                        long exchangedVal = consumerLock.exchange(false);
                                        assert(exchangedVal);
                                        assert(val);
                                        result = std::move(val);
					--mSize;
                                }

                                return true;
                        }

                        consumerLock.exchange(false);
                        return false;
                }
        }
#endif

        void pop(reference_type result) {
                while (!try_pop(result)) {
			++mWaiters;
			--mSize;
			std::unique_lock<std::mutex> lk(mMutex);
                        mCondition.wait(lk);
			++mSize;
			--mWaiters;
                }

        }


        bool pop(reference_type result, std::chrono::system_clock::duration wait)
        {
                using namespace std::chrono;
                system_clock::time_point pt = system_clock::now() + wait;
                while (!try_pop(result)) {
                        ++mWaiters;
                        --mSize; // Allow size to drop below zero
                        std::unique_lock<std::mutex> lk(mMutex);
                        if (mCondition.wait_until(lk,pt) == std::cv_status::timeout)
			{
				++mSize;
				--mWaiters;
				return false;
			}
                        ++mSize; // Don't allow size to double decrement (try_pop and here)
                        --mWaiters;
                }

                return true;
        }
        
        // relatively unsafe operation...
        bool peek(reference_type result)
	{
	  if ((Node*)first != (Node*)last && first != (Node*)0 && ((Node*)first)->next != 0)
	  {
	    result = ((Node*)first)->next->value;
	    return true;
	  }
	  
	  return false;
	}


	ssize_t size() const { return mSize; }
	bool empty() const { return mSize <= 0; }
};


}


#if (TEST_MAIN  == 1)

static std::atomic<int> pushed, popped;

void process(SpinLockQueue<std::vector<std::string> >* queue)
{
#pragma omp parallel sections
     {
#pragma omp section
       {
#pragma omp parallel for
        for (int i = 0; i < 1000000; ++i) {
                queue->push(std::vector<std::string>());
                ++pushed;
        }
      }
#pragma omp section
      {
                std::vector<std::string> a;
                for (int i = 0; i < 1000000; ++i)
		{
			queue->pop(a);
			++popped;
		}

      }

     }
#pragma omp  barrier
}


void process(SpinLockQueue<int*> * queue)
{
#pragma omp parallel sections
     {
#pragma omp section
       {
#pragma omp parallel for
        for (int i = 0; i < 1000000; ++i) {
                queue->push((int*)0xdeadbeef);
                ++pushed;
        }
      }
#pragma omp section
      {
                int *a;
                for (int i = 0; i < 1000000; ++i)
                {
                        queue->pop(a);
                        ++popped;
                }

      }

     }
#pragma omp  barrier

}


int main(int argc, char * argv[])
{
#define TESTING_POINTER 1
#ifndef TESTING_POINTER
        SpinLockQueue<std::vector<std::string> > *queue = new SpinLockQueue<std::vector<std::string> >;
#else
	SpinLockQueue<int*> * queue = new SpinLockQueue<int*>;
#endif
        pushed = 0;
        popped = 0;
#ifdef HAS_BOOST
        ptime begin(microsec_clock::local_time());
#endif
        process(queue);
#ifdef HAS_BOOST
        ptime end(microsec_clock::local_time());


        std::cout << "Numbers: pushes: " << pushed.load() << " pops: " << popped.load() << std::endl;
        std::cout << "Time taken: " << (end-begin) << std::endl;
#endif
        delete queue;
        return 0;
}


#endif

#endif
