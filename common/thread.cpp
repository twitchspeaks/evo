#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <utility>
#include <memory>
//#include <thread>
#include <boost/thread.hpp>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <sstream>
#include "common/util.hpp"
#include "common/result.hpp"
#include "common/i_stringable.hpp"
#include "common/thread.hpp"

using boost::thread;
using namespace std;
using namespace std_results;
using namespace evo;
using namespace evo::opt;

/** The default ctors of both the std:: and boost:: thread id types build an
 instance representing Not-A-Thread.
 */
const ThreadId UThread::kInvalidThreadId = ThreadId();


static unordered_map <ThreadId, UThread*, ThreadIdHash> g_uthread_registry;
static mutex g_uthread_registry_mutex;

/** Facilitates RAII-style global UThread registration
 */
class UThreadRegistration
{
public:

  UThreadRegistration (UThread* uthread) {
    unique_lock<mutex> lock (g_uthread_registry_mutex);
    g_uthread_registry [GetThisThreadId()] = uthread;
  }
  ~UThreadRegistration () {
    unique_lock<mutex> lock (g_uthread_registry_mutex);
    g_uthread_registry.erase(GetThisThreadId());
  }
};

UThread* evo::GetUThread (ThreadId id, bool acquire_state_lock)
{
  unique_lock<mutex> lock (g_uthread_registry_mutex);

  auto iter = g_uthread_registry.find(id);
  if (g_uthread_registry.end() == iter) {
    return nullptr;
  }

  // if the UThread exists in the registry, then it hasn't been freed, and
  // won't be unregistered as long as we control the registry mutex

  UThread* uthread = iter->second;

  if (acquire_state_lock) {
    uthread->LockState();
  }

  return uthread;
}

UThread* evo::GetThisUThread ()
{
  // not acquiring state lock, as the returned UThread represents THIS thread
  return GetUThread(GetThisThreadId(), false);
}


string UThread :: StateToString (State state)
{
  switch (state)
  {
  case State::kInvalid:
    return "Invalid";
  case State::kInit:
    return "Initializing";
  case State::kGo:
    return "Run";
  case State::kIdle:
    return "Idle";
  case State::kExiting:
    return "Exiting";
  case State::kExited:
    return "Exited";
  // no default clause so that the compiler will issue a warning if we've
  // skipped an enum member
  }

  return "Unknown UThread state '" + to_string(static_cast<int>(state)) + "'";
}

string UThread :: StateToString (State state, const string& invalid_state_str)
{
  if (State::kInvalid == state) {
    return invalid_state_str;
  }
  return StateToString(state);
}


string UThread :: CycleWaitToString (CycleWait cycle_wait)
{
  switch (cycle_wait)
  {
  case CycleWait::kInvalid:
    return "Invalid";
  case CycleWait::kAbsolute:
    return "Absolute";
  case CycleWait::kRelative:
    return "Relative";
  case CycleWait::kIndefinite:
    return "Indefinite";
  // no default clause so that the compiler will issue a warning if we've
  // skipped an enum member
  }

  return "Unknown UThread cycle wait type";
}

UThread :: UThread (ThreadExecFunc thread_func)
{
  Init();
  user_thread_func_ = thread_func;
}

UThread :: UThread (const string& thread_name, ThreadExecFunc thread_func)
{
  Init();
  name_ = thread_name;
  user_thread_func_ = thread_func;
}

void UThread :: Init ()
{
  lwpid_ = 0;

  cycle_count_ = 0;

  state_           = State::kInvalid;
  requested_state_ = State::kInvalid;
  prev_state_      = State::kInvalid;

  cycle_wait_type_       = CycleWait::kAbsolute;
  cycle_wait_period_     = Duration(0);
  cycle_wait_changed_    = false;
  cycle_wait_skip_count_ = 0;
  cycle_wait_skip_orig_state_ = State::kInvalid;

  dbg_paused_count_ = 0;

  state_ready_mutex_owner_id_ = kInvalidThreadId;

  next_state_change_listener_handle_ = 0;

  is_joined_with_thread_ = false;
  is_paused_ = false;
  is_pause_pending_ = false;
  is_between_cycles_ = false;

  // initialize all of these to zero, i.e., "state has never been applied"
  state_ready_time_points_[static_cast<int>(State::kInit)]    = TimePoint(0);
  state_ready_time_points_[static_cast<int>(State::kIdle)]    = TimePoint(0);
  state_ready_time_points_[static_cast<int>(State::kGo)]      = TimePoint(0);
  state_ready_time_points_[static_cast<int>(State::kExiting)] = TimePoint(0);
  state_ready_time_points_[static_cast<int>(State::kExited)]  = TimePoint(0);

  state_ready_wait_count_ = 0;

  thread_func_result_ = INVALID_RESULT;

  enable_thread_wrapper_log_messages_ = true;
}

void UThread :: InitThread (opt::Blocking block)
{
  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  assert(!thread_exists()); //<< "UThread::InitThread(): thread_ is non-null";

  state_ = State::kInit;
  requested_state_ = State::kIdle;

  thread_.reset(new boost::thread(&UThread::_mainThreadFunc, this));
  if (opt::Blocking::kOn == block) {
    // hold til the thread assumes 'Idle' state
    StateWait(State::kIdle);
  }

  if (use_lock) UnlockState();
}

UThread :: ~UThread ()
{
  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  // stop the thread, if running, before exiting
  if (state_ != State::kInvalid && state_ != State::kExited && thread_exists())
  {
    Result res = Exit(opt::Blocking::kOn);
    // FIXME: figure out why Exit() occasionally returns NOT_INIT despite this
    //        block being executed only if, in a locked context, we can see
    //        that the current state indicates the thread is running
    if (std_results::NOT_INIT != res) {
      assert(SUCCESS == res);
          // << "UThread failed to exit: " << res << ": " << ToString();
      JoinInternal();
    }
  }

  if (use_lock) UnlockState();
}

Result UThread :: Start ()
{
  return Start(opt::Blocking::kOn);
}

Result UThread :: Start (opt::Blocking block)
{
  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  if (thread_exists()) {
    if (use_lock) UnlockState();
    return STATE_ALREADY_EFFECTIVE.Prepend("UThread has already been started");
  }
  InitThread(block);

  if (use_lock) UnlockState();

  return SUCCESS;
}

