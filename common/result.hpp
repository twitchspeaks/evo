#ifndef COMMON_RESULT_HPP
#define COMMON_RESULT_HPP

#include <string>
#include <memory>
#include "common/i_stringable.hpp"
#include "common/i_cloneable.hpp"

namespace evo {

/** Represents the result of an operation. Can either be success, or one of
 * a variety of error codes. Also contains a message string that can
 * either be a default value or a custom string particular to the error
 * that occurred.
 */
class Result : public IStringable, public ICloneable
{
public:

  /** Constructs a result with code DEFAULT_ERROR_CODE.
   */
  Result ();

  /** Constructs a result by copying the code and message from \p copy_src.
   * @param copy_src the Result object that will be copied.
   */
  Result (const Result& copy_src);

  /** Constructs a Result object with result code \p code and the default
   * message string.
   * @param code The result code the new object will use.
   */
  explicit Result (int code);

  /** Constructs a Result object with result code \p code and custom message
   * \p message.
   * @param code The result code the new object will use.
   * @param message The custom message string the new object will use.
   */
  Result (int code, const std::string& message);

  virtual ~Result ();

  /** Copies the result code and message string from \p copy_src into the
   * object.
   * @param copy_src The Result object that will be copied.
   */
  Result& operator = (const Result& copy_src);

  /** Compares two Result objects for equality. This only takes into account the
   * code, except when both Results have the default error code. In this case
   * only the messages are used to determine equality.
   * @param arg The other Result.
   * @return True if the two Result instances are considered equal.
   */
  bool operator == (const Result& arg) const;

  /** Equivalent to !operator==
   */
  bool operator != (const Result& arg) const;

  /** Gets the object's result code.
   * @return The object's result code.
   */
  int code () const {
    return code_;
  }

  // TODO: can't return a const string& because if nullptr == message_, the
  //       result is an implied conversion from const char* to string, which
  //       is a temporary instance, and therefore cannot be returned as a reference;
  //       would it be better to return const char* here, or a std::string copy?
  /** Gets a copy of the object's message string.
   * @return the message string copy.
   */
  std::string message () const
  {
    if (message_) {
      return *message_;
    }
    else {
      return ErrorCodeToString(code_);
    }
  }

  /** Sets the Result instance's message string.
   * @param message The new message string.
   */
  void set_message (const std::string& message)
  {
    if (message_) {
      message_->assign(message);
    }
    else {
      message_.reset(new std::string(message));
    }
  }

  /** Whether or not the Result instance has a custom message.
   * @return True if a custom message has been set, false if the default message
   * is being used.
   */
  bool has_message () const {
    return (message_ && !message_->empty());
  }

  /** Whether or not the Result indicates an error.
   * @return True if the result code is not invalid or success, false otherwise.
   */
  bool is_error () const;

  /** Whether or not the Result indicates success.
   * @return True if the result code is success, false otherwise.
   */
  bool is_success () const;

  /** Populates the result code and message string from the value
   * of errno.
   * @param leading_msg A string that will be prepended to the
   * errno string acquired from strerror.
   * @return A reference to the instance.
   */
  Result& FromErrno (const std::string& leading_msg = "");

  /** Populates the result code and message string from an errno
   * value.
   * @param errno_code A code that was previously acquired from the value
   * of errno.
   * @param leading_msg A string that will be prepended to the errno
   * string acquired from strerror.
   * @return A reference to the instance.
   */
  Result& FromErrno (int errno_code, const std::string& leading_msg = "");

  /** Prepends a message to the beginning of the existing message string.
  * This is useful for modifying the message as the result goes up the
  * call stack.
  * @param msg The string that will be prepended to the error message.
  * @return A reference to the instance.
  */
  Result& Prepend (const std::string& msg);

  /** Creates a new Result that is equivalent to the instance with
   * a string prepended to the message.
  * @param msg The string that will be prepended to the error message.
  * @return A copy of the new Result.
  */
  Result Prepend (const std::string& msg) const;

