#ifndef __SynchronizedQueue_h__
#define __SynchronizedQueue_h__

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <sstream>

namespace bip = boost::interprocess;

class lock
{
public:
    lock(bip::interprocess_mutex& m)
        : _Mutex(&m), _Locked(false)
    {
        _Mutex->lock();
        _Locked = true;
    }

    ~lock()
    {
        try
        {
            if (_Locked && _Mutex)
                _Mutex->unlock();
        }
        catch (...) {}
    }
    
private:
    bip::interprocess_mutex *_Mutex;
    bool _Locked;
};

template <class T> class SynchronizedQueue
{
public:
    typedef bip::allocator<T, bip::managed_shared_memory::segment_manager> allocator_type;

private:
    bip::deque<T, allocator_type> _Queue;
    mutable bip::interprocess_mutex _IoMutex;
    mutable bip::interprocess_condition _WaitCondition;

public:
    SynchronizedQueue(allocator_type alloc) : _Queue(alloc) {}

    void push(T element)
    {
        lock lock(_IoMutex);
        _Queue.push_back(element);
        _WaitCondition.notify_one();
    }
    bool empty() const
    {
        lock lock(_IoMutex);
        return _Queue.empty();
    }

    bool pop(T &element)
    {
        lock lock(_IoMutex);

        if (_Queue.empty())
        {
            return false;
        }

        element = _Queue.front();
        _Queue.pop_front();
        return true;
    }

    unsigned int sizeOfQueue() const
    {
        // try to lock the mutex
        lock lock(_IoMutex);
        return _Queue.size();
    }

    void waitAndPop(T &element)
    {
        lock lock(_IoMutex);

        while (_Queue.empty())
        {
            _WaitCondition.wait(lock);
        }

        element = _Queue.front();
        _Queue.pop();
    }
};

#endif