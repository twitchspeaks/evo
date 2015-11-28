#ifndef COMMON_THREAD_HPP
#define COMMON_THREAD_HPP

#include <cstdint>
#include <memory>
#include <boost/thread.hpp>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_map>
#include "common/util.hpp"
#include "common/result.hpp"
#include "common/time_measures.hpp"
#include "common/i_stringable.hpp"
//#include "pscommon2/general.hpp"
//#include "pscommon2/result.hpp"
//#include "pscommon2/interfaces/i_stringable.hpp"

namespace evo {

/** Serves as a conveniently extendable wrapper for the lower-level
    thread library around which it is centered.
*/
class UThread : public IStringable
{
public:

  static const ThreadId kInvalidThreadId;

  enum class State
  {
    kInvalid,

    /** Thread has been started but has not yet made its first call to
        ProcState(); cannot be directly requested.
     */
    kInit,

    /** "Go" instead of "Run" to avoid confusion when speaking of a UThread's
      state, as "run" may refer to the thread's underlying process OR its
      UThread state, as in:
      Kermit: "Is the thread running?"
      Fozzie: "Yes, it's running, but its state is 'idle'; we need it to be
              'run' here."
      ^^ CONFUSING; this is more clear:
      Kermit: "Is the thread running?"
      Fozzie: "Yes, but its state is 'idle'; we need it to be 'go' here."
      -- Also, it sounds like a shortening of "OK Go", like from that 90's
         "The Avengers" arcade game?  Maybe?
     */
    kGo,

    /** The thread process is running, but is sleeping in the ProcState()
      component of its loop condition pending the signaling or timeout of an
      internal UThread condition variable whose purpose is to regulate
      thread state.
     */
    kIdle,

    kExiting,

    kExited
  };

  /** Maps each State to its respective text name.
   */
  static std::string StateToString (State state);

  /** Maps each State to its respective text name.
      \param state A state.
      \param invalid_state_str If state == State::kInvalid, then
      invalid_state_str is returned
   */
  static std::string StateToString (State state,
                                    const std::string& invalid_state_str);

  enum class CycleWait
  {
    kInvalid,

    /** The wait time is the total time to elapse between cycles, i.e.,
        ProcState() will return on fixed intervals.
        For example, if the cycle wait time is 500 us and the UThread spends
        200 us doing things, then ProcState() will sleep for 300 us.
     */
    kAbsolute,

    /** The wait time is the relative time to elapse between cycles, i.e.,
        ProcState() will always sleep for the specified wait time,
        regardless of how much time elapses between calls to ProcState().
     */
    kRelative,

    /** Special type of cycle wait period that never expires; typically useful
        only when a UThread's cycles are intended to be manually triggered via
        RunOneCycle() or RunNCycles().
     */
    kIndefinite
  };
  /** Maps each CycleWait to its respective text name
  */
  static std::string CycleWaitToString (CycleWait cycle_wait);

  typedef std::function <evo::Result (UThread*)> ThreadExecFunc;

  /** Arguments are: UThread* uthread, State new_state, State previous_state
   */
  typedef std::function <void (const UThread&, State, State)> StateChangeListenerFunc;

  struct RequestStateMultipleInfo
  {
    RequestStateMultipleInfo () {
      Clear();
    }
    void Clear () {
      in_progress = false;
      pending_count_p = nullptr;
      go_mutex_p = nullptr;
      go_cond_p = nullptr;
    }
    void Activate (int* _pending_count_p, std::mutex* _go_mutex_p,
                   std::condition_variable* _go_cond_p) {
      in_progress = true;
      pending_count_p = _pending_count_p;
      go_mutex_p = _go_mutex_p;
      go_cond_p = _go_cond_p;
    }
    bool in_progress;
    int* pending_count_p;
    std::mutex* go_mutex_p;
    std::condition_variable* go_cond_p;
  };