  /** Appends a message to the end of the existing message string.
   * This is useful for modifying the message as the result goes up the
   * call stack.
   * @param msg The string that will be appended to the error message.
   * @return A reference to the instance.
   */
  Result& Append (const std::string& msg);

  /** Appends a message to the end of the existing message string
  * and creates a new Result instance to avoid modifying the instance.
  * @param msg The string that will be appended to the error message.
  * @return A copy of the new Result.
  */
  Result Append (const std::string& msg) const;

  /** Resets the error code and message string so the instance is equivalent
   * to a default-constructed Result.
   */
  void Clear ();

  std::string ToString () const;

  std::shared_ptr<ICloneable> Clone () const {
    return std::shared_ptr<ICloneable>(new Result(*this));
  }

  /** Gets the default message string for an error code.
   * @param error_code An error code.
   * @return The default string describing \p error_code.
   */
  static std::string ErrorCodeToString (int error_code);

protected:

private:

  void Init (int code);
  void Init (int code, const std::string& message);

  /** Numeric code associated with the error
   */
  int code_;

  /** Message associated with the error; note that this is initialized to and
      remains a nullptr until such time as it becomes necessary to store a
      textual message.  This is because Result objects are expected to be
      constructed very regularly, and as such minimizing overhead is important.
      Result instances that specify a built-in error code (such as NIL_DATA) but
      not a custom message can simply invoke ErrorCodeToString() on demand,
      rather than initialize message_ to the return of ErrorCodeToString().
   */
  std::shared_ptr<std::string> message_;
};

/** This adds ostream support, so that one can write something like:
    Result res;
    if (SUCCESS != (res = DoJob("fruit salad"))) {
      cout << "Couldn't do job: " << res << "\n";
 */
template <typename Traits>
inline std::basic_ostream<char, Traits>& operator << (
    std::basic_ostream<char, Traits>& ostrm, const Result& res )
{ return std::operator<<(ostrm, res.ToString()); }

}

/** So that .cpp files using any of the built-in results can simply declare:
    using namespace std_results;
    ... without polluting the local namespace, as std_results contains only
    the following static Results.
    If there's a naming conflict, one could either declare only those static
    Results that are used by the cpp file:
    using std_results::SUCCESS;
    using std_results::NIL_DATA;
    etc...
    OR, always fully qualify the name:
    return std_results::SUCCESS;
    TODO: Decide whether the std_results namespace is really worth it; we could
          always just put the static Results in the parent namespace.
 */
