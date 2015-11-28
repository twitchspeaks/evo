#include <memory>
#include "common/util.hpp"
#include "common/thread.hpp"

namespace evo {

/** Triggers rendering lib's display() at regular intervals
 */
class PlanckTicker
{
public:

  /** Args: int tick_count, Duration virtual_time, Duration real_time
  */
  typedef std::function<Result (int, Duration, Duration)>
      TickHandlerFunc;

  PlanckTicker (Duration tick_interval, TickHandlerFunc func);

  virtual ~PlanckTicker ();

  TimePoint start_time () const {
    return start_time_;
  }

  Result Start ();

  Result Stop ();

private:

  Duration tick_interval_;
  Duration start_time_;
  std::shared_ptr<UThread> thread_;
  TickHandlerFunc tick_handler_func_;
};

}