  /** Used exclusively by RequestStateMultiple()
  */
  struct StateChangeFail
  {
    StateChangeFail (UThread* _uthread, const evo::Result& _error) {
      uthread = _uthread;
      error    = _error;
    }
    StateChangeFail& operator = (const StateChangeFail& copy_src) {
      uthread = copy_src.uthread;
      error    = copy_src.error;
      return *this;
    }
    UThread* uthread; ///< Associated UThread
    evo::Result error;    ///< Error details
  };

  /** The thread is started, its main function is called from
      within a static UThread wrapper function, and the thread's state
      becomes kInit until it makes its first call to ProcState(), at which
      point kIdle is assumed automatically; the constructor will block
      until this entire initialization routine has completed.
      \param thread_name A name that will be used to identify the thread
      in e.g. log messages.
  \param thread_func Function to act as the thread's main function; the
      thread's state will be automatically set to kExited and its thread
      process terminated upon returning from this function.
  \remark Taking a callback function (that could be the output of std::bind()
      for convenience) and keeping UThread concrete instead of requiring
      inheritance makes for a simpler and much more flexible API; the choice
      of implementation is left entirely to the user.
   */
  UThread (const std::string& thread_name, ThreadExecFunc thread_func);

  UThread (const UThread& copy_src) = delete;

  /** Destructor; cleanly stops the thread before freeing associated memory.
   */
  virtual ~UThread ();

  UThread& operator = (const UThread& copy_src) = delete;

  /** Accessor for thread::get_id()
   */
  ThreadId thread_id () const {
    if (thread_) {
      return thread_->get_id();
    } else {
      return kInvalidThreadId;
    }
  }

  std::string name () const {
    return name_;
  }

  /** As returned by syscall(SYS_gettid); should be the thread's unique PID,
      not the main process PID.  This value is stored when the thread is
      created, not on demand.
   */
  pid_t lwpid () const {
    return lwpid_;
  }

  /** Thread cycle count, incremented for each call to ProcState().  This is
      typically the # iterations of the thread's outermost loop.
      Initialized to zero when the thread is created and never reset.
  */
  int64_t cycle_count () const {
    return cycle_count_;
  }

  /** The current, applied, effective state of the UThread.
   The observed state is guaranteed to be current only while the observer
   controls the UThread's state lock.
   */
  State state () const {
    return state_;
  }

  /** Time at which the UThread most recently assumed \p state.
   */
  evo::TimePoint state_timestamp (State state) const {
    return state_ready_time_points_.at(static_cast<int>(state));
  }

  /** True if Start() has been successfully invoked; implies the underlying
   thread object has been initialized and the thread's process exists.
   */
  bool thread_exists () const {
    // note that thread_ is a shared_ptr
    return static_cast<bool>(thread_);
  }

  /** True if a new thread state is pending, i.e., requested but not yet applied.
   */
  bool is_state_changing () const {
    return (State::kInvalid != requested_state_);
  }

  CycleWait cycle_wait_type () const {
    return cycle_wait_type_;
  }

  void set_cycle_wait_type (CycleWait type) {
    ThreadSafeSetVar(&cycle_wait_type_, type);
  }

  evo::Duration cycle_wait_period () const {
    return cycle_wait_period_;
  }

  void set_cycle_wait_period (evo::Duration period) {
    ThreadSafeSetVar(&cycle_wait_period_, period);
  }

  evo::Result thread_func_result () const {
    return thread_func_result_;
  }

  bool is_current_thread () const {
    if (thread_) {
      return (boost::this_thread::get_id() == thread_->get_id());
    } else {
      return false;
    }
  }

  bool is_available () const {
    return (
        State::kExiting != state_ && State::kExited != state_
     && State::kExiting != requested_state_ && State::kExited != requested_state_ );
  }

  bool is_paused () const {
    return is_paused_;
  }

  bool has_exited () const {
    return (State::kExited == state_);
  }

