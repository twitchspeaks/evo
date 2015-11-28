#ifndef LIB_TIME_MEASURES_HPP
#define LIB_TIME_MEASURES_HPP

#include <chrono>
#include <ratio>
#include <string>
#include <cstdint>

#include "common/i_stringable.hpp"
#include "common/compat.hpp"

namespace evo {

/** Represents an interval of time by wrapping a std::chrono::duration object.
 */
class Duration : public IStringable
{
public:

  /** The std::chrono::duration type used for underlying storage. This stores
   * the time interval as a 64-bit number of nanoseconds.
   */
  typedef std::chrono::duration<std::int64_t, std::nano> StdDuration;

  Duration ();

  /** @param nanoseconds A number of nanoseconds.
   */
  explicit Duration (int64_t nanoseconds);

  /** @param std_duration A standard duration object.
   */
  Duration (StdDuration std_duration);

  Duration (const Duration &rhs);

  /** Retrieves a copy of the underlying std::chrono::duration instance.
   */
  StdDuration std_duration () const;

  virtual std::string ToString () const override;

  /** Retrieves a human-readable string representation of the time interval.
   */
  std::string ToStringPretty () const;

  /** Retrieves a standard-form string representation of the time interval.
   * This is in units of seconds, and will always be exactly convertible to an
   * integral number of nanoseconds.
   */
  std::string ToStringParsable () const;

  /** Retrieves the duration in nanoseconds.
   */
  std::int64_t Nanoseconds () const;

  /** Retrieves the duration in microseconds.
   */
  double Microseconds () const;

  /** Retrieves the duration in milliseconds.
   */
  double Milliseconds () const;

  /** Retrieves the duration in seconds.
   */
  double Seconds () const;

  /** Creates a new instance from a number of nanoseconds.
   * @param nanoseconds A time duration, in nanoseconds.
   * @return The new instance.
   */
  static Duration FromNanoseconds (int64_t nanoseconds);

  /** Creates a new instance from a number of microseconds.
   * @param microseconds A time duration, in microseconds.
   * @return The new instance.
   */
  static Duration FromMicroseconds (double microseconds);

  /** Creates a new instance from a number of milliseconds.
   * @param milliseconds A time duration, in milliseconds.
   * @return The new instance.
   */
  static Duration FromMilliseconds (double milliseconds);

  /** Creates a new instance from a number of seconds.
   * @param seconds A time duration, in seconds.
   * @return The new instance.
   */
  static Duration FromSeconds (double seconds);

  /** Creates a new instance from a number of minutes.
   * @param minutes A time duration, in minutes.
   * @return The new instance.
   */
  static Duration FromMinutes (double minutes);

  /** Returns the smallest time interval that can be represented by
   * the underlying std::chrono::duration. This will be a rather large
   * negative number.
   */
  static Duration Min ();

  inline Duration& operator = (const Duration& rhs) {
    if(this == &rhs) {
      return *this;
    }

    std_duration_ = rhs.std_duration_;
    return *this;
  }

  inline Duration operator+() const {
    return Duration(std_duration_);
  }

  inline Duration operator-() const {
    return Duration(-std_duration_);
  }

  inline Duration operator+(const Duration &rhs) const {
    return Duration(std_duration_ + rhs.std_duration_);
  }

  inline Duration operator-(const Duration &rhs) const {
    return Duration(std_duration_ - rhs.std_duration_);
  }

  inline Duration operator*(int64_t rhs) const {
    return Duration(std_duration_ * rhs);
  }

  inline Duration operator/(int64_t rhs) const {
    return Duration(std_duration_ / rhs);
  }

  inline Duration operator%(Duration rhs) const {
    return Duration(std_duration_ % rhs.std_duration_);
  }

  inline double operator/(const Duration &rhs) const {
    return std_duration_.count() /
        static_cast<long double>(rhs.std_duration_.count());
  }

  inline Duration &operator+=(const Duration &rhs) {
    std_duration_ += rhs.std_duration_;
    return *this;
  }

  inline Duration &operator-=(const Duration &rhs) {
    std_duration_ -= rhs.std_duration_;
    return *this;
  }

  inline Duration &operator*=(int64_t rhs) {
    std_duration_ *= rhs;
    return *this;
  }

  inline Duration &operator*=(double rhs) {
    std_duration_ *= rhs;
    return *this;
  }

  inline Duration &operator/=(int64_t rhs) {
    std_duration_ /= rhs;
    return *this;
  }

  inline bool operator==(const Duration &rhs) const {
    return std_duration_ == rhs.std_duration_;
  }

  inline bool operator!=(const Duration &rhs) const {
    return std_duration_ != rhs.std_duration_;
  }

  inline bool operator<(const Duration &rhs) const {
    return std_duration_ < rhs.std_duration_;
  }

  inline bool operator<=(const Duration &rhs) const {
    return std_duration_ <= rhs.std_duration_;
  }