Result UThread :: RequestState (State newstate, Blocking block)
{
  if (Blocking::kOn == block) {
    return RequestState(newstate, Duration::Min());
  }
  else {
    return RequestState(newstate, Duration(0));
  }
}

Result UThread :: RequestState (State newstate, Duration timeout)
{
  if (!thread_exists()) {
    return NOT_INIT.Prepend("UThread has not been started");
  }

  Result res;

  // Cannot request kInit state, stupid.  It's an internal thing.
  if (State::kInit == newstate) {
    return INVALID_ARGUMENT.Prepend(
        "'Init' thread state cannot be directly requested" );
  }
  // Cannot directly request kExited, either; one must request kExiting and
  // then StateWait(kExited) to block until the state has changed to kExited.
  if (State::kExited == newstate) {
    return INVALID_ARGUMENT.Prepend(
        "'Exited' thread state cannot be directly requested" );
  }

  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  if (State::kExited == state_) {
    if (use_lock) UnlockState();
    return RESOURCE_UNAVAILABLE.Prepend(
        "Thread has exited, ignoring request for new state '"
        + StateToString(newstate) + "'" );
  }

  if (State::kExiting == state_)
  {
    if (use_lock) UnlockState();

    if (State::kExiting == newstate) {
      return SUCCESS;
    }
    else {
      return SHUTTING_DOWN.Prepend(
          "Thread is exiting, ignoring request for new state '"
          + StateToString(newstate) + "'" );
    }
  }

  // if we've joined with the thread, then the current state should be locked
  // at kExited
  assert (!is_joined_with_thread_);

  // make sure we aren't paused
  if (is_paused_) {
    Unpause();
  }

  // cannot override a pending EXITING/EXITED state request
  if ( (State::kExiting == requested_state_ ||
        State::kExited  == requested_state_)  &&
      State::kExiting != newstate )
  {
    if (use_lock) UnlockState();
    stringstream strm;
    strm << "Thread is exiting, cannot request new state '"
         << StateToString(newstate) << "'";
    return SHUTTING_DOWN.Prepend(strm.str());
  }

  if (newstate == state_)
  {
    // current state matches requested state, do nothing (re-assigning the
    // current state could actually result in unexpected behavior, e.g., the
    // invocation of the respective state change callback func even though
    // the state did not actually change), but do cancel any other pending
    // state change request
    requested_state_ = State::kInvalid;
    // because we're canceling any other pending state change request, we must
    // wake any thread that may be waiting for the former requested_state to
    // become realized.  Note that said waiting thread may then automatically
    // re-issue its state change request.
    state_ready_cond_.notify_all();
  }
  else
  {
    if (Duration(0) == timeout) // don't block
    {
      requested_state_ = newstate;
      // wake thread regardless of state since State::kGo threads may still
      // enter into an interruptible sleep to satisfy the cycle wait period
      go_cond_.notify_one();
    }
    else
    {
      // If blocking was enabled and the current thread IS <this>, then
      // we'll deadlock while waiting for our own state to change; really, this
      // is indicative of a bug in the caller.
      // The moral here is, pass block = false when invoking this function
      // from the thread that this UThread represents.
      if (is_current_thread()) {
        if (use_lock) UnlockState();
        return DEADLOCK_AVERTED.Prepend(
            "New thread state requested with blocking enabled, but target"
            " thread == current thread" );
      }

      Duration remaining_wait_time = timeout;

      while (newstate != state_)
      {
        if (State::kExiting == state_)
        {
          if (use_lock) UnlockState();
          return SHUTTING_DOWN.Prepend(
              "Thread is exiting, canceling request for new state '"
              + StateToString(newstate) + "'" );
        }
        else if (State::kExited == state_)
        {
          if (State::kExiting != newstate) {
            if (use_lock) UnlockState();
            return RESOURCE_UNAVAILABLE.Prepend(
                "Thread has exited, canceling request for new state '"
                + StateToString(newstate) + "'" );
          }
          break;
        }
        // break if the thread is exiting even though we didn't request an
        // 'exiting' state
        if (State::kExiting == state_ && State::kExiting != newstate) {
          break;
        }

        requested_state_ = newstate;

        // wake thread regardless of state since State::kGo threads enter into
        // an interruptible sleep for UThread::cycle_wait_usec at the top of
        // every cycle
        go_cond_.notify_one();


        if (timeout != Duration::Min())
        {
          bool timed_out = false;

          TimePoint start_time = TimePoint::Now();
          res = StateWaitFor(newstate, remaining_wait_time);
          if (SUCCESS == res) {
            timed_out = false;
          }
          else if (TIMED_OUT == res) {
            timed_out = true;
          }
          else {
            if (use_lock) UnlockState();
            return res.Prepend(
                "Error occurred while waiting for thread state to change (with timeout)" );
          }

          remaining_wait_time -= (TimePoint::Now() - start_time);

          if (timed_out || remaining_wait_time <= Duration(0))
          {
            if (newstate == state_) {
              break;
            }
            else {
              // timeout!!  uthread failed to apply the new state within the
              // given timeout period
              if (use_lock) UnlockState();
              return TIMED_OUT.Prepend(
                  "Timed out while waiting for thread to assume new state" );
            }
          }
        }
        else // timeout <= duration<Rep, Period>::zero()
        {
          // wait indefinitely
          if (SUCCESS != (res = StateWait(newstate))) {
            if (use_lock) UnlockState();
            return res.Prepend(
                "Error occurred while waiting for thread state to change (no timeout)" );
          }
        }
      }
    }
  }

  if (use_lock) UnlockState();

  return SUCCESS;
}