  bool have_state_lock () const {
    return (boost::this_thread::get_id() == state_ready_mutex_owner_id_);
  }

  void set_internal_logging_enabled (bool enable) {
    enable_thread_wrapper_log_messages_ = enable;
  }

  /** Starts the thread and blocks until it has assumed the 'Idle' state.
   */
  evo::Result Start ();

  evo::Result Start (evo::opt::Blocking block);

  /** Requests a new state for the thread.
  \param newstate The new state.  Cannot be kInit.
  \param block If enabled, the call doesn't return until the thread assumes the
      new state, OR the request is superseded by another state change
      request received before \p newstate is applied, OR the destructor
      is invoked.
      If false, the call returns immediately after submitting the request.
  \returns Result; fails if:
      * \p newstate is invalid; or
      * \p block is true and the request is canceled (see 'block'); or
      * The request is denied because the thread's state is either
        kExiting or kExited.
   */
  evo::Result RequestState (State newstate, evo::opt::Blocking block);

  /** Requests a new state for the thread.
  \param newstate The new state.  Cannot be kInit.
  \param timeout Time-out period of the state request;
       > 0 - Wait no longer than \p timeout for thread to assume \p newstate.
      == 0 - Return immediately after requesting \p newstate.  Equivalent to
             RequestState(newstate, false).
      == chrono::duration::min - Block until thread assumes \p newstate.
                                 Equivalent to RequestState(newstate, true).
  \returns Result; fails if:
      * \p newstate is invalid; or
      * \p timeout is invalid; or
      * \p timeout expires before the thread assumes \p newstate; or
      * The request is denied because the thread's state is either
        kExiting or kExited.
   */
  evo::Result RequestState (State newstate, evo::Duration timeout);

  /** Synchronously applies \p newstate to all UThreads in \p threads.
      This happens in parallel, and while the times at which the threads actually
      assume \p newstate should be very nearly equal, the order in which
      \p newstate is assumed by the threads is arbitrary.
      The purpose of this function is to minimize the average time required to
      simultaneously apply a given state to multiple UThreads, and also to
      standardize the procedure by which this is accomplished.
      Blocks until all UThreads in \p threads have assumed \p newstate.
  \param threads The UThreads that are to assume \p newstate.
  \param newstate The new thread state.  Cannot be kInit.
  \param failures_out If non-null, will be populated with one StateChangeFail
      instance for every UThread in \p threads that failed to assume \p newstate.
      It follows that the # threads that successfully applied \p newstate is
      equal to threads.size() - failures_out->size().
  \returns Result; fails if:
      * \p newstate is invalid; or
      * One or more threads failed to assume \p newstate for any reason.
  */
  static evo::Result RequestStateMultiple (
      const std::vector<UThread*>& threads,
      State newstate,
      std::vector<StateChangeFail>* failures_out = nullptr );

  /** Equivalent to RequestState(State::kGo, block)
   */
  evo::Result Run (evo::opt::Blocking block);

  /** Equivalent to RequestState(State::kIdle, block)
   */
  evo::Result Idle (evo::opt::Blocking block);

  /** Equivalent to RequestState(State::kExiting, block)
   */
  evo::Result Exit (evo::opt::Blocking block);

  void RegisterStateChangeListener (StateChangeListenerFunc listener,
                                    evo::ListenerHandle* handle_out);

  evo::Result UnregisterStateChangeListener (evo::ListenerHandle handle);

  /** Resume thread for just one cycle, i.e., one pass between calls to
      ProcState().  If the thread is kIdle, then its state is changed to
      kGo for a single cycle, and is then reset to kIdle.
      If the thread is already running but is sleeping in ProcState() for its
      nonzero cycle wait period, then it is simply woken up.
      If the thread is already running and is between calls to ProcState(),
      then the next call to ProcState() will return immediately, i.e., the
      cycle wait time is ignored, unless of course somebody alters the state
      before the next call to ProcState().
  \param block If enabled, the call doesn't return until the single cycle has
      completed; otherwise, it returns immediately after scheduling the cycle.
  \returns Result; fails if:
      * The thread's state is either kExiting or kExited.
  */
  evo::Result RunOneCycle (evo::opt::Blocking block);

