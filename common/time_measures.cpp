#include <ctime>
#include <sstream>
#include <iomanip>

#include "common/time_measures.hpp"

using namespace evo;
using namespace std;
using namespace std::chrono;

Duration :: Duration ()
    : std_duration_(0) { }

Duration :: Duration (int64_t nanoseconds)
    : std_duration_(nanoseconds) { }

Duration :: Duration (Duration::StdDuration std_duration)
    : std_duration_(std_duration) { }

Duration :: Duration (const Duration &rhs)
    : std_duration_(rhs.std_duration_) { }

Duration::StdDuration Duration :: std_duration () const
{
  return std_duration_;
}

string Duration :: ToString () const
{
  return ToStringPretty();
}

std::string Duration :: ToStringPretty () const
{
  int64_t ns = Nanoseconds();
  stringstream stream;

  if(std::abs(ns) < 1e3) {
    stream << ns << " ns";
  } else if(std::abs(ns) < 1e6) {
    stream << Microseconds() << " us";
  } else if(std::abs(ns) < 1e9) {
    stream << Milliseconds() << " ms";
  } else {
    stream << Seconds() << " s";
  }

  return stream.str();
}

std::string Duration :: ToStringParsable () const
{
  stringstream stream;

  stream << std::fixed << std::setprecision(9) << Seconds();

  return stream.str();
}

int64_t Duration :: Nanoseconds () const
{
  return std_duration_.count();
}

double Duration :: Microseconds () const
{
  return static_cast<double>(std_duration_.count()) / 1e3;
}

double Duration :: Milliseconds () const {
  return static_cast<double>(std_duration_.count()) / 1e6;
}

double Duration::Seconds() const {
  return static_cast<double>(std_duration_.count()) / 1e9;
}

Duration Duration::FromNanoseconds(int64_t nanoseconds) {
  return Duration(nanoseconds);
}

Duration Duration::FromMicroseconds(double microseconds) {
  return Duration(static_cast<int64_t>(microseconds * 1e3));
}

Duration Duration::FromMilliseconds(double milliseconds) {
  return Duration(static_cast<int64_t>(milliseconds * 1e6));
}

Duration Duration::FromSeconds(double seconds) {
  return Duration(static_cast<int64_t>(seconds * 1e9));
}

Duration Duration::FromMinutes(double minutes) {
  return Duration(static_cast<int64_t>(minutes * 60e9));
}

Duration Duration::Min() {
  return Duration(StdDuration::min());
}


/**
 Note: std::chrono::system_clock::now() executes very quickly; calling it in
 the default ctor shouldn't impose significant overhead.
 */
TimePoint::TimePoint()
    : std_time_point_(StdClock::now()) { }

TimePoint::TimePoint(int64_t nanoseconds)
    : std_time_point_(Duration::StdDuration(nanoseconds)) { }

TimePoint::TimePoint(Duration duration)
    : std_time_point_(duration.std_duration()) { }

TimePoint::TimePoint(StdTimePoint std_time_point)
    : std_time_point_(std_time_point) { }

TimePoint::TimePoint(const TimePoint &rhs)
    : std_time_point_(rhs.std_time_point_) { }

TimePoint::StdTimePoint TimePoint::std_time_point() const {
  return std_time_point_;
}

string TimePoint::ToString() const {
  return ToStringPretty();
}

std::string TimePoint::ToStringPretty() const {
  return ToString("%c");
}

std::string TimePoint::ToStringParsable() const {
  stringstream strm;

  time_t time = StdClock::to_time_t(
      time_point_cast<StdClock::duration>(std_time_point_));

  strm << std::fixed << time;

  return strm.str();
}

string TimePoint::ToString(const string &format) const {
  static const int BUFFER_SIZE = 128;
  char buffer[BUFFER_SIZE];

  time_t time = StdClock::to_time_t(
      time_point_cast<StdClock::duration>(std_time_point_));
  tm *timeinfo = gmtime(&time);

  if(strftime(buffer, BUFFER_SIZE, format.c_str(), timeinfo) == 0) {
    //the expanded string exceeded BUFFER_SIZE, returning an empty
    //string for safety purposes
    buffer[0] = '\0';
  }

  return string(buffer);
}

string TimePoint::ToStringWithNs(IncludeDate include_date) const {
  stringstream strm;
  if (include_date == IncludeDate::kOn) {
    strm << ToString("%Y-%m-%d %H:%M:%S.");
  } else {
    strm << ToString("%H:%M:%S.");
  }
  if (Nanoseconds() < 0) {
    strm << "(negative ns?)";
  } else {
    strm << setfill('0') << setw(9) << (Nanoseconds() % 1000000000LL);
  }

  return strm.str();
}

int64_t TimePoint::Nanoseconds() const
{
  auto converted_tp = chrono::time_point_cast<
      chrono::duration<int64_t, std::nano>, StdClock, Duration::StdDuration> (
          std_time_point_);

  return converted_tp.time_since_epoch().count();
}

double TimePoint::Microseconds() const
{
  auto converted_tp = chrono::time_point_cast<
      chrono::duration<double, std::micro>, StdClock, Duration::StdDuration> (
          std_time_point_);

  return converted_tp.time_since_epoch().count();
}

double TimePoint::Milliseconds() const
{
  auto converted_tp = chrono::time_point_cast<
      chrono::duration<double, std::milli>, StdClock, Duration::StdDuration> (
          std_time_point_);

  return converted_tp.time_since_epoch().count();
}

double TimePoint::Seconds() const
{
  auto converted_tp = chrono::time_point_cast<
      chrono::duration<double, std::ratio<1, 1> >, StdClock, Duration::StdDuration> (
          std_time_point_);

  return converted_tp.time_since_epoch().count();
}

TimePoint TimePoint::Now() {
  return TimePoint();
}

TimePoint TimePoint::Min() {
  return TimePoint(StdTimePoint::min());
}

TimePoint TimePoint::Max() {
  return TimePoint(StdTimePoint::max());
}

TimePoint TimePoint::AverageTimePoints(TimePoint first, TimePoint second) {
  int64_t first_ticks = first.std_time_point().time_since_epoch().count();
  int64_t second_ticks = second.std_time_point().time_since_epoch().count();

  first_ticks /= 2;
  second_ticks /= 2;

  return TimePoint(first_ticks + second_ticks);
}

