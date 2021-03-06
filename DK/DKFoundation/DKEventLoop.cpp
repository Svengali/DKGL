//
//  File: DKEventLoop.cpp
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2004-2019 Hongtae Kim. All rights reserved.
//

#include "DKObject.h"
#include "DKEventLoop.h"
#include "DKMap.h"
#include "DKArray.h"
#include "DKSpinLock.h"
#include "DKFunction.h"
#include "DKLog.h"
#include "DKCondition.h"

namespace DKFoundation::Private
{
#if defined(__APPLE__) && defined(__MACH__)
    void PerformOperationInsidePool(DKOperation* op);
#else
    FORCEINLINE void PerformOperationInsidePool(DKOperation* op) { op->Perform(); }
#endif

    typedef DKMap<DKThread::ThreadId, DKObject<DKEventLoop>, DKDummyLock> EventLoopMap;
    static EventLoopMap& GetEventLoopMap()
    {
        static EventLoopMap eventLoopMap;
        return eventLoopMap;
    }
    static DKSpinLock& GetEventLoopMapLock()
    {
        static DKSpinLock lock;
        return lock;
    }
    static bool RegisterEventLoop(DKThread::ThreadId id, DKEventLoop* eventLoop)
    {
        bool ret = false;
        GetEventLoopMapLock().Lock();
        ret = GetEventLoopMap().Insert(id, eventLoop);
        GetEventLoopMapLock().Unlock();
        return ret;
    }
    static void UnregisterEventLoop(DKThread::ThreadId id)
    {
        GetEventLoopMapLock().Lock();
        GetEventLoopMap().Remove(id);
        GetEventLoopMapLock().Unlock();
    }
    static DKObject<DKEventLoop> GetEventLoop(DKThread::ThreadId id)
    {
        DKObject<DKEventLoop> ret = NULL;
        GetEventLoopMapLock().Lock();
        EventLoopMap::Pair* p = GetEventLoopMap().Find(id);
        if (p)
            ret = p->value;
        GetEventLoopMapLock().Unlock();
        return ret;
    }
    static bool IsEventLoopExist(const DKEventLoop* eventLoop)
    {
        bool found = false;
        GetEventLoopMapLock().Lock();
        GetEventLoopMap().EnumerateForward([&](EventLoopMap::Pair & pair, bool* stop)
                                           {
                                               if (pair.value == eventLoop)
                                               {
                                                   *stop = true;
                                                   found = true;
                                               }
                                           });
        GetEventLoopMapLock().Unlock();
        return found;
    }

    static DKCondition resultCond;
    struct EventLoopPendingState : public DKEventLoop::PendingState
    {
        enum State
        {
            StatePending,
            StateProcessing,
            StateProcessed,
            StateRevoked,
        };
        mutable State state;

        EventLoopPendingState() : state(StatePending)
        {
        }
        bool EnterOperation() const
        {
            DKCriticalSection<DKCondition> guard(resultCond);
            if (state == StatePending)
            {
                state = StateProcessing;
                return true;
            }
            return false;
        }
        void LeaveOperation() const
        {
            DKCriticalSection<DKCondition> guard(resultCond);
            DKASSERT(state == StateProcessing);
            state = StateProcessed;
            resultCond.Broadcast();
        }
        bool Revoke() const override
        {
            DKCriticalSection<DKCondition> guard(resultCond);
            if (state == StatePending)
            {
                state = StateRevoked;
                resultCond.Broadcast();
            }
            return state == StateRevoked;
        }
        bool Result() const override
        {
            DKCriticalSection<DKCondition> guard(resultCond);
            while (state != StateProcessed && state != StateRevoked)
                resultCond.Wait();

            return state == StateProcessed;
        }
        bool IsDone() const override
        {
            DKCriticalSection<DKCondition> guard(resultCond);
            return state == StateProcessed;
        }
        bool IsRevoked() const override
        {
            DKCriticalSection<DKCondition> guard(resultCond);
            return state == StateRevoked;
        }
        bool IsPending() const override
        {
            DKCriticalSection<DKCondition> guard(resultCond);
            return state == StatePending;
        }
    };
}
using namespace DKFoundation;
using namespace DKFoundation::Private;

bool DKEventLoop::InternalCommandCompareOrder(const InternalCommandTick& lhs, const InternalCommandTick& rhs)
{
	return lhs.fire < rhs.fire;
}

bool DKEventLoop::InternalCommandCompareOrder(const InternalCommandTime& lhs, const InternalCommandTime& rhs)
{
	return lhs.fire < rhs.fire;
}

DKEventLoop::DKEventLoop()
: running(false)
, threadId(DKThread::invalidId)
, commandQueueTick(&DKEventLoop::InternalCommandCompareOrder)
, commandQueueTime(&DKEventLoop::InternalCommandCompareOrder)
{
}