  /** Generalization of RunOneCycle().  The cycle wait period is zero for the
      duration of the N cycles.
      \param n_cycles The number of cycles that will be executed.
      \param block If enabled, the call blocks until all N cycles have completed,
          otherwise the call simply arranges for them to occur and returns.
      \returns Result; fails if:
          * The thread's state is either kExiting or kExited.
  */
  evo::Result RunNCycles (int n_cycles, evo::opt::Blocking block);

  /** Block until the given state is effective.  Returns immediately if the
      current state == \p state.
  \param state The state to wait for.
  \param ext_mutex If non-null, it is released before sleeping, and reacquired
      when woken; it MUST, therefore, be locked when StateWait() is invoked.
  \returns Result; fails if:
      * \p state is not one of: kIdle, kGo, kExiting, kExited.
  */
  evo::Result StateWait (State state, std::mutex* ext_mutex = nullptr);

  /** Block until the given state is effective or given timeout period expires.
      I know the name is weird, but trying to be consistent about 'duration'
      functions (...For()) vs. 'time_point' functions (...Until())
      Returns immediately if the current state == \p state.
  \param state The state to wait for.
  \param timeout The maximum time to wait for \p state.  If this value is
      chrono::duration::min, then the call never times out.
  \param ext_mutex If non-null, it is released before sleeping, and reacquired
      when woken; it MUST, therefore, be locked when StateWait() is invoked.
  \returns Result; fails if:
      * \p state is not one of: kIdle, kGo, kExiting, kExited.
      * \p timeout is invalid.
  */
  evo::Result StateWaitFor (State state, evo::Duration timeout,
                             std::mutex* ext_mutex = nullptr);

  // TODO: StateWaitUntil(time_point)

  /** Temporarily pause the UThread.  If the thread is running or its state
      is kInit, this method waits for the thread's next call to
      ProcState(), and then freezes it within ProcState() without altering its
      effective state (i.e. state() would return the same value) until someone
      calls Unpause().
      Time spent paused counts toward the thread's cycle period, i.e., if a
      cycle wait period has been set and the thread is running, then the time
      that elapses while the thread is paused is considered 'running' time,
      and is subtracted from the cycle wait period when determining the
      necessary sleep duration to fulfill the 'cycle wait' time.
      If the UThread is idle, then this method prevents the thread from
      leaving ProcState() until someone calls Unpause().
  \returns Result; fails if:
      * The thread is already paused; or
      * The thread's state is either kExiting or kExited.
  */
  evo::Result Pause ();

  /** Complement of Pause().
  \returns Result; fails if:
      * The thread is not paused.
  */
  evo::Result Unpause ();

  /** Acquire thread's state lock.  The context of the mutex covers ProcState()
      (called at the top of every thread cycle) and every UThread method that
      directly or indirectly accesses the thread state.
      The function will know whether the current thread already controls the
      lock; if it does, LockState() will deliberately induce a software abort
      rather than deadlock.
      NOTE: Even though this method obviously alters the state of an internal
          variable (the state mutex), const contexts are allowed to invoke it.
          The justification for this rare departure from standard C++ convention
          is the need to permit const components of the public UThread API to
          be accessed in a thread-safe manner regardless of context (whether
          const or not).
          For example, if we have a const UThread& and simply need to check
          its current state, but wish to do so in thread-safe manner, then
          without this exception to the usual 'const' rules, we'd be SOL, and
          would need to either modify our code such that the UThread is
          mutable, wrap the UThread around an external mutex that we have
          full access to, or forfeit thread safety, though IMO all of these
          solutions are ugly.  Thus, I think it's better to allow const contexts
          to call LockState() & UnlockState().
   */
  void LockState () const;