Result UThread :: RequestStateMultiple (const vector<UThread*>& in_threads,
                                         State newstate,
                                         vector<StateChangeFail>* failures_out)
{
  // TODO: Should really have a debug-only assertion that verfies there are
  // no other active calls to this function whose <threads> array intersects
  // with the <threads> array to this call, because should this race condition
  // ever arise, it could manifest as bizarre symptoms that are difficult to
  // diagnose.

  Result res;

  int go_state_pending_count = 0;
  condition_variable go_state_ready_cond;
  mutex go_state_ready_mutex;

  mutex failures_mutex;

  // subset of in_threads excluding those UThreads whose state cannot be
  // changed for whatever reason (indicated in failures_out)
  vector<UThread*> threads;

  // synchronously activating multiple UThreads is a little more complicated
  // than activating a single UThread.  The idea here is to make sure that
  // each target UThread (in <uthreads>) pauses after setting its state to
  // kGo until all of the threads have set their states to kGo;
  // this way the application-land thread functions associated with each
  // UThread in <uthreads> will not resume until all <uthreads> have reached
  // a checkpoint in ProcState() strategically positioned beyond the
  // point at which UThread::state changes.
  if (State::kGo == newstate)
  {
    go_state_ready_mutex.lock();

    for (auto& thread : in_threads)
    {
      thread->LockState();
      if (thread->is_available()) {
        threads.push_back(thread);
        thread->RequestStateMultiple_Prepare( &go_state_pending_count,
                                              &go_state_ready_mutex,
                                              &go_state_ready_cond );
      }
      else { // !thread.is_available(), i.e. thread has exiting or has exited.
        // Skip threads that are exiting or have already exited;
        // it's thread-safe to decrement this repeatedly in this loop even
        // though multiple UThreads are pointing to it since we haven't
        // unlocked the go_state_ready_mutex yet.
        --go_state_pending_count;
        stringstream strm;
        strm << "[RequestStateMultiple] Cannot set target thread state to '"
             << StateToString(newstate) << "', thread is exiting or has exited.";
        if (failures_out) {
          failures_out->push_back( StateChangeFail(thread,
              SHUTTING_DOWN.Prepend(strm.str())) );
        }
      }
      thread->UnlockState();
    }
    go_state_pending_count = threads.size();
  }

  vector<unique_ptr<RequestStateMultipleHelper>> helpers;

  // first start one temporary thread for each UThread entering <newstate>
  for (auto& thread : threads)
  {
    RequestStateMultipleHelper* helper = new RequestStateMultipleHelper(
        thread, newstate, &failures_mutex, failures_out );
    if (SUCCESS != (res = helper->Start()))
    {
      if (failures_out) {
        stringstream strm;
        strm << "[RequestStateMultiple] Couldn't start helper for thread [LWPID "
             << thread->lwpid() << "]";
        failures_out->push_back(StateChangeFail(thread, res.Prepend(strm.str())));
      }
      delete helper;
      continue;
    }
    helpers.push_back(unique_ptr<RequestStateMultipleHelper>(helper));
  }

  if (State::kGo == newstate)
  {
    if (go_state_pending_count > 0) {
      // wait for all target UThreads to activate, i.e., enter the GO state
      unique_lock<mutex> go_state_ready_lock (go_state_ready_mutex,
                                              defer_lock_t());
      go_state_ready_cond.wait(go_state_ready_lock);
      go_state_ready_lock.release();
    }
    go_state_ready_mutex.unlock();
    // Bug check: ensure that this actually worked (# threads not in GO state == 0)
    assert (0 == go_state_pending_count);
  }

  // now wait for all of the helpers to complete
  for (auto& helper : helpers)
  {
    if (SUCCESS != (res = helper->Join()))
    {
      if (failures_out) {
        failures_out->push_back( StateChangeFail(helper->uthread(),
            res.Prepend("[RequestStateMultiple] Couldn't join with helper thread")) );
      }
    }
  }

  return SUCCESS;
}

Result UThread :: Run (Blocking block)
{
  return RequestState(State::kGo, block);
}

Result UThread :: Idle (Blocking block)
{
  return RequestState(State::kIdle, block);
}

Result UThread :: Exit (Blocking block)
{
  Result res;

  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  if (is_joined_with_thread_) {
    // we've already exited
    if (use_lock) UnlockState();
    return SUCCESS;
  }
  if (State::kExited != state_)
  {
    if ( State::kExiting != state_
         && SUCCESS != (res = RequestState(State::kExiting, block))
         && RESOURCE_UNAVAILABLE != res ) { // RESOURCE_UNAVAILABLE: already exited
      if (use_lock) UnlockState();
      return res.Prepend("Couldn't change UThread state to 'exiting'");
    }
    if (Blocking::kOn == block && SUCCESS != (res = StateWait(State::kExited))) {
      if (use_lock) UnlockState();
      return res.Prepend("Couldn't wait for UThread to exit");
    }
  }

  if (use_lock) UnlockState();

  return SUCCESS;
}

void UThread :: RegisterStateChangeListener (StateChangeListenerFunc listener,
                                              ListenerHandle* handle_out)
{
  state_change_listeners_mutex_.lock();

  ListenerHandle handle = next_state_change_listener_handle_++;
  state_change_listeners_.insert(make_pair(handle, listener));

  state_change_listeners_mutex_.unlock();

  if (handle_out) {
    *handle_out = handle;
  }
}

Result UThread :: UnregisterStateChangeListener (ListenerHandle handle)
{
  state_change_listeners_mutex_.lock();

  if (0 == state_change_listeners_.erase(handle)) {
    state_change_listeners_mutex_.unlock();
    return NOT_REGISTERED.Prepend(
        "Cannot unregister invalid state change listener handle '"
        + std::to_string(handle) + "'");
  }

  state_change_listeners_mutex_.unlock();

  return SUCCESS;
}

Result UThread :: RunOneCycle (Blocking block)
{
  return RunNCycles(1, block);
}