DKEventLoop::~DKEventLoop()
{
	if (threadId != DKThread::invalidId)
	{
		DKERROR_THROW_DEBUG("EventLoop must be terminated before destroy!");
	}

	RevokeAll();
}

bool DKEventLoop::Run()
{
	if (BindThread())
	{
		bool loop = true;
		bool next = true;
		while (loop)
		{
			next = this->Dispatch();
			loop = this->running;
			if (!next && loop)
				OnIdle();
		}
		UnbindThread();
		return true;
	}
	return false;
}

bool DKEventLoop::IsRunning() const
{
	if (threadId != DKThread::invalidId)
	{
		DKASSERT_DEBUG(IsEventLoopExist(this));
		return true;
	}
	return false;
}

void DKEventLoop::InternalPostCommand(const InternalCommandTick& cmd)
{
	DKCriticalSection<DKCondition> guard(commandQueueCond);
	commandQueueTick.Insert(cmd);
	commandQueueCond.Signal();
}

void DKEventLoop::InternalPostCommand(const InternalCommandTime& cmd)
{
	DKCriticalSection<DKCondition> guard(commandQueueCond);
	commandQueueTime.Insert(cmd);
	commandQueueCond.Signal();
}

bool DKEventLoop::BindThread()
{
	if (this->threadId == DKThread::invalidId)
	{
		DKASSERT_DEBUG(IsEventLoopExist(this) == false);
		DKThread::ThreadId tid = DKThread::CurrentThreadId();
		if (RegisterEventLoop(tid, this))
		{
			this->threadId = tid;
			running = true;
			return true;
		}
		else
		{
			DKLog("BindThread failed!\n");
		}
	}
	return false;
}

void DKEventLoop::UnbindThread()
{
	DKASSERT_DEBUG(IsEventLoopExist(this));
	DKASSERT_DEBUG(threadId != DKThread::invalidId);
	DKASSERT_DEBUG(GetEventLoop(threadId) == this);
	UnregisterEventLoop(this->threadId);
	threadId = DKThread::invalidId;
}

void DKEventLoop::Stop()
{
	if (IsRunning())
	{
		this->Post(DKFunction([this]() {
			this->running = false;
		})->Invocation());
	}
}

DKObject<DKEventLoop::PendingState> DKEventLoop::Post(const DKOperation* operation, double delay)
{
	if (operation)
	{
		InternalCommandTick cmd;
		cmd.operation = const_cast<DKOperation*>(operation);
		cmd.state = DKOBJECT_NEW EventLoopPendingState();
		cmd.fire = DKTimer::SystemTick() + static_cast<DKTimer::Tick>(DKTimer::SystemTickFrequency() * Max(delay, 0.0));
		InternalPostCommand(cmd);

		return cmd.state;
	}
	else
	{
		// Just wake up the dispatch thread
		DKCriticalSection<DKCondition> guard(commandQueueCond);
		commandQueueCond.Signal();
	}
	return NULL;
}

DKObject<DKEventLoop::PendingState> DKEventLoop::Post(const DKOperation* operation, const DKDateTime& runAfter)
{
	if (operation)
	{
		InternalCommandTime cmd;
		cmd.operation = const_cast<DKOperation*>(operation);
		cmd.state = DKOBJECT_NEW EventLoopPendingState();
		cmd.fire = runAfter;
		InternalPostCommand(cmd);

		return cmd.state;
	}
	else
	{
		// Just wake up the dispatch thread
		DKCriticalSection<DKCondition> guard(commandQueueCond);
		commandQueueCond.Signal();
	}
	return NULL;
}

bool DKEventLoop::Process(const DKOperation* op)
{
	if (op)
	{
		if (this->IsWrokingThread())
		{
            this->PerformOperation(op);
			return true;
		}
		else
		{
			if (auto p = this->Post(op); p)
				return p->Result();
		}
	}
	return false;
}

size_t DKEventLoop::RevokeAll()
{
	DKCriticalSection<DKCondition> guard(this->commandQueueCond);

	//DKTimer::Tick tick = DKTimer::SystemTick();

	size_t numItems = this->commandQueueTick.Count() + this->commandQueueTime.Count();

	auto revoke = [](const InternalCommand& ic)
	{
		const EventLoopPendingState* state = ic.state.StaticCast<EventLoopPendingState>();
		if (state)
			state->Revoke();
	};

	for (const InternalCommand& ic : this->commandQueueTick)
		revoke(ic);
	for (const InternalCommand& ic : this->commandQueueTime)
		revoke(ic);

	this->commandQueueTick.Clear();
	this->commandQueueTime.Clear();
	return numItems;
}