  /** Complement of LockState().
      NOTE: For an explanation of why this is const, see LockState().
   */
  void UnlockState () const;

  /** Thread condition wait function that uses the UThread's state lock
      as the condition semaphore.
      If the current thread doesn't already control the state lock, it will
      be locked automatically before waiting on \p cond.
      Equivalent to StateCondWaitUntil(cond, system_clock::time_point::min())
  \param cond The condition variable.
  \see StateCondWaitFor(), StateCondWaitUntil()
   */
  void StateCondWait (std::condition_variable& cond);

  /** Thread condition wait function that uses the UThread's state lock
      as the condition semaphore.  This overload accepts a maximum wait period.
      If the current thread doesn't already control the state lock, it will
      be locked automatically before waiting on \p cond.
  \param cond The condition variable.
  \param wait_period The maximum waiting time to elapse.  This is a quantity
      of time relative to the present, not an absolute time.
  \returns Result; fails if:
      * \p wait_period expired before the condition was signalled.
  \see StateCondWait(), StateCondWaitUntil()
   */
  evo::Result StateCondWaitFor (std::condition_variable& cond,
                                 evo::Duration wait_period);

  /** Thread condition wait function that uses the UThread's state lock
      as the condition semaphore.  This overload accepts a specific time at
      which timeout occurs if the thread is still waiting.
      If the current thread doesn't already control the state lock, it will
      be locked automatically before waiting on \p cond.
  \param cond The condition variable.
  \param timeout_time The time at which to stop waiting.  This is a precise
      point in time, not a duration relative to the current time.
      If this is system_clock::time_point::min(), the call waits indefinitely
      (equivalent to StateCondWait(cond))
  \returns Result; fails if:
      * \p timeout_time was reached before the condition was signalled.
  \see StateCondWait(), StateCondWaitFor()
   */
  evo::Result StateCondWaitUntil (std::condition_variable& cond,
                                   evo::TimePoint timeout_time);

  /** Adjust scheduling priority of UThread relative to the parent process group.
      See 'nice' manual page.
  \returns Result; fails if:
      * The 'nice' syscall fails.
  */
  evo::Result SetRelativePriority (int priority);

  /** The only way for a UThread to manually set its own state to kExiting.
      Typically, kExiting is explicitly requested by another thread when
      necessary, but in certain circumstances a thread may decide to abort
      itself, whether prematurely due to an unforeseen error or as part of a
      planned, terminable series of operations.
      This may be invoked only by the thread itself.
  \returns Result; fails if:
      * The current thread is not that which the UThread instance represents; or
      * The thread's state is already kExiting.
  */
  evo::Result SetSelfExiting ();

  /** Enter into an interruptible sleep of the specified maximum duration.
      This can only be invoked by the thread associated with the UThread.
      The thread is woken if a new state is requested before or while sleeping.
      Notably, it follows that the sleep period is interrupted when the
      UThread is deleted, as its state is automatically changed to kExiting
      during destruction.
  \param max_duration Desired sleep duration.  Actual duration may be less if
      interrupted by a thread state change.  If this is duration::min(), then
      the thread remains asleep until interrupted; note that in this case,
      SUCCESS is never returned -- the function returns INTERRUPTED_OPERATION
      if no error occurs.
  \returns Result; fails if:
      * A new state is requested before or while sleeping; or
      * The UThread destructor is invoked (as this constitutes a thread state
        change to kExiting).
      * NOTE: Returns INTERRUPTED_OPERATION if max_duration == duration::min()
              (indefinite sleep period) and no error occurs.
  */
  evo::Result Sleep (evo::Duration max_duration);

  enum class ProcStateResult
  {
    kContinue, ///< Thread should continue (execute the current cycle)
    kExit      ///< Thread should exit immediately (break from outermost loop)
  };

