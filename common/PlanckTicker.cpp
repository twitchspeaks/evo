#include "common/result.hpp"
#include "common/thread.hpp"
#include "common/PlanckTicker.hpp"

using namespace evo;
using namespace std_results;

PlanckTicker :: PlanckTicker (Duration tick_interval)
  : tick_interval_(tick_interval),
    start_time_(0)
{
}

PlanckTicker :: ~PlanckTicker ()
{
  Stop();
}

Result PlanckTicker :: Start ()
{
  if (thread_) {
    return STATE_ALREADY_EFFECTIVE.Prepend(
        "Thread has already been started; call Stop() first");
  }

  thread_.reset( new UThread("PlanckTicker",
      std::bind(&PlanckTicker::ThreadFunc, this, std::placeholders::_1)) );

  thread_->set_cycle_wait_type(UThread::CycleWait::kAbsolute);
  thread_->set_cycle_wait_period(tick_interval_);

  return thread_->Start();
}

Result PlanckTicker :: Stop ()
{
  thread_.reset();
}

Result PlanckTicker :: ThreadFunc (UThread* uthread)
{
  Result res;

  int tick_index = 0;

  start_time_ = TimePoint::Now();

  while (UThread::ProcStateResult::kContinue == uthread->ProcState())
  {
    TimePoint tick_real_time (TimePoint::Now() - start_time_);
    TimePoint tick_virt_time = tick_index * tick_interval_;

    if (SUCCESS != (res = tick_handler_func_(
            tick_index, tick_virt_time, tick_real_time))) {
      return res.Prepend("Planck tick handler failed");
    }

    ++tick_index;
  }

  return SUCCESS;
}