bool DKEventLoop::GetNextLoopIntervalNL(double* d) const
{
	DKTimer::Tick currentTick = DKTimer::SystemTick();
	DKDateTime currentDate = DKDateTime::Now();

	size_t numTickCmd = 0;
	size_t numTimeCmd = 0;
	double tickDelay = 0;
	double timeDelay = 0;

	numTickCmd = commandQueueTick.Count();
	numTimeCmd = commandQueueTime.Count();

	double freq = 1.0 / static_cast<double>(DKTimer::SystemTickFrequency());
	if (numTickCmd > 0)
	{
		const InternalCommandTick& cmd = commandQueueTick.Value(0);
		if (cmd.fire > currentTick && freq > 0)
			tickDelay = static_cast<double>(cmd.fire - currentTick) * freq;
	}
	if (numTimeCmd > 0)
	{
		const InternalCommandTime& cmd = commandQueueTime.Value(0);
		if (cmd.fire > currentDate)
			timeDelay = cmd.fire.Interval(currentDate);
	}

	if (numTickCmd > 0 || numTimeCmd > 0)
	{
		double delay = 0;

		if (numTickCmd > 0 && numTimeCmd > 0)
			delay = Min(tickDelay, timeDelay);
		else if (numTickCmd > 0)
			delay = tickDelay;
		else
			delay = timeDelay;

		*d = delay;
		return true;
	}
	return false;
}

void DKEventLoop::WaitNextLoop()
{
	DKCriticalSection<DKCondition> guard(this->commandQueueCond);

	double d = 0.0;
	if (GetNextLoopIntervalNL(&d))
	{
		d = Max(d, 0.0);
		if (d > 0.0)
			this->commandQueueCond.WaitTimeout(d);
	}
	else
	{
		this->commandQueueCond.Wait();
	}
}

bool DKEventLoop::WaitNextLoopTimeout(double t)
{
	if (t > 0.0)
	{
		DKCriticalSection<DKCondition> guard(this->commandQueueCond);

		double d = 0.0;
		if (GetNextLoopIntervalNL(&d))
		{
			double delay = Clamp(d, 0.0, t);
			if (delay > 0.0)
			{
				this->commandQueueCond.WaitTimeout(delay);
			}
			return delay < t;
		}
		else
		{
			this->commandQueueCond.WaitTimeout(t);
		}
	}
	return false;
}

double DKEventLoop::PendingEventInterval() const
{
	DKCriticalSection<DKCondition> guard(this->commandQueueCond);

	double d = 0.0;
	if (GetNextLoopIntervalNL(&d))
	{
		return Max(d, 0.0);
	}
	return -1.0;
}

void DKEventLoop::PerformOperation(const DKOperation* operation)
{
	operation->Perform();
}

bool DKEventLoop::Dispatch()
{
	DKASSERT_DEBUG(this->threadId == DKThread::CurrentThreadId());

	DKObject<DKOperation> operation = NULL;
	DKObject<PendingState> state = NULL;

	commandQueueCond.Lock();

	DKDateTime currentTime = DKDateTime::Now();
	DKTimer::Tick currentTick = DKTimer::SystemTick();

	if (operation == NULL && this->commandQueueTick.Count() > 0)
	{
		const InternalCommandTick& cmd = this->commandQueueTick.Value(0);
		if (cmd.fire <= currentTick)
		{
			operation = cmd.operation;
			state = cmd.state;
			this->commandQueueTick.Remove(0);
		}
	}
	if (operation == NULL && this->commandQueueTime.Count() > 0)
	{
		const InternalCommandTime& cmd = this->commandQueueTime.Value(0);
		if (cmd.fire <= currentTime)
		{
			operation = cmd.operation;
			state = cmd.state;
			this->commandQueueTime.Remove(0);
		}
	}
	commandQueueCond.Unlock();

	if (operation)
	{
		struct OpWrapper : public DKOperation
		{
			OpWrapper(DKEventLoop* e, DKOperation* o) : el(e), op(o) {}
			DKEventLoop* el;
			DKOperation* op;

			void Perform() const override
			{
				el->PerformOperation(op);
			}
		};
		OpWrapper op(this, operation);
		EventLoopPendingState* stateCallback = state.StaticCast<EventLoopPendingState>();
		if (stateCallback)
		{
			if (stateCallback->EnterOperation())
			{
				Private::PerformOperationInsidePool(&op);
				stateCallback->LeaveOperation();
			}
		}
		else
		{
			Private::PerformOperationInsidePool(&op);
		}

		return true;
	}
	return false;
}

bool DKEventLoop::IsWrokingThread() const
{
	return DKThread::CurrentThreadId() == this->threadId;
}

DKThread::ThreadId DKEventLoop::RunningThreadId() const
{
	return this->threadId;
}

DKObject<DKEventLoop> DKEventLoop::CurrentEventLoop()
{
	return GetEventLoop(DKThread::CurrentThreadId());
}

DKObject<DKEventLoop> DKEventLoop::EventLoopForThreadId(DKThread::ThreadId id)
{
	return GetEventLoop(id);
}

bool DKEventLoop::IsRunning(DKEventLoop* eventLoop)
{
	return IsEventLoopExist(eventLoop);
}