Result UThread :: RunNCycles (int n_cycles, Blocking block)
{
  // incomplete; if/when needed, remove this return and test the existing code
  // to determine what remains to be done
  return NOT_IMPLEMENTED.Prepend("UThread::RunNCycles()");

  if (!thread_exists()) {
    return NOT_INIT.Prepend("UThread has not been started");
  }

  if (0 == n_cycles) {
    return SUCCESS; // boy, that was easy!
  }

  Result res;
  bool use_lock = !have_state_lock();

  if (n_cycles < 0) {
    stringstream strm;
    strm << "n_cycles must be >= 0, got " << n_cycles;
    return INVALID_ARGUMENT.Prepend(strm.str());
  }

  if (use_lock) LockState();

  switch (state_)
  {
  case State::kInit:
  case State::kIdle:
  case State::kGo:

    if (cycle_wait_skip_count_ < n_cycles) {
      // only change cycle_wait_skip_count if another thread isn't waiting
      // for its own n_cycles to elapse; e.g., if thread A wants to block until
      // thread C has completed N wait_skip cycles (i.e., cycles without the
      // cycle_wait_usec sleep period), and thread B wants the same for M
      // cycles, then we need to select max(N, M) so that threads A and B will
      // both get what they want.
      cycle_wait_skip_count_ = n_cycles;
    }
    cycle_wait_skip_orig_state_ = state_;
    if (State::kIdle == state_) {
      res = RequestState(State::kGo, opt::Blocking::kOff);
    }
    else {
      // just wake up UThread from cycle sleep period, if necessary
      go_cond_.notify_one();
    }
    if (Blocking::kOn == block && SUCCESS == res)
    {
      int64_t cycle_stop_count = cycle_count_ + n_cycles;
      if (!is_between_cycles_) {
        // if the thread is NOT currently sleeping in State::kIdle or as part of
        // the cycle_wait_usec time, then we're actually going to wait for the
        // cycle_count to elapse an additional n_cycles+1 to conform to the
        // RunNCycles() documentation; we want to allow exactly
        // n_cycles worth of cycles *beyond* the current cycle to elapse before
        // returning.
        ++cycle_stop_count;
      }
      // using thread-local counter for blocking condition here since we want
      // to account for the possibility of other threads calling
      // RunNCycles() while this thread is waiting for its
      // n_cycles to elapse.
      while (cycle_count_ < cycle_stop_count  &&
             State::kInvalid != cycle_wait_skip_orig_state_)
      {
        StateCondWait(cycle_wait_skip_advance_cond_);
      }
    }
    break;

  case State::kExited:
  case State::kExiting:
    if (use_lock) UnlockState();
    return SHUTTING_DOWN.Prepend(
        "UThread::RunNCycles() failed, thread is exiting or has exited" );

  default:
    assert (false);
    break;
  }

  if (use_lock) UnlockState();

  return SUCCESS;
}

Result UThread :: StateWait (State state, mutex* ext_mutex)
{
  return StateWaitFor(state, Duration::Min(), ext_mutex);
}

Result UThread :: StateWaitFor (State desired_state,
                                 Duration timeout,
                                 mutex* ext_mutex)
{
  if (!thread_exists()) {
    return NOT_INIT.Prepend("UThread has not been started");
  }

  bool use_lock = !have_state_lock();

  if (State::kInit == desired_state) {
    return INVALID_ARGUMENT.Prepend("Cannot wait for 'Init' thread state");
  }

  // Cannot wait for the *current* thread's state to change, silly.
  if (is_current_thread()) {
    return THREAD_RESTRICTION.Prepend(
        "A thread cannot call UThread::StateWait() on itself" );
  }

  if (use_lock) LockState();

  while (state_ != desired_state)
  {
    if (State::kExited == state_) {
      JoinInternal();
      if (use_lock) UnlockState();
      return RESOURCE_UNAVAILABLE.Prepend("Thread has exited");
      break;
    }

    if (State::kExiting == state_ &&
        State::kExiting != desired_state && State::kExited != desired_state) {
      if (use_lock) UnlockState();
      return SHUTTING_DOWN.Prepend("Thread is exiting");
    }

    if (ext_mutex) {
      ext_mutex->unlock();
    }

    // for State::kGo and State::kIdle, I'm using the respective state-specific
    // thread condition instead of the general state_ready_cond to avoid the
    // race condition in which the UThread state changes to <state>, but then
    // rapidly changes to another state so quickly that we miss it, and we
    // don't get CPU time until the second (non-<state>) broadcast of the
    // general state_ready_cond, since the state_ready_cond is broadcast every
    // time the state changes, regardless of the new state.
    bool cond_signalled = false;
    switch (desired_state)
    {
    case State::kIdle:
      cond_signalled = (TIMED_OUT != StateCondWaitFor(idle_ready_cond_, timeout));
      break;
    case State::kGo:
      cond_signalled = (TIMED_OUT != StateCondWaitFor(go_ready_cond_, timeout));
      break;
    default:
      // else just use the general state_ready_cond
      cond_signalled = (TIMED_OUT != StateCondWaitFor(state_ready_cond_, timeout));
      break;
    }

    if (ext_mutex) {
      ext_mutex->lock();
    }

    if (!cond_signalled) {
      // timeout
      if (use_lock) UnlockState();
      return TIMED_OUT;
    }

    if (State::kIdle == desired_state || State::kGo == desired_state) {
      // if we're not waiting for the general state change cond (i.e. <state>
      // is TSTATE_EXITED or _EXITING), then the state-specific cond has been
      // met, which means that, even if the current state is not <state>, we
      // can be certain that the UThread entered <state> after we began our
      // cond wait above.  For example, if <state> is State::kGo, then we wait
      // for the uthread->go_ready_cond condition, which is only broadcast
      // when the thread is either entering State::kGo or exiting.
      // The same reasoning applies in the State::kIdle case.
      break;
    }
  }

  if (State::kExited == state_) {
    JoinInternal();
  }

  if (use_lock) UnlockState();

  return SUCCESS;
}

Result UThread :: Pause ()
{
  if (!thread_exists()) {
    return NOT_INIT.Prepend("UThread has not been started");
  }

  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  if (is_paused_) {
    // already paused
    if (use_lock) UnlockState();
    return STATE_ALREADY_EFFECTIVE.Prepend("Thread is already paused");
  }

  if (!is_available()) {
    if (use_lock) UnlockState();
    return RESOURCE_UNAVAILABLE.Prepend("Thread has exited");
  }

  is_pause_pending_ = true;

  // if we're between cycles and were able to acquire the state_ready_lock,
  // it's safe to assume that we're either State::kIdle or waiting for a
  // cycle wait period to elapse
  if (State::kGo == state_ && is_between_cycles_)
  {
    // checked by assertion below
    int prev_paused_count = dbg_paused_count_;
    StateCondWait(paused_cond_);
    // if this fails, there's a bug; this flag is supposed
    // to be cleared when the thread notices it.
    assert (!is_pause_pending_);
    // Should not have returned until either the uthread paused itself or an
    // EXITING state was requested;
    // note that the state lock ensures that u_thread_is_available() will return
    // false regardless of the timing between the broadcasting of paused_cond and
    // fulfillment of the EXITING state request.
    // Checking paused_count instead of 'is paused' flag to avoid the
    // contingency in which the thread becomes paused & unpaused before the
    // cond_wait above can reacquire the lock following the paused_cond broadcast,
    // i.e., unpauses immediately after the thread begins cond_waiting on the
    // unpause_cond, and the thread gets the state lock before the above
    // cond_wait(paused_cond) does so.
    assert (dbg_paused_count_ > prev_paused_count || !is_available());
  }

  if (use_lock) UnlockState();

  return SUCCESS;
}