  inline bool operator>(const Duration &rhs) const {
    return std_duration_ > rhs.std_duration_;
  }

  inline bool operator>=(const Duration &rhs) const {
    return std_duration_ >= rhs.std_duration_;
  }

private:

  StdDuration std_duration_;
};

class TimePoint : public IStringable
{
public:

  /** The system clock that will be used for the underlying
   * std::chrono::time_point instance.
   */
  typedef std::chrono::system_clock StdClock;

  /** The underlying std::chrono::time_point instance that will be used
   * to store the time. The time is represented in a 64-bit integral number
   * of nanoseconds since the unix epoch.
   */
  typedef std::chrono::time_point<StdClock, Duration::StdDuration> StdTimePoint;

  TimePoint();

  /** @param nanoseconds A number of nanoseconds since the unix epoch.
   */
  explicit TimePoint(std::int64_t nanoseconds);

  /** @param duration A time interval since the unix epoch.
   */
  explicit TimePoint(Duration duration);

  /** @param time_point A std::chrono::time_point instance.
   */
  explicit TimePoint(StdTimePoint time_point);

  TimePoint(const TimePoint &rhs);

  /** Retrieves a copy of the underlying std::chrono::time_point instance.
   */
  StdTimePoint std_time_point() const;

  virtual std::string ToString() const override;

  /** Retrieves a human-readable string representation of the time point.
   */
  std::string ToStringPretty() const;

  /** Retrieves a standard-form string representation of the time point.
   * This is always an integral number of nanoseconds since the unix epoch.
   */
  std::string ToStringParsable() const;

  /** Retrieves a string representation of the time point.
   * @param format A format string that will be passed to std::strftime
   * to create the string.
   * @return The string representation.
   */
  std::string ToString(const std::string &format) const;

  enum class IncludeDate { kOff, kOn };
  std::string ToStringWithNs(IncludeDate include_date = IncludeDate::kOn) const;

  /** Retrieves the time point as an integral number of nanoseconds since
   * the unix epoch.
   */
  std::int64_t Nanoseconds() const;

  /** Retrieves the time point as a floating point number of microseconds since
   * the unix epoch.
   */
  double Microseconds() const;

  /** Retrieves the time point as a floating point number of milliseconds since
   * the unix epoch.
   */
  double Milliseconds() const;

  /** Retrieves the time point as a floating point number of seconds since
   * the unix epoch.
   */
  double Seconds() const;

  /** Retrieves an instance that represents the current system time.
   */
  static TimePoint Now();

  /** Retrieves an instance that represents the smallest possible time point
   * that can be represented by the underlying std::chrono::time_point type.
   */
  static TimePoint Min();

  /** Retrieves an instance that represents the largest possible time point
   * that can be represented by the underlying std::chrono::time_point type.
   */
  static TimePoint Max();

  inline TimePoint &operator+=(const Duration &rhs) {
    std_time_point_ += rhs.std_duration();
    return *this;
  }

  inline TimePoint &operator-=(const Duration &rhs) {
    std_time_point_ -= rhs.std_duration();
    return *this;
  }

  inline TimePoint operator+(const Duration &rhs) const {
    return TimePoint(std_time_point_ + rhs.std_duration());
  }

  inline TimePoint operator-(const Duration &rhs) const {
    return TimePoint(std_time_point_ - rhs.std_duration());
  }

  inline Duration operator-(const TimePoint &rhs) const {
    return Duration(std_time_point_ - rhs.std_time_point_);
  }

  inline bool operator==(const TimePoint &rhs) const {
    return std_time_point_ == rhs.std_time_point_;
  }

  inline bool operator!=(const TimePoint &rhs) const {
    return std_time_point_ != rhs.std_time_point_;
  }

  inline bool operator<(const TimePoint &rhs) const {
    return std_time_point_ < rhs.std_time_point_;
  }

  inline bool operator<=(const TimePoint &rhs) const {
    return std_time_point_ <= rhs.std_time_point_;
  }

  inline bool operator>(const TimePoint &rhs) const {
    return std_time_point_ > rhs.std_time_point_;
  }

  inline bool operator>=(const TimePoint &rhs) const {
    return std_time_point_ >= rhs.std_time_point_;
  }

  static TimePoint AverageTimePoints(TimePoint first, TimePoint second);

private:

  StdTimePoint std_time_point_;
};

inline Duration operator*(int64_t lhs, const Duration &rhs) {
  return rhs * lhs;
}

#if 0
/** The result of frequency * time is a cycle count.
 */
inline double operator*(const Frequency &lhs, const Duration &rhs) {
  return lhs.Hz() * rhs.Seconds();
}
inline double operator*(const Duration &lhs, const Frequency &rhs) {
  return lhs.Seconds() * rhs.Hz();
}

inline TimePoint operator+(const Duration &lhs, const TimePoint &rhs) {
  return TimePoint(lhs.std_duration() + rhs.std_time_point());
}
#endif

}

#endif