namespace std_results {

// NOTE: Not putting these in class Result so that gdb, when printing a Result
// instance, won't print all of these codes, as they clutter the output and
// obscure the useful debug text.

static const int INVALID_RESULT_CODE       = 1;
static const int SUCCESS_CODE              = 0;
// The following result codes are separated by 100 so that it's possible to
// insert new codes at arbitrary positions without compromising the
// ordering of the codes, e.g.:
// POTATO_EXPLODED_CODE = -1200;
// CARROT_EXPLODED_CODE = -1300;
// a new code could be inserted without requiring us to renumber subsequent
// codes to preserve the descending order:
// POTATO_EXPLODED_CODE = -1200;
// CELERY_EXPLODED_CODE = -1250;
// CARROT_EXPLODED_CODE = -1300;
// Further, it's worth noting that we're using decimal 100 instead of
// hex 0x100 (256) for convenience when debugging, as GDB presents integers
// in decimal format by default.
static const int POOL_EMPTY_CODE           = -100;
static const int NOT_CONNECTED_CODE        = -200;
static const int ALREADY_CONNECTED_CODE    = -300;
static const int CONNECTION_FAILED_CODE    = -400;
static const int INDEX_OUT_OF_RANGE_CODE   = -500;
static const int TIMED_OUT_CODE            = -600;
static const int NOT_ENABLED_CODE          = -700;
static const int ALREADY_MAPPED_CODE       = -800;
static const int ENABLED_CODE              = -900;
static const int INDEX_NOT_FOUND_CODE      = -1000;
static const int INSUFFICIENT_DATA_CODE    = -1100;
static const int NIL_DATA_CODE             = -1200;
static const int INSUFFICIENT_SPACE_CODE   = -1300;
static const int ILLEGAL_MAPPING_CODE      = -1400;
static const int NOT_MAPPED_CODE           = -1500;
static const int SINGLE_THREADED_CODE      = -1600;
static const int THREAD_RESTRICTION_CODE   = -1700;
static const int NOT_REGISTERED_CODE       = -1800;
static const int ALREADY_REGISTERED_CODE   = -1900;
static const int NOT_INIT_CODE             = -2000;
static const int ILLEGAL_OPERATION_CODE    = -2100;
static const int BAD_CONFIG_CODE           = -2200;
static const int FILE_MANIP_FAILED_CODE    = -2300;
static const int OPEN_FAILED_CODE          = -2400;
static const int MMAP_FAILED_CODE          = -2500;
static const int NOT_OPEN_CODE             = -2600;
static const int ALREADY_OPEN_CODE         = -2700;
static const int OUT_OF_MEM_CODE           = -2800;
static const int COMM_ERROR_CODE           = -2900;
static const int INVALID_ARGUMENT_CODE     = -3000;
static const int SHUTTING_DOWN_CODE        = -3100;
static const int DEADLOCK_AVERTED_CODE     = -3200;
static const int NOT_IMPLEMENTED_CODE      = -3300;
static const int STATE_ALREADY_EFFECTIVE_CODE = -3400;
static const int RESOURCE_UNAVAILABLE_CODE = -3500;
static const int INTERRUPTED_OPERATION_CODE= -3600;
static const int NOT_READY_CODE            = -3700;
static const int VALUE_OUT_OF_RANGE_CODE   = -3800;
static const int VALUE_INVALID_CODE        = -3900;
static const int WRITE_FAILED_CODE         = -4000;
static const int IO_ERROR_CODE             = -4100;
static const int READ_ONLY_CODE            = -4200;
static const int RESOURCES_ALREADY_RESERVED_CODE = -4300;
static const int NO_POOLS_ALLOCATED_CODE   = -4400;
static const int NOT_FOUND_CODE            = -4500;
static const int PARSE_FAILED_CODE         = -4600;
static const int VALIDATION_FAILED_CODE    = -4700;
static const int NOT_RUNNING_CODE          = -4800;
static const int CONFIG_TRANSITIONING_CODE = -4900;
static const int INVALID_SIZE_CODE         = -5000;
static const int BAD_DATA_CODE             = -5100;
static const int INTERRUPTED_CODE = -5200;
static const int READ_FAILED_CODE = -5300;
static const int WRONG_ARCHIVE_INDEX_CODE = -5400;

static const int DEFAULT_ERROR_CODE = INVALID_RESULT_CODE;

static const evo::Result SUCCESS              (SUCCESS_CODE);
static const evo::Result INVALID_RESULT       (INVALID_RESULT_CODE);
static const evo::Result POOL_EMPTY           (POOL_EMPTY_CODE);
static const evo::Result NOT_CONNECTED        (NOT_CONNECTED_CODE);
static const evo::Result ALREADY_CONNECTED    (ALREADY_CONNECTED_CODE);
static const evo::Result CONNECTION_FAILED    (CONNECTION_FAILED_CODE);
static const evo::Result INDEX_OUT_OF_RANGE   (INDEX_OUT_OF_RANGE_CODE);
static const evo::Result TIMED_OUT            (TIMED_OUT_CODE);
static const evo::Result NOT_ENABLED          (NOT_ENABLED_CODE);
static const evo::Result ALREADY_MAPPED       (ALREADY_MAPPED_CODE);
static const evo::Result ENABLED              (ENABLED_CODE);
static const evo::Result INDEX_NOT_FOUND      (INDEX_NOT_FOUND_CODE);
static const evo::Result INSUFFICIENT_DATA    (INSUFFICIENT_DATA_CODE);
// "NO_DATA" is #defined in the C header netdb.h, so is off limits here, sadly;
// had to therefore settle for "NIL_DATA"
static const evo::Result NIL_DATA             (NIL_DATA_CODE);
static const evo::Result INSUFFICIENT_SPACE   (INSUFFICIENT_SPACE_CODE);
static const evo::Result ILLEGAL_MAPPING      (ILLEGAL_MAPPING_CODE);
static const evo::Result NOT_MAPPED           (NOT_MAPPED_CODE);
static const evo::Result SINGLE_THREADED      (SINGLE_THREADED_CODE);
static const evo::Result THREAD_RESTRICTION   (THREAD_RESTRICTION_CODE);
static const evo::Result NOT_REGISTERED       (NOT_REGISTERED_CODE);
static const evo::Result ALREADY_REGISTERED   (ALREADY_REGISTERED_CODE);
static const evo::Result NOT_INIT             (NOT_INIT_CODE);
static const evo::Result ILLEGAL_OPERATION    (ILLEGAL_OPERATION_CODE);
static const evo::Result BAD_CONFIG           (BAD_CONFIG_CODE);
static const evo::Result FILE_MANIP_FAILED    (FILE_MANIP_FAILED_CODE);
static const evo::Result OPEN_FAILED          (OPEN_FAILED_CODE);
static const evo::Result MMAP_FAILED          (MMAP_FAILED_CODE);
static const evo::Result NOT_OPEN             (NOT_OPEN_CODE);
static const evo::Result ALREADY_OPEN         (ALREADY_OPEN_CODE);
static const evo::Result OUT_OF_MEM           (OUT_OF_MEM_CODE);
static const evo::Result COMM_ERROR           (COMM_ERROR_CODE);
static const evo::Result INVALID_ARGUMENT     (INVALID_ARGUMENT_CODE);
static const evo::Result SHUTTING_DOWN        (SHUTTING_DOWN_CODE);
static const evo::Result DEADLOCK_AVERTED     (DEADLOCK_AVERTED_CODE);
static const evo::Result NOT_IMPLEMENTED      (NOT_IMPLEMENTED_CODE);
static const evo::Result STATE_ALREADY_EFFECTIVE (STATE_ALREADY_EFFECTIVE_CODE);
static const evo::Result RESOURCE_UNAVAILABLE (RESOURCE_UNAVAILABLE_CODE);
static const evo::Result INTERRUPTED_OPERATION(INTERRUPTED_OPERATION_CODE);
static const evo::Result NOT_READY            (NOT_READY_CODE);
static const evo::Result VALUE_OUT_OF_RANGE   (VALUE_OUT_OF_RANGE_CODE);
static const evo::Result VALUE_INVALID        (VALUE_INVALID_CODE);
static const evo::Result WRITE_FAILED         (WRITE_FAILED_CODE);
static const evo::Result IO_ERROR             (IO_ERROR_CODE);
static const evo::Result READ_ONLY            (READ_ONLY_CODE);
static const evo::Result RESOURCES_ALREADY_RESERVED (RESOURCES_ALREADY_RESERVED_CODE);
static const evo::Result NO_POOLS_ALLOCATED   (NO_POOLS_ALLOCATED_CODE);
static const evo::Result NOT_FOUND            (NOT_FOUND_CODE);
static const evo::Result PARSE_FAILED         (PARSE_FAILED_CODE);
static const evo::Result VALIDATION_FAILED    (VALIDATION_FAILED_CODE);
static const evo::Result NOT_RUNNING          (NOT_RUNNING_CODE);
static const evo::Result CONFIG_TRANSITIONING (CONFIG_TRANSITIONING_CODE);
static const evo::Result INVALID_SIZE         (INVALID_SIZE_CODE);
static const evo::Result BAD_DATA             (BAD_DATA_CODE);
static const evo::Result INTERRUPTED (INTERRUPTED_CODE);
static const evo::Result READ_FAILED (READ_FAILED_CODE);
static const evo::Result WRONG_ARCHIVE_INDEX (WRONG_ARCHIVE_INDEX_CODE);

}

#endif