Result UThread :: Unpause ()
{
  if (!thread_exists()) {
    return NOT_INIT.Prepend("UThread has not been started");
  }

  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  if (!is_paused_)
  {
    if (is_pause_pending_) {
      // clear flag since, e.g., 'pause' may have been activated while the
      // UThread was kIdle, and we entered this method without the UThread
      // ever switching to kGo.
      is_pause_pending_ = false;
      if (use_lock) UnlockState();
      return SUCCESS; // not an error
    }
    else { // not paused
      if (use_lock) UnlockState();
      return STATE_ALREADY_EFFECTIVE.Prepend("Thread isn't paused");
    }
  }

  // if this fails, there's a bug; the pause pending flag is supposed
  // to be cleared when the thread sets the paused flag.
  assert (!is_pause_pending_);

  unpause_cond_.notify_one();

  if (use_lock) UnlockState();

  return SUCCESS;
}

void UThread :: LockState () const
{
  // catch self-inflicted deadlock condition (thread tries to lock state mutex
  // despite already controlling it)
  assert (boost::this_thread::get_id() != state_ready_mutex_owner_id_);

  // see declaration in header for an explanation of these const_casts
  const_cast<UThread*>(this)->state_ready_mutex_.lock();
  const_cast<UThread*>(this)->state_ready_mutex_owner_id_ =
      boost::this_thread::get_id();
}

void UThread :: UnlockState () const
{
  // catch attempt by caller to unlock the state mutex despite not controlling it
  assert (boost::this_thread::get_id() == state_ready_mutex_owner_id_);

  // see declaration in header for an explanation of these const_casts
  const_cast<UThread*>(this)->state_ready_mutex_owner_id_ = kInvalidThreadId;
  const_cast<UThread*>(this)->state_ready_mutex_.unlock();
}

void UThread :: StateCondWait (condition_variable& cond)
{
  // StateCondWaitFor() only fails if the timeout expires, so safe to assume
  // success here
  StateCondWaitUntil(cond, TimePoint::Min()); // wait indefinitely
}

Result UThread :: StateCondWaitFor (condition_variable& cond,
                                     Duration wait_period)
{
  if (Duration(0) == wait_period) {
    return SUCCESS;
  }
  if (Duration::Min() == wait_period) {
    return StateCondWaitUntil(cond, TimePoint::Min());
  }

  Duration time_offset = wait_period;

  return StateCondWaitUntil(cond, TimePoint::Now() + time_offset);
}

Result UThread :: StateCondWaitUntil (condition_variable& cond,
                                       TimePoint timeout_time)
{
  if (!thread_exists()) {
    return NOT_INIT.Prepend("UThread has not been started");
  }

  bool use_lock = !have_state_lock();
  Result res;

  if (use_lock) LockState();

  // we're about to relinquish the lock implicitly in wait_until()
  state_ready_mutex_owner_id_ = kInvalidThreadId;
  cv_status cond_wait_result = cv_status::no_timeout;

  ++state_ready_wait_count_;

  // we're handling the locking ourselves via LockState() and UnlockState(), so
  // set mode defer_lock_t
  unique_lock<mutex> state_lock (state_ready_mutex_, defer_lock_t());

  if (TimePoint::Min() == timeout_time) {
    // wait indefinitely
    cond.wait(state_lock);
  }
  else {
    cond_wait_result = cond.wait_until(state_lock,
                                       timeout_time.std_time_point());
  }

  // disassociate state_ready_mutex_ from state_lock.  If we don't do this,
  // then -- as I understand it -- state_lock will unlock state_ready_mutex_
  // in state_lock's destructor
  state_lock.release();

  --state_ready_wait_count_;

  if (State::kExited == state_ && 0 == state_ready_wait_count_) {
    // we only do this if the UThread wrapper func may be waiting to exit
    state_ready_none_waiting_cond_.notify_one();
  }

  // we've now (implicitly) reacquired the lock
  state_ready_mutex_owner_id_ = boost::this_thread::get_id();

  if (use_lock) UnlockState();

  if (cv_status::timeout == cond_wait_result) {
    return TIMED_OUT.Prepend("Timed out while waiting for thread condition");
  }

  return SUCCESS;
}

Result UThread :: SetRelativePriority (int priority)
{
  // TODO!! (if ever needed)
  return NOT_IMPLEMENTED.Prepend("UThread::SetRelativePriority()");
}

Result UThread :: SetSelfExiting ()
{
  Result res;

  if (!is_current_thread()) {
    return THREAD_RESTRICTION.Prepend(
        "UThread::SetSelfExiting() can be invoked only by the UThread process"
        " on itself" );
  }

  if (SUCCESS != (res = RequestState(State::kExiting, opt::Blocking::kOff))) {
    return res.Prepend("SetSelfExiting");
  }
  ProcState();

  return SUCCESS;
}

Result UThread :: Sleep (Duration max_duration)
{
  if (!thread_exists()) {
    return NOT_INIT.Prepend("UThread has not been started");
  }

  // only the thread associated with this UThread should be calling this method
  if (boost::this_thread::get_id() != thread_id()) {
    return THREAD_RESTRICTION.Prepend(
        "UThread::Sleep() can be invoked only by the UThread process"
        " on itself" );
  }

  Result res;
  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  if (is_state_changing())
  {
    // a new state is already pending; don't sleep at all
    if (use_lock) UnlockState();
    return INTERRUPTED_OPERATION.Prepend(
        "Thread state change is pending, not sleeping" );
  }
  else
  {
    if (Duration::Min() == max_duration)
    {
      // sleep indefinitely (until woken by another thread)
      StateCondWait(go_cond_);
      // unblocks only when the condition is signalled
      if (use_lock) UnlockState();
      return INTERRUPTED_OPERATION;
    }
    else
    {
      if (TIMED_OUT != (res = StateCondWaitFor(go_cond_, max_duration))) {
        if (use_lock) UnlockState();
        return res.Prepend("Thread sleep was interrupted");
      }
    }
  }

  if (use_lock) UnlockState();

  return SUCCESS;
}

