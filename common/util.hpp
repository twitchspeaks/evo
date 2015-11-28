#ifndef LIB_UTIL_HPP
#define LIB_UTIL_HPP

#include <stdlib.h>
#include <stdint.h>

#include <string>
#include <chrono>
#include <sstream>
#include <glog/logging.h>
#include <boost/thread.hpp>

#include "common/compat.hpp"
#include "common/string.hpp"

#define QLOG(level__) \
  LOG(level__) << "[" << evo::GetCurrentThreadName() << "]: "

#define QLOG_EVERY_N(level__, n__) \
  LOG_EVERY_N(level__, n__) << "[" << evo::GetCurrentThreadName() << "]: "

#define QLOG_IF(level__, condition__) \
  LOG_IF(level__, condition__) << "[" << evo::GetCurrentThreadName() << "]: "

namespace evo {

namespace opt {

  /** Intended for use by functions that accept an argument indicating whether
   or not to block the caller until the respective task(s) are complete.
  */
  enum class Blocking { kOff, kOn };
}

typedef uint8_t byte;

typedef boost::thread::id ThreadId;

typedef uint64_t ListenerHandle;

static const int INVAL_INDEX = -1;

/**
 * A define that makes invalidating file descriptors more explicit.
 */
static const int INVAL_FD = -1;

/** Necessary since std::*map does not natively support boost::thread::id as
 * the key type.
 * This approach was recommended by
 * http://stackoverflow.com/questions/2850142/how-can-i-use-boostthreadid-as-key-to-an-unordered-map
 */
struct ThreadIdHash
{
  size_t operator () (const ThreadId& id) const {
    std::ostringstream os;
    os << id;
    return std::hash<std::string>()(os.str());
  }
};

/**
 * A string hash functor that ignores case.
 */
struct CaseInsensitiveStringHash {
  size_t operator () (const std::string& str) const {
    return std::hash<std::string>()(StrToLower(str));
  }
};

/**
 * A string comparer functor that ignores case.
 */
struct CaseInsensitiveStringComparer {
  bool operator () (const std::string &lhs, const std::string &rhs) const {
    return StrToLower(rhs) == StrToLower(lhs);
  }
};

/**
 * Convenience function to convert from kOff / kOn, KFalse / kTrue, etc option
 * enums to booleans.
 * Note that this assumes the "false" enum class member is the first, i.e., has
 * an integral value of zero.
 * @param val {The option enum value which should be either kOn or
 * something else.}
 * @return The boolean equivalent of \p val.
 */
template <class T>
bool BooleanEnumToBool (T val) {
  return (0 != static_cast<int>(val));
}

/**
 * Convenience function to convert from kOff / kOn, KFalse / kTrue, etc option
 * enums to a string.
 * Note that this assumes the "false" enum class member is the first, i.e., has
 * an integral value of zero.
 * @param val The true/false option enum value.
 * @return A string representation of \p val.
 */
template <class T>
std::string BooleanEnumToString (T val) {
  return (static_cast<int>(val) ? "true" : "false");
}

/**
 * Convenience function to convert from booleans to kOn / kOff
 * enums. This might be obvious, but you'll have to explicitly
 * specify the type T.
 * Note that this assumes the "false" enum class member is the first, i.e., has
 * an integral value of zero.
 * @param val The boolean being converted.
 * @return The enum value that corresponds to \p val.
 */
template <class T>
T BoolToBooleanEnum (bool val) {
  return static_cast<T>(val);
}

/**
 * Registers the thread with a global map so its name can be later
 * looked up by thread id.
 * @param name The name that will be assigned to the calling thread.
 */
void RegisterCurrentThreadName (const std::string& name);

void UnregisterThread (ThreadId id);

/**
 * Retrieves the name of the calling thread.
 * @return The calling thread's name. If this thread's id was not registered
 * by calling RegisterCurrentThreadName, an empty string is returned.
 */
std::string GetCurrentThreadName ();

/**
 * Retrieves the name of a thread with a particular thread id.
 * @param id The id of the thread whose name is being retrieved.
 * @return The thread's name, if \p id was previously registered
 * by calling RegisterCurrentThreadName, otherwise an empty
 * string is returned.
 */
std::string GetThreadName (ThreadId id);

/**
 * Checks to see if a thread with \p id has been registered with
 * RegisterCurrentThreadName.
 * @param id The thread id that is being checked.
 * @return True if the thread id has been registered with the name
 * map, false otherwise.
 */
bool IsThreadNameRegistered (ThreadId thread_id);

/** @return The thread ID of the current thread.
 */
ThreadId GetThisThreadId ();

/** @return The LWPID (lightweight PID, a unique PID assigned to every thread of
 a parent process) of the current thread.
 */
pid_t GetCurrentThreadLwpid ();

/** @return String representation of the time given by \p tp
 */
std::string SystemTimeToString (
    const std::chrono::system_clock::time_point& tp );

/** @return String representation of the current system time
 */
std::string CurrentSystemTimeToString ();


class Coords3
{
public:

  Coords3 (float _x = 0, float _y = 0, float _z = 0)
  : x(_x), y(_y), z(_z)
  {}

  std::string ToString () const {
    return std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z);
  }

  float x;
  float y;
  float z;
};

}

#endif