  /** To be called by the UThread's exec function at the top of its main loop.
      If the cycle wait period has been set and no new state is pending,
      then this period, then this period elapses as per the cycle wait
      type before the call returns.  If a new state is requested while
      waiting, then the thread is woken and the new state is processed
      before ProcState() returns.
      \returns true if the thread should continue normally, or false if the
          UThread state is kExiting, and as such the thread should return
          from its function ASAP.
          The idea is to have something like:
          while (uthread->ProcState()) {
            DoJob();
            MakeWaffles(2*n_people + 5*n_pandas);
          }
          return SUCCESS;
   */
  ProcStateResult ProcState ();

  std::string ToString () const;

protected:

  /** This version which doesn't require a thread name is protected because it
      is my belief that every thread should have SOME name associated with it!
   */
  explicit UThread (ThreadExecFunc thread_func);

private:

  class RequestStateMultipleHelper
  {
  public:

    RequestStateMultipleHelper ( UThread* uthread, State newstate,
        std::mutex* failures_mutex, std::vector<StateChangeFail>* failures_out );

    UThread* uthread () {
      return uthread_;
    }
    evo::Result Start ();
    evo::Result Join ();

  private:

    void _threadFunc ();

    UThread* uthread_;
    State newstate_;
    std::mutex* failures_mutex_;
    std::vector<StateChangeFail>* failures_out_;
    std::unique_ptr<boost::thread> helper_thread_;
  };

  void Init ();

  void InitThread (evo::opt::Blocking block);

  void ConsiderPause_locked ();

  void RequestStateMultiple_Prepare (int* pending_count,
                                     std::mutex* go_mutex,
                                     std::condition_variable* go_cond);

  void SetStateInternal (State new_state);

  void JoinInternal ();

  /** Performs the assignment *internal_var_p = new_val within the context
      of state_ready_mutex_.
      Inline because the template arg is one of several possibilities; not
      worth explicitly instantiating w/ specific args in the cpp.
   */
  template <typename InternalVarType>
  void ThreadSafeSetVar (InternalVarType* internal_var_p,
                         const InternalVarType& new_val) {
    bool use_lock = !have_state_lock();
    if (use_lock) LockState();
    *internal_var_p = new_val;
    if (use_lock) UnlockState();
  }

  /** boost::thread doesn't require this to be static!  Awesome.
   Special naming because this method is only indirectly invoked by the new thread.
   */
  void _mainThreadFunc ();

  /** It's redundant to internally store the thread's name and also globally
   register it, but the advantage is, the UThread will remember its name
   even after the thread process exits.
   */
  std::string name_;

  /** Instance of the underlying thread implementation.
   We use boost::thread because, unlike std::thread, the boost::thread
   outermost function wrapper doesn't catch exceptions, which is highly
   desirable behavior when debugging, as the stack is otherwise totally unwound
   and the call trace lost.
 */
  std::shared_ptr<boost::thread> thread_;

  /** Unique thread-specific PID as given by syscall(SYS_gettid)
      (not necessarily the thing returned by thread::get_id()
      or thread::native_handle())
  */
  pid_t lwpid_;

  /** Thread cycle counter, incremented for each call to ProcState().  This is
      typically the # iterations of the thread's outermost loop.
  */
  int64_t cycle_count_;

  /** Current, effective state of the UThread.
   */
  State state_;

  /** Pending state of the UThread, not yet applied.  \p state_ will change to
      \p requested_state_ when \p requested_state_ takes effect.
      When no state is pending, \p requested_state_ is kInvalid.
   */
  State requested_state_;

  /** UThread state immediately preceding the current state
   */
  State prev_state_;

  /** Original state of UThread prior to activation of the cycle wait skip
      feature via RunNCycles().
  */
  State cycle_wait_skip_orig_state_;

  std::mutex cycle_wait_mutex_;
  CycleWait  cycle_wait_type_;