UThread::ProcStateResult UThread :: ProcState ()
{
  Result res;
  ProcStateResult procstate_res = ProcStateResult::kContinue;
  bool handle_state_changed = false;
  bool cond_signalled = false;
  bool use_lock = false;
  const ThreadId self_thread_id = boost::this_thread::get_id();

  // as this method should only be invoked by the thread function, arriving
  // at this point implies the thread actually exists...
  assert (thread_exists());

  // only the thread associated with this UThread should be calling this method
  assert (thread_id() == self_thread_id);

  // The current state should never be 'idle' here, as the thread function
  // only calls ProcState() when its state is NOT idle.
  assert (State::kIdle != state_);

  // if this fails, then the UThread has come back from the dead (TVirusException);
  // it somehow found a way to call ProcState() AFTER returning
  // (kExited is only set by the thread wrapper function (that called the
  // custom thread func) after the custom thread function returns)

  use_lock = !have_state_lock();

  if (use_lock) LockState();

  ++cycle_count_;

  if (State::kExiting == state_)
  {
    // we're already in an 'exiting' state.  This actually shouldn't happen
    // because this condition implies that ProcState() has already
    // returned true (i.e. exiting==true), and the thread is supposed to stop
    // calling ProcState() and terminate itself when that happens,
    // but I guess it's not awful if we end up here again, so we'll just deal
    // with it.
    ConsiderPause_locked();
    if (use_lock) UnlockState();
    return ProcStateResult::kExit;
  }

  if (State::kInvalid != cycle_wait_skip_orig_state_)
  {
    // if this fails, then cycle_wait_skip_count was decremented too many
    // times, which would indicate a bug in u_thread.
    assert (cycle_wait_skip_count_ >= 0);

    // force blocking call to RunNCycles() to re-evaluate whether the desired
    // # of cycles has elapsed
    // NOTE: since we have the state lock, this won't actually wake the
    // blocking threads in u_thread_run_n_cycles() until we release the lock
    cycle_wait_skip_advance_cond_.notify_all();

    if (State::kInvalid == requested_state_)
    {
      if (cycle_wait_skip_count_ > 0)
      {
        // the cycle_wait_skip feature has been activated, the requested skip
        // count has not yet elapsed, and no new state has been requested since
        // the feature was activated, so just decrement the # of skips
        // remaining and continue
        ConsiderPause_locked();
        --cycle_wait_skip_count_;
        if (use_lock) UnlockState();
        return ProcStateResult::kContinue;
      }
      else
      {
        if (State::kIdle == cycle_wait_skip_orig_state_) {
          requested_state_ = State::kIdle;
        }
        // just continue running if we weren't idle when we started skipping
        // cycles
        cycle_wait_skip_orig_state_ = State::kInvalid;
      }
    }
    else
    {
      // a new state has been requested since the cycle_wait_skip feature
      // was activated, so deactivate the feature and continue normally
      cycle_wait_skip_orig_state_ = State::kInvalid;
      cycle_wait_skip_count_ = 0;
    }
  }

  is_between_cycles_ = true;

  if (State::kInvalid == requested_state_)
  {
    cycle_wait_mutex_.lock();

    assert( CycleWait::kIndefinite == cycle_wait_type_ ||
            CycleWait::kRelative   == cycle_wait_type_ ||
            CycleWait::kAbsolute   == cycle_wait_type_ );

    if ( cycle_wait_period_ > Duration(0) ||
         CycleWait::kIndefinite == cycle_wait_type_ )
    {
      decltype(cycle_wait_period_) sleep_period;

      while (true)
      {
        if (CycleWait::kIndefinite != cycle_wait_type_)
        {
          if (CycleWait::kRelative == cycle_wait_type_)
          {
            sleep_period = cycle_wait_period_;
          }
          else if (CycleWait::kAbsolute == cycle_wait_type_)
          {
            auto cycle_elapsed = TimePoint::Now() - prev_cycle_time_point_;
            if (cycle_elapsed >= cycle_wait_period_) {
              sleep_period = Duration(0);
            } else {
              sleep_period = cycle_wait_period_ - cycle_elapsed;
            }
          }
        }

        if (sleep_period != Duration(0))
        {
          cycle_wait_changed_ = false;

          cycle_wait_mutex_.unlock();

          if (CycleWait::kIndefinite == cycle_wait_type_) {
            StateCondWait(go_cond_);
            cond_signalled = true;
          }
          else {
            res = StateCondWaitFor(go_cond_, sleep_period);
            // the StateCond*() functions should all return SUCCESS or TIMED_OUT
            assert (res.is_success() || TIMED_OUT == res);
            cond_signalled = res.is_success();
          }

          cycle_wait_mutex_.lock();

          if (cond_signalled)
          {
            // we were awakened before timeval expired, which means that a new
            // state has been requested, OR the cycle wait time has changed,
            // OR someone called u_thread_run_n_cycles().

            if (State::kInvalid != cycle_wait_skip_orig_state_)
            {
              // should never be decremented into negative land
              assert (cycle_wait_skip_count_ > 0);
              if (State::kInvalid == requested_state_) {
                --cycle_wait_skip_count_;
              }
              else {
                // prevent race condition in which both RunNCycles() and
                // RequestState() are called before the UThread wakes from its
                // cycle sleep
                cycle_wait_skip_orig_state_ = State::kInvalid;
                cycle_wait_skip_count_      = 0;
              }
            }

            if (cycle_wait_changed_)
            {
              // the cycle wait parameters changed, so recalculate the sleep
              // period, if any, WITHOUT updating prev_cycle_time_point_; the
              // latter is critical since otherwise the sleep period should
              // *resume*, not *restart*
              cycle_wait_changed_ = false;
              // If the new time is CycleWait::kRelative, then do NOT continue to
              // sleep; just return.  The new relative cycle wait period will
              // take effect on the next call to this function.
              // Otherwise (if CycleWait::kAbsolute), then re-evaluate our sleep
              // duration relative to whatever prev_cycle_time_point_ was set to
              // when we entered this function.  This helps to ensure that we
              // don't under/oversleep ;)
              // Further - don't go back to sleep if someone just called
              // u_thread_run_n_cycles().
              if ( CycleWait::kRelative == cycle_wait_type_ ||
                   State::kInvalid != cycle_wait_skip_orig_state_ ) {
                break;
              }
              else {
                continue;
              }
            }
            break;
          }
          else
          {
            cycle_wait_mutex_.unlock();
            // we were *not* awakened before timeval expired, which means that
            // a new state has not been requested
            prev_cycle_time_point_ = TimePoint::Now();
            is_between_cycles_ = false;
            ConsiderPause_locked();
            if (use_lock) UnlockState();
            return ProcStateResult::kContinue;
          }
        }
        else
        {
          break;
        }
      }
      cycle_wait_mutex_.unlock();
      prev_cycle_time_point_ = TimePoint::Now();
      // else we were awakened manually (go_cond was signalled),
      // so we need to look at the new pending state
    }
    else
    {
      // no cycle wait period has been defined and no new state is pending
      cycle_wait_mutex_.unlock();
      is_between_cycles_ = false;
      ConsiderPause_locked();
      if (use_lock) UnlockState();
      return ProcStateResult::kContinue;
    }
  }

  if (requested_state_ == state_)
  {
    // Don't do anything.  If State::kGo was requested and we're already GOing,
    // then we should NOT re-invoke uthread->go_func().
    requested_state_ = State::kInvalid;
    is_between_cycles_ = false;
  }
  else
  {
    do
    {
      switch (requested_state_)
      {
      case State::kGo:
        // set requested_state before calling go_func() in case the go_func()
        // itself changes the requested state
        requested_state_ = State::kInvalid;
        handle_state_changed = true;
        go_ready_cond_.notify_all();
        SetStateInternal(State::kGo);
        break;

      case State::kIdle:
        // This is to prevent a potential race condition in which someone
        // (a thread) wants to run this UThread via
        // RequestStateMultiple(State::kGo), but someone else
        // (another thread) squeezes in a State::kIdle request before this
        // UThread sees _u_thread_set_state_multiple_helper()'s State::kGo
        // request.
        // In this weird case, the State::kIdle request is ignored so the
        // RequestStateMultiple(State::kGo) request can prevail; this should
        // prevent the deadlock that would occur otherwise.
        if (set_state_multiple_info_.in_progress)
        {
          // the loop should cycle around and land in case State::kGo
          requested_state_ = State::kGo;
        }
        else
        {
          requested_state_ = State::kInvalid;
          // have to handle this change-of-state separately since it involves
          // sleeping indefinitely
          handle_state_changed = false;
          // if the requested state was changed by a handler, re-evaluate
          // what we should be doing
          if (State::kInvalid != requested_state_) {
            break;
          }
          SetStateInternal(State::kIdle);
          state_ready_cond_.notify_all();
          idle_ready_cond_.notify_all();
          StateCondWait(go_cond_);
          prev_cycle_time_point_ = TimePoint::Now();
          // we were awakened from our idle slumber, which means that a new
          // state has been requested, OR someone called u_thread_run_n_cycles().
          if (State::kInvalid != cycle_wait_skip_orig_state_) {
            // this should never be decremented below zero
            assert (cycle_wait_skip_count_ > 0);
            --cycle_wait_skip_count_;
          }
        }
        break;

        // the app should really ask for State::kExiting instead of State::kExited,
        // (in fact State::kExited should never be requested, as it's set
        // automatically) but whatever, we'll accept either.
      case State::kExiting:
        procstate_res = ProcStateResult::kExit;
        SetStateInternal(State::kExiting);
        requested_state_ = State::kInvalid;
        handle_state_changed = true;
        break;

      case State::kInvalid: // no new state is pending
        break;

      default:
        // There's a bug somewhere (not necessarily in u_thread) if a state
        // is requested other than those covered above; this situation should
        // have been prevented by validation logic in RequestState()
        assert( State::kGo      == requested_state_ ||
                State::kIdle    == requested_state_ ||
                State::kExiting == requested_state_ ||
                State::kInvalid == requested_state_ );
      }
      // looping so we'll catch the next state when IDLE is woken from its
      // slumber; this also properly handles state changes in the state change
      // callback functions.
    } while (State::kInvalid != requested_state_);
  }

  // important to do this before signaling the state_ready_cond;
  // if u_thread_set_state_multiple() is attempting to synchronize all
  // UThreads entering State::kGo (see RequestStateMultiple()), and
  // this UThread is one such thread, then wait for the other UThreads to
  // get to this point
  if (set_state_multiple_info_.in_progress)
  {
    // Note that, in this context, State::kGo, kExiting, and kExited states
    // are valid
    set_state_multiple_info_.go_mutex_p->lock();

    --(*set_state_multiple_info_.pending_count_p);

    if (*set_state_multiple_info_.pending_count_p) {
      // this assertion happens if we decrement too many times (which would
      // amount to one yucky bug.)
      assert (*set_state_multiple_info_.pending_count_p > 0);
      unique_lock<mutex> unique_state_ready_lock (
          *set_state_multiple_info_.go_mutex_p, defer_lock_t() );
      set_state_multiple_info_.go_cond_p->wait(unique_state_ready_lock);
      unique_state_ready_lock.release();
    }
    else {
      // this is the last UThread to reach this State::kGo checkpoint
      set_state_multiple_info_.go_cond_p->notify_all();
    }
    set_state_multiple_info_.go_mutex_p->unlock();
    set_state_multiple_info_.Clear();
  }

  // this is here so it won't happen more than is necessary
  if (handle_state_changed)
  {
    state_change_listeners_mutex_.lock();
    for (auto& map_pair : state_change_listeners_) {
      map_pair.second(*this, state_, prev_state_);
    }
    state_change_listeners_mutex_.unlock();
    state_ready_cond_.notify_all();
    state_ready_time_points_[static_cast<int>(state_)] = TimePoint::Now();
  }

  is_between_cycles_ = false;

  ConsiderPause_locked();

  if (use_lock) UnlockState();

  return procstate_res;
}

