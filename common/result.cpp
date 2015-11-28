#include <string>
#include <sstream>
#include <cstring>

#include "common/result.hpp"

using namespace std;
using namespace evo;
using namespace std_results;

Result :: Result ()
{
  Init(DEFAULT_ERROR_CODE);
}

Result :: Result (const Result& copy_src)
{
  operator=(copy_src);
}

Result :: Result (int code)
{
  Init(code);
}

Result :: Result (int code, const string& message)
{
  Init(code, message);
}

Result :: ~Result ()
{
}

void Result :: Init (int code)
{
  code_ = code;
}

void Result :: Init (int code, const string& message)
{
  code_ = code;
  message_.reset(new string(message));
}

Result& Result :: operator = (const Result& copy_src)
{
  code_ = copy_src.code_;

  if (copy_src.message_) {
    set_message(*copy_src.message_);
  }
  else {
    message_.reset();
  }

  return *this;
}

bool Result :: operator == (const Result& arg) const
{
  // deliberately ignoring message if either code has been set; this facilitates
  // intuitive evaluation of code such as:
  // Result res = file.Read(&buf, 222);
  // if (NOT_OPEN == res) ...

  if (DEFAULT_ERROR_CODE == arg.code_ && DEFAULT_ERROR_CODE == code_) {
    return ( (!arg.message_ && !message_) ||
             (arg.message_ && message_ && *arg.message_ == *message_) );
  }
  else {
    return (arg.code_ == code_);
  }
}

bool Result :: operator != (const Result& arg) const
{
  return !operator==(arg);
}

bool Result :: is_error () const
{
  return (INVALID_RESULT_CODE != code_ && SUCCESS_CODE != code_);
}

bool Result :: is_success () const
{
  return (SUCCESS_CODE == code_);
}

Result& Result :: FromErrno (const string& leading_msg)
{
  return FromErrno(errno, leading_msg);
}

Result& Result :: FromErrno (int errno_code, const string& leading_msg)
{
  code_ = errno_code;

  if (leading_msg.empty()) {
    set_message(strerror(errno_code));
  }
  else {
    set_message((leading_msg + ": ") + strerror(errno_code));
  }

  return *this; // for convenience (e.g. "return Result().FromErrno();")
}

void Result :: Clear ()
{
  code_ = DEFAULT_ERROR_CODE;
  if (message_) {
    message_->clear();
  }
}

Result& Result :: Prepend (const string& msg)
{
  if (has_message()) {
    set_message(msg + ": " + *message_);
  }
  else {
    set_message(msg);
  }
  return *this;
}

Result Result :: Prepend (const string& msg) const
{
  if (has_message()) {
    return Result(code_, msg + ": " + *message_);
  }
  else {
    return Result(code_, msg);
  }
}

Result& Result :: Append (const string& msg)
{
  if (has_message()) {
    set_message(*message_ + ": " + msg);
  }
  else {
    set_message(msg);
  }
  return *this;
}

Result Result :: Append (const string& msg) const
{
  if (has_message()) {
    return Result(code_, *message_ + ": " + msg);
  }
  else {
    return Result(code_, msg);
  }
}

string Result :: ToString () const
{
  if (code_ != DEFAULT_ERROR_CODE)
  {
    stringstream strm;
    if (has_message())
    {
      const string& msg = message();
      strm << msg;
      char endchar = (msg.empty() ? '\0' : *(msg.end()-1));
      if (!std::isspace(endchar)) {
        strm << " ";
      }
      strm << "(errcode " << code_ << ": "<< ErrorCodeToString(code_) << ")";
    }
    else
    {
      strm << "errcode " << code_ << ": " << ErrorCodeToString(code_);
    }
    return strm.str();
  }
  else if (has_message())
  {
    return *message_;
  }

  return "<uninitialized>";
}