  /** Span of time that elapses for every call to ProcState() when
      requested_state_ is kInvalid.  The exact interpretation of this
      period depends on the value of cycle_wait_type_.
   */
  evo::Duration cycle_wait_period_;
  bool    cycle_wait_changed_;
  int64_t cycle_wait_skip_count_;

  evo::TimePoint prev_cycle_time_point_;

  /** state_ready_mutex protects state_ready_cond, state_ready_none_waiting_cond,
  state_ready_wait_count, state, requested_state, cycle_wait_skip_orig_state,
  and flags (bool is_*).
  */
  std::mutex      state_ready_mutex_;
  ThreadId state_ready_mutex_owner_id_;
  std::condition_variable state_ready_cond_;
  std::condition_variable state_ready_none_waiting_cond_;
  int state_ready_wait_count_;

  /** Times at which the UThread most recently assumed each state
   */
  std::unordered_map <int, evo::TimePoint> state_ready_time_points_;

  /** Like state_ready_cond, but these are state-specific, whereas
  state_ready_cond is general (state_ready_cond is broadcast whenever the state
  changes from anything to anything)
  */
  std::condition_variable go_ready_cond_;
  std::condition_variable idle_ready_cond_;

  /** Bitflags!  No, this actually counts how many different national flags
   have been accumulated.
   */
  // Replaced with individual booleans for improved readability: int flags_;
  bool is_between_cycles_;
  bool is_joined_with_thread_;
  bool is_pause_pending_;
  bool is_paused_;

  /** Used when waking from kIdle
  */
  std::condition_variable go_cond_;

  /** Used when waiting for some # of cycles to elapse in RunNCycles()
  */
  std::condition_variable cycle_wait_skip_advance_cond_;

  /** Broadcast when the UThread becomes paused.
  Protected by state_ready_mutex.
  */
  std::condition_variable paused_cond_;

  /** Signalled when the UThread should unpause itself.
  Protected by state_ready_mutex.
  */
  std::condition_variable unpause_cond_;

  std::mutex state_change_listeners_mutex_;

  evo::ListenerHandle next_state_change_listener_handle_;

  std::unordered_map <evo::ListenerHandle, StateChangeListenerFunc>
      state_change_listeners_;

  ThreadExecFunc user_thread_func_;

  RequestStateMultipleInfo set_state_multiple_info_;

  evo::Result thread_func_result_;

  bool enable_thread_wrapper_log_messages_;

  /** Checked exclusively by debug assertions; helps to ensure that a 'pause'
   actually occurred following a pause request.
   TODO: Remove this eventually.
   */
  int dbg_paused_count_;

  /* DELETE: These are now covered by RegisterStateChangeListener()
  * psThreadCallback idle_func_;
  * void*            idle_func_data_;
  *
  * psThreadCallback go_func_;
  * void*            go_func_data_;
  *
  * // Each of these callbacks is invoked when the thread exits.
  * // The type of each node's data is _ExitCallbackInfo.
  * GQueue* exit_callback_queue_;
  */
};

/** WARNING: Retrieving an unlocked UThread is generally a bad idea, as it
 could become freed before the caller has a chance to access it, except of
 course when the returned UThread is associated with the CURRENT thread.
 */
UThread* GetUThread (ThreadId id, bool acquire_state_lock);

UThread* GetThisUThread ();

}

# if 0

//\\ THE RECYCLE BIN: Things to eventually be reimplemented OO-style or discarded. \\//

/** Prevents the UThread lib from joining with the respective GThread when
the UThread is freed.
Very rarely needed - because it is *IMPORTANT* that g_thread_join() be invoked
for every UThread's GThread (so the GLib will free all resources associated
with the GThread), it is almost always best to just let the UThread lib handle
the joining.  Use only in very special circumstances!
NOTE: this cannot be undone.
*/
void u_thread_prevent_internal_join (UThread* uthread);

# endif

#endif