string UThread :: ToString () const
{
  bool use_lock = !have_state_lock();

  // we need the state lock to ensure that the UThread state depicted by the
  // constructed string is consistent
  if (use_lock) LockState();

  stringstream strm;
  //strm << "UThread: ";
  if (State::kInvalid == state_ || State::kInit == state_)
  {
    if (thread_exists())
    {
      strm << "name = " << (name_.empty() ? "(none)" : name_.c_str());
      if (State::kInvalid == state_) {
        strm << ", initialized = (invalid state)";
      } else {
        strm << ", initialized = in progress";
      }
    }
    else
    {
      if (name_.empty()) {
        strm << "name = (none), initialized = no";
      } else {
        strm << "name = " << name_ << ", initialized = no";
      }
    }
  }
  else
  {
    strm << "name = " << name_ << ", initialized = yes";
    /*
    if (IsThreadNameRegistered(thread_->get_id())) {
       strm << ", name = " << GetThreadName(thread_->get_id());
    }
    else {
      strm << ", name = (not set)";
    }
    */
  }

  // including memory address of UThread for use in a debugger such as GDB
  strm << ", LWPID = " << lwpid_;
  if (thread_exists()) {
    strm << ", thread id = " << thread_->get_id()
         << ", native handle = "
         << const_cast<boost::thread*>(thread_.get())->native_handle();
  } else {
    strm << ", thread id = N/A"
         << ", native handle = N/A";
  }
  strm << ", current state = \"" << StateToString(state_) << "\""
       << ", pending state = \"" << StateToString(requested_state_, "(none)") << "\""
       << ", cycle wait type = \"" << CycleWaitToString(cycle_wait_type_) << "\""
       << ", cycle wait period = "
       << cycle_wait_period_.Microseconds() << " us"
       << ", addr = " << (void*)this;

  if (use_lock) UnlockState();

  return strm.str();
}