string Result::ErrorCodeToString (int code)
{
  switch (code) {
    case INVALID_RESULT_CODE:
      return "Invalid result (software bug?)";
    case SUCCESS_CODE:
      return "Success";
    case POOL_EMPTY_CODE:
      return "Pool is empty";
    case NOT_CONNECTED_CODE:
      return "Not connected";
    case ALREADY_CONNECTED_CODE:
      return "Already connected";
    case CONNECTION_FAILED_CODE:
      return "Connection failed";
    case INDEX_OUT_OF_RANGE_CODE:
      return "Index out of range";
    case TIMED_OUT_CODE:
      return "Timed out";
    case NOT_ENABLED_CODE:
      return "Not enabled";
    case ALREADY_MAPPED_CODE:
      return "The specified index has already been mapped";
    case ENABLED_CODE:
      return "The operation cannot be completed while the object is enabled";
    case INDEX_NOT_FOUND_CODE:
      return "Index not found";
    case INSUFFICIENT_DATA_CODE:
      return "Insufficient data";
    case NIL_DATA_CODE:
      return "No data";
    case INSUFFICIENT_SPACE_CODE:
      return "Insufficient space";
    case ILLEGAL_MAPPING_CODE:
      return "Illegal mapping";
    case NOT_MAPPED_CODE:
      return "Not mapped";
    case SINGLE_THREADED_CODE:
      return "Single threaded only";
    case THREAD_RESTRICTION_CODE:
      return "Forbidden by thread restriction";
    case NOT_REGISTERED_CODE:
      return "Not registered";
    case ALREADY_REGISTERED_CODE:
      return "Already registered";
    case NOT_INIT_CODE:
      return "Not initialized";
    case ILLEGAL_OPERATION_CODE:
      return "Illegal operation";
    case BAD_CONFIG_CODE:
      return "Bad configuration";
    case FILE_MANIP_FAILED_CODE:
      return "File manipulation failed";
    case OPEN_FAILED_CODE:
      return "Failed to open resource";
    case MMAP_FAILED_CODE:
      return "mmap() failed";
    case NOT_OPEN_CODE:
      return "Resource is not open";
    case ALREADY_OPEN_CODE:
      return "Resource is already open";
    case OUT_OF_MEM_CODE:
      return "Out of memory";
    case COMM_ERROR_CODE:
      return "Communication error";
    case INVALID_ARGUMENT_CODE:
      return "Invalid argument";
    case SHUTTING_DOWN_CODE:
      return "Resource is shutting down";
    case DEADLOCK_AVERTED_CODE:
      return "Deadlock averted";
    case NOT_IMPLEMENTED_CODE:
      return "Function not implemented";
    case STATE_ALREADY_EFFECTIVE_CODE:
      return "State is already effective";
    case RESOURCE_UNAVAILABLE_CODE:
      return "Resource is unavailable";
    case INTERRUPTED_OPERATION_CODE:
      return "Operation was interrupted";
    case NOT_READY_CODE:
      return "Resource is not ready";
    case VALUE_OUT_OF_RANGE_CODE:
      return "Numeric value is out of range";
    case VALUE_INVALID_CODE:
      return "Numeric value is not valid";
    case WRITE_FAILED_CODE:
      return "Write failed";
    case IO_ERROR_CODE:
      return "I/O error";
    case READ_ONLY_CODE:
      return "Read only";
    case RESOURCES_ALREADY_RESERVED_CODE:
      return "Resources are already reserved";
    case NO_POOLS_ALLOCATED_CODE:
      return "Pools have not been allocated yet";
    case NOT_FOUND_CODE:
      return "Not found";
    case PARSE_FAILED_CODE:
      return "Parse failed";
    case VALIDATION_FAILED_CODE:
      return "Validation failed";
    case NOT_RUNNING_CODE:
      return "Process/resource is not running";
    case CONFIG_TRANSITIONING_CODE:
      return "Config transitioning";
    case INVALID_SIZE_CODE:
      return "Size is invalid";
    case BAD_DATA_CODE:
      return "Bad data";
    case INTERRUPTED_CODE:
      return "Interrupted";
    case READ_FAILED_CODE:
      return "Read failed";
  }

  return strerror(code);
}