void UThread :: ConsiderPause_locked ()
{
  // MUST have state lock
  assert (have_state_lock());

  if (is_pause_pending_)
  {
    is_pause_pending_ = false;
    is_paused_ = true;
    // checked exclusively by debugging assertions
    ++dbg_paused_count_;

    paused_cond_.notify_all();
    StateCondWait(unpause_cond_);

    is_paused_ = false;
  }
}

void UThread :: RequestStateMultiple_Prepare (int* pending_count_p,
                                               std::mutex* go_mutex_p,
                                               std::condition_variable* go_cond_p)
{
  assert (have_state_lock());
  set_state_multiple_info_.Activate(pending_count_p, go_mutex_p, go_cond_p);
}

void UThread :: SetStateInternal (State new_state)
{
  prev_state_ = state_;
  state_      = new_state;
}

void UThread :: JoinInternal ()
{
  // shouldn't ever be invoked with state != kExited, as this would deadlock
  assert (State::kExited == state_);

  bool use_lock = !have_state_lock();

  if (use_lock) LockState();

  if (!is_joined_with_thread_ && thread_) {
    thread_->join();
    is_joined_with_thread_ = true;
  }

  if (use_lock) UnlockState();
}

void UThread :: _mainThreadFunc ()
{
  UThreadRegistration registration (this);

  LockState();

  if (!name_.empty()) {
    RegisterCurrentThreadName(name_);
  }

  lwpid_ = GetCurrentThreadLwpid();

  if (enable_thread_wrapper_log_messages_) {
    QLOG(INFO) << "New thread: " << ToString();
  }

  UnlockState();

  // uthread->state_ready_cond will be signalled by u_thread_proc_state()
  // once we've entered TSTATE_IDLE, at which point u_thread_create() will
  // wake up and return the new UThread*
  const Result& thread_result = user_thread_func_(this);

  LockState();

  if (state_ready_wait_count_ > 0) {
    state_ready_cond_.notify_all();
    idle_ready_cond_.notify_all();
    go_ready_cond_.notify_all();
  }
  else {
    // if this fails, then state_ready_wait_count is getting over-decremented
    assert (0 == state_ready_wait_count_);
  }
  thread_func_result_ = thread_result;

  // invoke all on-exit callbacks
  state_change_listeners_mutex_.lock();
  for (auto& map_pair : state_change_listeners_) {
    map_pair.second(*this, state_, prev_state_);
  }
  state_change_listeners_mutex_.unlock();

  SetStateInternal(State::kExited);

  if (enable_thread_wrapper_log_messages_)
  {
    if (SUCCESS == thread_result) {
      QLOG(INFO) << "Thread exited normally: " << ToString();
    } else {
      QLOG(ERROR) << "Thread exited with an error, result = \""
          << thread_result.ToString() << "\": " << ToString();
    }
  }

  thread_.reset();

  UnlockState();
}


UThread::RequestStateMultipleHelper :: RequestStateMultipleHelper (
    UThread* uthread, State newstate, std::mutex* failures_mutex,
    std::vector<StateChangeFail>* failures_out )
{
  uthread_ = uthread;
  newstate_ = newstate;
  failures_mutex_ = failures_mutex;
  failures_out_   = failures_out;
  helper_thread_  = nullptr;
}

Result UThread::RequestStateMultipleHelper :: Start ()
{
  if (helper_thread_) {
    return STATE_ALREADY_EFFECTIVE;
  }
  helper_thread_.reset(
      new boost::thread(&UThread::RequestStateMultipleHelper::_threadFunc, this) );

  return SUCCESS;
}

Result UThread::RequestStateMultipleHelper :: Join ()
{
  helper_thread_->join();
  helper_thread_.reset();

  return SUCCESS;
}

void UThread::RequestStateMultipleHelper :: _threadFunc ()
{
  Result res;

  if (State::kExited == newstate_)
  {
    // handle State::kExited separately; the caller wants to block until the
    // thread is no longer running
    if (SUCCESS != (res = uthread_->RequestState(State::kExiting,
                                                  opt::Blocking::kOff))) {
      res.Prepend("Couldn't request 'exiting' state for thread");
    }
    else if (SUCCESS != (res = uthread_->StateWait(State::kExited))) {
      res.Prepend("Failed while waiting for thread to exit");
    }
  }
  else
  {
    if (SUCCESS != (res = uthread_->RequestState(newstate_,
                                                  opt::Blocking::kOn))) {
      stringstream strm;
      strm << "Couldn't request new state '" << StateToString(newstate_)
           << "' for thread";
      res.Prepend(strm.str());
    }
  }

  if (res.is_error())
  {
    if (failures_out_) {
      failures_mutex_->lock();
      failures_out_->push_back(StateChangeFail(uthread_, res));
      failures_mutex_->unlock();
    }
  }
}

