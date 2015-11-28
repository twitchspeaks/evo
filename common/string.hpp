#ifndef HPP_ps_string
#define HPP_ps_string

#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <boost/regex.hpp>

#include "common/result.hpp"

namespace evo {

/** Strings containing certain ASCII subsets, currently used in
 * pacsi for validation.
 */
namespace charsets {

/** Set of all lowercase alphebetic characters.
 */
static const std::string kAlphaLower = "abcdefghijklmnopqrstuvwxyz";

/** Set of all uppercase alphebetic characters.
 */
static const std::string kAlphaUpper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/** Set of all alphebetic characters.
 */
static const std::string kAlpha = kAlphaLower + kAlphaUpper;

/** Set of all decimal digits.
 */
static const std::string kDigits = "0123456789";

/** Set of all characters that could represent a positive integer.
 */
static const std::string kPositiveInteger = "+0123456789";

/** Set of all characters that could represent an integer.
 */
static const std::string kInteger = "+-0123456789";

/** Set of all characters that could represent a real-valued number
 (no exponent, e.g. "9200.0" matches but "9.2e3" does not).
 */
static const std::string kNumericReal_NoExp = kDigits + "+-.";

/** Set of all characters that could represent a real-valued number
 (with exponent component, e.g. "9.2e3").
 */
static const std::string kNumericReal_Exp = kNumericReal_NoExp + "eE";

/** Set of non-numeric punctuation characters ('.' is omitted).
 */
static const std::string kNonNumericPunctuation =
    ",<>/?;:'\"[{]}\\|`~!@#$%^&*()=";

/** Set of all punctuation characters.
 */
static const std::string kPunctuation = kNonNumericPunctuation + ".";

/** Set of all whitespace characters.
 */
static const std::string kWhitespace = " \t\v\n\r\f";

/** Set of all printable characters.
 */
static const std::string kPrintable =
    kAlpha + kDigits + kPunctuation + kWhitespace;

/** Set of all non-numeric printable characters.
 */
static const std::string kNonNumericPrintable =
    kAlpha + kNonNumericPunctuation + kWhitespace;
}

namespace regex {

/** Regex pattern that matches a real-valued number.
 */
static const std::string kDoublePattern = R"([eE\+\.\-\d]+)";

/** @return A regex pattern that matches the assignment of a real-valued
 number to a variable of given name.
 @param field_name The name of the variable/parameter/field to which a
 real-valued number is being assigned.
 */
inline std::string MakeDoubleAssignmentPattern (const std::string &field_name) {
  return R"(\b)" + field_name + R"(\s*=\s*()" + kDoublePattern + R"()\s*$)";
}

}

/** Gets the file name from a file path.
 * @param file_path The full or relative path to a file.
 * @return If there is one or more '/' characters in \p file_path, the portion
 * of the path following the final '/' character is returned. If there is no
 * '/' character present, the entire string is returned.
 */
std::string GetFileNameFromPath (const std::string& file_path);

/** True if the call strtod(str) would succeed, see man strtod.
 */
bool StrIsNumeric (const std::string& str);

/** Converts the string to a double using strtod. This performs no checking,
 * so call StrIsNumeric first.
 * @param str A string that hopefully can be parsed as a double.
 * @return The double parsed from \p str.
 */
double StrToNum (const std::string& str);

bool StrBeginsWith (const std::string& str, const std::string& beginning);

bool StrEndsWith (const std::string& str, const std::string& ending);

inline std::string BoolToString (bool val) {
  return val ? "true" : "false";
}

/** Performs a case-insensitive comparison between two strings.
 * @param str1 The first string.
 * @param str2 The second string.
 * @return < 0 if \p str1 < \p str2, 0 if \p str1 == \p str2, and > 0 if
 * \p str1 > \p str2.
 */
int StrCaseCmp (const std::string& str1, const std::string& str2);

/** Converts a string to all lowercase characters.
 * @param str A string.
 * @return A lowercase representation of \p str.
 */
std::string  StrToLower (const std::string& str);

/** Converts a string to all lowercase characters.
 * @param str A string.
 * @param out A string where the lowercase output will be written.
 * @return A reference to \p out.
 */
std::string& StrToLower (const std::string& str, std::string* out);

/** Converts a string to all uppercase characters.
 * @param str A string.
 * @return The uppercase representation of \p str.
 */
std::string  StrToUpper (const std::string& str);

/** Converts a string to all uppercase characters.
 * @param str A string.
 * @param out A string where the uppercase output will be written.
 * @return A reference to \p out.
 */
std::string& StrToUpper (const std::string& str, std::string* out);

/** Joins an array of things that define operator<< together.
 * @param delim A delimeter that will be placed between each item
 * in the string.
 * @param c_array A c-style array of a type whose members are supported
 * by stringstream.
 * @param n_items The number of items in \p c_array.
 * @return A string that is the result of all items in \p c_array being
 * streamed together, with each item separated by \p delim.
 */
template <typename ArrayThing>
inline std::string StrImplode (const std::string& delim,
                               const ArrayThing* c_array, std::size_t n_items)
{
  if (0 == n_items) {
    return "";
  }
  std::stringstream strm;
  strm << c_array[0];
  for (int i = 1; i < n_items; ++i) {
    strm << delim << c_array[i];
  }
  return strm.str();
}

/** Joins a vector of things that define operator<< together.
 * @param delim A delimeter that will be placed between items in
 * the string.
 * @param array A vector of a type whose members are supported by
 * stringstream.
 * @return A string that is the result of all items in \p array being
 * streamed together, with each item separated by \p delim.
 */
template <typename ArrayThing>
inline std::string StrImplode (const std::string& delim,
                               const std::vector<ArrayThing>& array)
{
  // The elements of any std::vector implementation are guaranteed to be stored
  // contiguously in memory, as per the spec, so passing &array[0] should
  // always be correct (additionally, [] doesn't bounds check, so no worries
  // about whether array.empty(), as the other overload will handle that case).
  return StrImplode(delim, &array[0], array.size());
}

/** Joins a collection of things that define operator<< together.
 * @param delim A delimeter that will be placed between items in
 * the string.
 * @param iter_begin The iterator at which the collection begins.
 * @param iter_end The iterator at which the collection ends.
 * @return A string that is the result of all items in \p array being
 * streamed together, with each item separated by \p delim.
 */
template <class IterType>
inline std::string StrImplode (const std::string& delim,
                               IterType iter_begin,
                               IterType iter_end)
{
  if (iter_begin == iter_end) {
    return "";
  }
  std::stringstream strm;
  // An iterator is required to implement unary operator++, but not binary operator+
  auto iter = iter_begin;
  strm << *iter;
  for (++iter; iter != iter_end; ++iter) {
    strm << delim << *iter;
  }
  return strm.str();
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @return A vector of substrings of \p str.
 */
std::vector<std::string> StrExplode (const std::string& str, char delim);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out A vector where the substrings of \p str will be stored.
 * @return A reference to \p tokens_out.
 */
std::vector<std::string>& StrExplode (const std::string& str, char delim,
                                      std::vector<std::string>* tokens_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out A vector where the substrings of \p str will be stored.
 * @param delimeters_out A vector where the found delimeters will be stored.
 */
void StrExplode (const std::string& str, char delim, bool group_delimiters,
                 std::vector<std::string>* tokens_out,
                 std::vector<std::string>* delimiters_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out A set where the substrings of \p str will be stored.
 * @return A reference to \p tokens_out.
 */
std::unordered_set<std::string>& StrExplode (const std::string& str,
    char delim, std::unordered_set<std::string>* tokens_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str, char delim,
                                       TokenOutputIterator tokens_out_iter)
{
  // std::getline() was originally used to split the input str at delimiters,
  // but it behaved unexpectedly in certain rare use cases;
  // valgrind --track-origins=yes reports invalid reads and access of
  // uninitial data in the buffer section of the string object that getline()
  // works on.  This is likely due to a misunderstanding on my part of what
  // getline() really does.

  if (str.empty()) {
    return tokens_out_iter;
  }

  // Note that 'pos' is either npos or the index of a delimiter, and prev_pos
  // is the index of the first char of the next token
  std::string::size_type pos = str.find_first_of(delim);
  std::string::size_type prev_pos = 0;

  while (std::string::npos != pos)
  {
    *(tokens_out_iter++) = str.substr(prev_pos, pos - prev_pos);
    prev_pos = pos+1;
    pos = str.find_first_of(delim, prev_pos);
  }

  // write the final token
  *(tokens_out_iter++) = str.substr(prev_pos);

  return tokens_out_iter;
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param is_delimeter_predicate A function that returns true if the indicated
 * character is a delimeter.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str,
    std::function<bool(char, int)> is_delimiter_predicate,
    bool group_delimiters,
    TokenOutputIterator tokens_out_iter)
{
  int offset = 0;

  for (int i = 0; i < str.length(); ++i)
  {
    if (is_delimiter_predicate(str[i], i)) {
      if (!group_delimiters || i > offset) {
        *(tokens_out_iter++) = str.substr(offset, i - offset);
      }
      offset = i + 1;
    }
  }

  // note that if offset == str.length()-1, then the final token is a single
  // character in size, and hasn't yet been pushed to the output.
  if(offset < str.length()) {
    *tokens_out_iter = str.substr(offset);
  }

  return tokens_out_iter;
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param is_delimeter_predicate A function that returns true if the indicated
 * character is a delimeter.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @param delimeters_out_iter An output iterator where the delimeters
 * used to split \p str will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator, class DelimiterOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str,
    std::function<bool(char, int)> is_delimiter_predicate,
    bool group_delimiters,
    TokenOutputIterator tokens_out_iter,
    DelimiterOutputIterator delimiters_out_iter)
{
  int token_offset = 0;

  if (group_delimiters)
  {
    int delim_offset = 0;
    for (int i = 0; i < str.length(); ++i)
    {
      if (is_delimiter_predicate(str[i], i)) {
        if (i > token_offset) {
          *(tokens_out_iter++) = str.substr(token_offset, i - token_offset);
        }
        token_offset = i + 1;
      }
      else {
        if (i > delim_offset) {
          *(delimiters_out_iter++) = str.substr(delim_offset, i - delim_offset);
        }
        delim_offset = i + 1;
      }
    }
    // if group_delimiters, final token should not be an empty string
    if (str.length() > token_offset) {
      *tokens_out_iter = str.substr(token_offset);
    }
  }
  else
  {
    for (int i = 0; i < str.length(); ++i)
    {
      if (is_delimiter_predicate(str[i], i)) {
        *(tokens_out_iter++) = str.substr(token_offset, i - token_offset);
        *(delimiters_out_iter++) = std::string(1, str[i]);
        token_offset = i + 1;
      }
    }
    // if !group_delimiters, final token may be an empty string
    *tokens_out_iter = str.substr(token_offset);
  }

  return tokens_out_iter;
}

/** Splits a string on a specified delimeter and attempts to parse the
 * resulting string-parts as doubles. This currently doesn't really work
 * properly since any substrings that failed to parse will be returned
 * as zeroes, which are not discernible from actual zeroes.
 * @param str A string.
 * @param delim A delimeter that \p str will be split on.
 * @param tokens_out A vector where the parsed values will be stored.
 * @return A reference to \p tokens_out.
 */
template <typename NumericType>
std::vector<NumericType>& StrExplodeNum (const std::string& str, char delim,
                                         std::vector<NumericType>* tokens_out);

/** Splits a string on a specified delimeter and attempts to parse the
 * resulting string-parts as doubles. This currently doesn't really work
 * properly since any substrings that failed to parse will be returned
 * as zeroes, which are not discernible from actual zeroes.
 * @param str A string.
 * @param delim A delimeter that \p str will be split on.
 * @return A vector where the parsed values will be stored.
 */
template <typename NumericType>
std::vector<NumericType> StrExplodeNum (const std::string& str, char delim);

/** Counts the number of instances of \p ch in a string.
 * @param begin_iter An iterator pointing to the start of a string.
 * @param end_iter An iterator pointing to the end of a string.
 * @param ch The character being counted.
 * @return The number of instances of \p ch that were counted.
 */
int StrCountChars (std::string::const_iterator begin_iter,
                   std::string::const_iterator end_iter, char ch);

/** Counts the number of instances of \p ch in a string.
 * @param string A string.
 * @param ch The character being counted.
 * @return The number of instances of \p ch that were counted.
 */
int StrCountChars (const std::string& str, char ch);


/** Render given binary data buffer to human-readable text.
 * @param buf The buffer where the binary data is stored.
 * @param size The number of bytes in \p buf.
 * @param zero_byte_offset Index of the byte to be labeled the zeroth ("0:")
 * relative to buf[0]; can be negative.  Particularly useful when aligning
 * the printed byte indices of a text-rendered buffer to match those of a
 * spec, i.e., for convenience when debugging (think Barry's RCB spec).
 * @return The string showing the bytes in \p buf.
 */
std::string BinaryDataToString (const void* buf, size_t size,
                                ssize_t zero_byte_offset = 0);

/*
std::string ArrayToString (...); NOTE: This function is provided via StrImplode
 */

template <typename MapPairIterType>
inline std::string MapToString (const std::string& delimiter,
    MapPairIterType begin_pair_iter,
    MapPairIterType end_pair_iter,
    std::function<std::string (const typename MapPairIterType::value_type::first_type&)> key_to_string_func,
    std::function<std::string (const typename MapPairIterType::value_type::second_type&)> value_to_string_func)
{
  if (begin_pair_iter == end_pair_iter) {
    return "";
  }

  MapPairIterType pair_iter = begin_pair_iter;
  std::stringstream strm;
  strm << pair_iter->first << " => " << pair_iter->second;
  for (++pair_iter; pair_iter != end_pair_iter; ++pair_iter) {
    strm << delimiter << key_to_string_func(pair_iter->first)
         << " => " << value_to_string_func(pair_iter->second);
  }

  return strm.str();
}

/** TODO: documentation
  MapPairIterType MUST be a pair<keyType, valueType>, which is used by
  std::map and std::unordered_map.
  The types of both the first and second components of the pair must support
  operator<<.
 */
template <typename MapPairIterType>
inline std::string MapToString (const std::string& delimiter,
                                MapPairIterType begin_pair_iter,
                                MapPairIterType end_pair_iter)
{
  auto ostream_compatible_first_type_to_string =
      [](const typename MapPairIterType::value_type::first_type& arg) {
        std::stringstream strm;
        strm << arg;
        return strm.str();
      };
  auto ostream_compatible_second_type_to_string =
      [](const typename MapPairIterType::value_type::second_type& arg) {
        std::stringstream strm;
        strm << arg;
        return strm.str();
      };

  return MapToString<MapPairIterType>(delimiter, begin_pair_iter, end_pair_iter,
      ostream_compatible_first_type_to_string,
      ostream_compatible_second_type_to_string
      );
  /*
  return MapToString<MapPairIterType>(delimiter, begin_pair_iter, end_pair_iter,
      static_cast<std::string (*)(const typename MapPairIterType::value_type::first_type&)>(&std::to_string),
      static_cast<std::string (*)(const typename MapPairIterType::value_type::second_type&)>(&std::to_string)
      );
  */
}

template <typename MapPairIterType>
inline std::string MapToString (MapPairIterType begin_pair_iter,
                                MapPairIterType end_pair_iter)
{
  return MapToString<MapPairIterType>(", ", begin_pair_iter, end_pair_iter);
}

/** Trims leading whitespace from a std::string.  Partially copied from
 * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring.
 * @param str_io A string.
 * @param whitespace_out A string that is set to the string that was trimmed
 * off the beginning of \p str_io.
 * @return A reference to \p str_io.
 */
std::string& TrimLeadingWhitespace (std::string* str_io,
                                    std::string* whitespace_out = nullptr);

/** Trims trailing whitespace from a std::string. Partially copied from
 * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring.
 * @param str_io A string.
 * @param whitespace_out A string that is set to the string that was trimmed
 * off the beginning of \p str_io.
 * @return A reference to \p str_io.
 */
std::string& TrimTrailingWhitespace (std::string* str_io,
                                     std::string* whitespace_out = nullptr);

/** Trims both leading and trailing whitespace from a std::string.
 * @param str_io A string.
 * @return A reference to \p str_io.
 */
std::string& TrimWhitespace (std::string* str_io);

/** Attempt to convert given string to type T (typically a numeric type).
 @param str String to be converted.
 @param value Output that receives the converted value.
 @return SUCCESS if the conversion was successful, PARSE_FAILED otherwise.
 */
template <class T>
inline evo::Result ParseValue(const std::string &str, T *value)
{
  std::stringstream stream(str);

  stream >> *value;

  if(stream.fail()) {
    return std_results::PARSE_FAILED;
  }

  return std_results::SUCCESS;
}

/** Specialization of ParseValue for bool that recognizes "true" and "false".
 */
template <>
evo::Result ParseValue(const std::string &str, bool *value);

namespace regex {

/** Attempt to match given regex pattern against given subject.
 @param contents Subject against which to match the given pattern.
 @param pattern Regex pattern to be matched.
 @param value_capture_index The index of the capture within the match to be
     returned.  '0' is the entire match, '1' is the first parenthetical capture, etc.
 @param match_out The first match discovered, unmodified if no match is found.
 @return SUCCESS if all goes well, otherwise:
     INDEX_OUT_OF_RANGE if at least one match was found, but
         value_capture_index >= the total # captures;
     PARSE_FAILED if there were no matches.
 */
evo::Result GetMatch(const std::string &contents, const std::string &pattern,
                      int value_capture_index, std::string *match_out);

/** A simpler version of GetMatch() that returns the entire match, i.e., the
 zeroth capture.
 */
evo::Result GetMatch(const std::string &contents, const std::string &pattern,
                      std::string *match_out);

/** Convert first match of given regex pattern in given subject to given type.
 @param contents The subject within which to match the pattern.
 @param pattern The regex pattern to be matched.
 @param value_capture_index The index of the capture within the match on which
     the conversion is to be performed; '0' is the entire match, '1' is the
     first parenthetical capture, etc.
 @param value Output that receives the converted value.
 @return SUCCESS on success, or the error return of GetMatch() or ParseValue()
     on failure.
 */
template <class T>
inline evo::Result GetValue(const std::string &contents,
                            const std::string &pattern,
                            int value_capture_index,
                            T *value)
{
  std::string str;
  evo::Result res = GetMatch(contents, pattern, value_capture_index, &str);

  if(res.is_error()) {
    return res;
  }

  res = ParseValue(str, value);

  if(res.is_error()) {
    return res;
  }

  return std_results::SUCCESS;
}

/** A simpler version of GetValue() that converts the entire match, i.e.,
 the zeroth capture.
 */
template <class T>
inline evo::Result GetValue(const std::string& contents,
                            const std::string& pattern,
                            T* value)
{
  return GetValue(contents, pattern, 0, value);
}

}

}

#endif
#ifndef HPP_ps_string
#define HPP_ps_string

#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <boost/regex.hpp>

#include "result.hpp"

namespace evo {

/** Strings containing certain ASCII subsets, currently used in
 * pacsi for validation.
 */
namespace charsets {

/** Set of all lowercase alphebetic characters.
 */
static const std::string kAlphaLower = "abcdefghijklmnopqrstuvwxyz";

/** Set of all uppercase alphebetic characters.
 */
static const std::string kAlphaUpper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/** Set of all alphebetic characters.
 */
static const std::string kAlpha = kAlphaLower + kAlphaUpper;

/** Set of all decimal digits.
 */
static const std::string kDigits = "0123456789";

/** Set of all characters that could represent a positive integer.
 */
static const std::string kPositiveInteger = "+0123456789";

/** Set of all characters that could represent an integer.
 */
static const std::string kInteger = "+-0123456789";

/** Set of all characters that could represent a real-valued number
 (no exponent, e.g. "9200.0" matches but "9.2e3" does not).
 */
static const std::string kNumericReal_NoExp = kDigits + "+-.";

/** Set of all characters that could represent a real-valued number
 (with exponent component, e.g. "9.2e3").
 */
static const std::string kNumericReal_Exp = kNumericReal_NoExp + "eE";

/** Set of non-numeric punctuation characters ('.' is omitted).
 */
static const std::string kNonNumericPunctuation =
    ",<>/?;:'\"[{]}\\|`~!@#$%^&*()=";

/** Set of all punctuation characters.
 */
static const std::string kPunctuation = kNonNumericPunctuation + ".";

/** Set of all whitespace characters.
 */
static const std::string kWhitespace = " \t\v\n\r\f";

/** Set of all printable characters.
 */
static const std::string kPrintable =
    kAlpha + kDigits + kPunctuation + kWhitespace;

/** Set of all non-numeric printable characters.
 */
static const std::string kNonNumericPrintable =
    kAlpha + kNonNumericPunctuation + kWhitespace;
}

namespace regex {

/** Regex pattern that matches a real-valued number.
 */
static const std::string kDoublePattern = R"([eE\+\.\-\d]+)";

/** @return A regex pattern that matches the assignment of a real-valued
 number to a variable of given name.
 @param field_name The name of the variable/parameter/field to which a
 real-valued number is being assigned.
 */
inline std::string MakeDoubleAssignmentPattern (const std::string &field_name) {
  return R"(\b)" + field_name + R"(\s*=\s*()" + kDoublePattern + R"()\s*$)";
}

}

/** Gets the file name from a file path.
 * @param file_path The full or relative path to a file.
 * @return If there is one or more '/' characters in \p file_path, the portion
 * of the path following the final '/' character is returned. If there is no
 * '/' character present, the entire string is returned.
 */
std::string GetFileNameFromPath (const std::string& file_path);

/** True if the call strtod(str) would succeed, see man strtod.
 */
bool StrIsNumeric (const std::string& str);

/** Converts the string to a double using strtod. This performs no checking,
 * so call StrIsNumeric first.
 * @param str A string that hopefully can be parsed as a double.
 * @return The double parsed from \p str.
 */
double StrToNum (const std::string& str);

bool StrBeginsWith (const std::string& str, const std::string& beginning);

bool StrEndsWith (const std::string& str, const std::string& ending);

inline std::string BoolToString (bool val) {
  return val ? "true" : "false";
}

/** Performs a case-insensitive comparison between two strings.
 * @param str1 The first string.
 * @param str2 The second string.
 * @return < 0 if \p str1 < \p str2, 0 if \p str1 == \p str2, and > 0 if
 * \p str1 > \p str2.
 */
int StrCaseCmp (const std::string& str1, const std::string& str2);

/** Converts a string to all lowercase characters.
 * @param str A string.
 * @return A lowercase representation of \p str.
 */
std::string  StrToLower (const std::string& str);

/** Converts a string to all lowercase characters.
 * @param str A string.
 * @param out A string where the lowercase output will be written.
 * @return A reference to \p out.
 */
std::string& StrToLower (const std::string& str, std::string* out);

/** Converts a string to all uppercase characters.
 * @param str A string.
 * @return The uppercase representation of \p str.
 */
std::string  StrToUpper (const std::string& str);

/** Converts a string to all uppercase characters.
 * @param str A string.
 * @param out A string where the uppercase output will be written.
 * @return A reference to \p out.
 */
std::string& StrToUpper (const std::string& str, std::string* out);

/** Joins an array of things that define operator<< together.
 * @param delim A delimeter that will be placed between each item
 * in the string.
 * @param c_array A c-style array of a type whose members are supported
 * by stringstream.
 * @param n_items The number of items in \p c_array.
 * @return A string that is the result of all items in \p c_array being
 * streamed together, with each item separated by \p delim.
 */
template <typename ArrayThing>
inline std::string StrImplode (const std::string& delim,
                               const ArrayThing* c_array, std::size_t n_items)
{
  if (0 == n_items) {
    return "";
  }
  std::stringstream strm;
  strm << c_array[0];
  for (int i = 1; i < n_items; ++i) {
    strm << delim << c_array[i];
  }
  return strm.str();
}

/** Joins a vector of things that define operator<< together.
 * @param delim A delimeter that will be placed between items in
 * the string.
 * @param array A vector of a type whose members are supported by
 * stringstream.
 * @return A string that is the result of all items in \p array being
 * streamed together, with each item separated by \p delim.
 */
template <typename ArrayThing>
inline std::string StrImplode (const std::string& delim,
                               const std::vector<ArrayThing>& array)
{
  // The elements of any std::vector implementation are guaranteed to be stored
  // contiguously in memory, as per the spec, so passing &array[0] should
  // always be correct (additionally, [] doesn't bounds check, so no worries
  // about whether array.empty(), as the other overload will handle that case).
  return StrImplode(delim, &array[0], array.size());
}

/** Joins a collection of things that define operator<< together.
 * @param delim A delimeter that will be placed between items in
 * the string.
 * @param iter_begin The iterator at which the collection begins.
 * @param iter_end The iterator at which the collection ends.
 * @return A string that is the result of all items in \p array being
 * streamed together, with each item separated by \p delim.
 */
template <class IterType>
inline std::string StrImplode (const std::string& delim,
                               IterType iter_begin,
                               IterType iter_end)
{
  if (iter_begin == iter_end) {
    return "";
  }
  std::stringstream strm;
  // An iterator is required to implement unary operator++, but not binary operator+
  auto iter = iter_begin;
  strm << *iter;
  for (++iter; iter != iter_end; ++iter) {
    strm << delim << *iter;
  }
  return strm.str();
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @return A vector of substrings of \p str.
 */
std::vector<std::string> StrExplode (const std::string& str, char delim);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out A vector where the substrings of \p str will be stored.
 * @return A reference to \p tokens_out.
 */
std::vector<std::string>& StrExplode (const std::string& str, char delim,
                                      std::vector<std::string>* tokens_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out A vector where the substrings of \p str will be stored.
 * @param delimeters_out A vector where the found delimeters will be stored.
 */
void StrExplode (const std::string& str, char delim, bool group_delimiters,
                 std::vector<std::string>* tokens_out,
                 std::vector<std::string>* delimiters_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out A set where the substrings of \p str will be stored.
 * @return A reference to \p tokens_out.
 */
std::unordered_set<std::string>& StrExplode (const std::string& str,
    char delim, std::unordered_set<std::string>* tokens_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str, char delim,
                                       TokenOutputIterator tokens_out_iter)
{
  // std::getline() was originally used to split the input str at delimiters,
  // but it behaved unexpectedly in certain rare use cases;
  // valgrind --track-origins=yes reports invalid reads and access of
  // uninitial data in the buffer section of the string object that getline()
  // works on.  This is likely due to a misunderstanding on my part of what
  // getline() really does.

  if (str.empty()) {
    return tokens_out_iter;
  }

  // Note that 'pos' is either npos or the index of a delimiter, and prev_pos
  // is the index of the first char of the next token
  std::string::size_type pos = str.find_first_of(delim);
  std::string::size_type prev_pos = 0;

  while (std::string::npos != pos)
  {
    *(tokens_out_iter++) = str.substr(prev_pos, pos - prev_pos);
    prev_pos = pos+1;
    pos = str.find_first_of(delim, prev_pos);
  }

  // write the final token
  *(tokens_out_iter++) = str.substr(prev_pos);

  return tokens_out_iter;
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param is_delimeter_predicate A function that returns true if the indicated
 * character is a delimeter.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str,
    std::function<bool(char, int)> is_delimiter_predicate,
    bool group_delimiters,
    TokenOutputIterator tokens_out_iter)
{
  int offset = 0;

  for (int i = 0; i < str.length(); ++i)
  {
    if (is_delimiter_predicate(str[i], i)) {
      if (!group_delimiters || i > offset) {
        *(tokens_out_iter++) = str.substr(offset, i - offset);
      }
      offset = i + 1;
    }
  }

  // note that if offset == str.length()-1, then the final token is a single
  // character in size, and hasn't yet been pushed to the output.
  if(offset < str.length()) {
    *tokens_out_iter = str.substr(offset);
  }

  return tokens_out_iter;
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param is_delimeter_predicate A function that returns true if the indicated
 * character is a delimeter.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @param delimeters_out_iter An output iterator where the delimeters
 * used to split \p str will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator, class DelimiterOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str,
    std::function<bool(char, int)> is_delimiter_predicate,
    bool group_delimiters,
    TokenOutputIterator tokens_out_iter,
    DelimiterOutputIterator delimiters_out_iter)
{
  int token_offset = 0;

  if (group_delimiters)
  {
    int delim_offset = 0;
    for (int i = 0; i < str.length(); ++i)
    {
      if (is_delimiter_predicate(str[i], i)) {
        if (i > token_offset) {
          *(tokens_out_iter++) = str.substr(token_offset, i - token_offset);
        }
        token_offset = i + 1;
      }
      else {
        if (i > delim_offset) {
          *(delimiters_out_iter++) = str.substr(delim_offset, i - delim_offset);
        }
        delim_offset = i + 1;
      }
    }
    // if group_delimiters, final token should not be an empty string
    if (str.length() > token_offset) {
      *tokens_out_iter = str.substr(token_offset);
    }
  }
  else
  {
    for (int i = 0; i < str.length(); ++i)
    {
      if (is_delimiter_predicate(str[i], i)) {
        *(tokens_out_iter++) = str.substr(token_offset, i - token_offset);
        *(delimiters_out_iter++) = std::string(1, str[i]);
        token_offset = i + 1;
      }
    }
    // if !group_delimiters, final token may be an empty string
    *tokens_out_iter = str.substr(token_offset);
  }

  return tokens_out_iter;
}

/** Splits a string on a specified delimeter and attempts to parse the
 * resulting string-parts as doubles. This currently doesn't really work
 * properly since any substrings that failed to parse will be returned
 * as zeroes, which are not discernible from actual zeroes.
 * @param str A string.
 * @param delim A delimeter that \p str will be split on.
 * @param tokens_out A vector where the parsed values will be stored.
 * @return A reference to \p tokens_out.
 */
template <typename NumericType>
std::vector<NumericType>& StrExplodeNum (const std::string& str, char delim,
                                         std::vector<NumericType>* tokens_out);

/** Splits a string on a specified delimeter and attempts to parse the
 * resulting string-parts as doubles. This currently doesn't really work
 * properly since any substrings that failed to parse will be returned
 * as zeroes, which are not discernible from actual zeroes.
 * @param str A string.
 * @param delim A delimeter that \p str will be split on.
 * @return A vector where the parsed values will be stored.
 */
template <typename NumericType>
std::vector<NumericType> StrExplodeNum (const std::string& str, char delim);

/** Counts the number of instances of \p ch in a string.
 * @param begin_iter An iterator pointing to the start of a string.
 * @param end_iter An iterator pointing to the end of a string.
 * @param ch The character being counted.
 * @return The number of instances of \p ch that were counted.
 */
int StrCountChars (std::string::const_iterator begin_iter,
                   std::string::const_iterator end_iter, char ch);

/** Counts the number of instances of \p ch in a string.
 * @param string A string.
 * @param ch The character being counted.
 * @return The number of instances of \p ch that were counted.
 */
int StrCountChars (const std::string& str, char ch);


/** Render given binary data buffer to human-readable text.
 * @param buf The buffer where the binary data is stored.
 * @param size The number of bytes in \p buf.
 * @param zero_byte_offset Index of the byte to be labeled the zeroth ("0:")
 * relative to buf[0]; can be negative.  Particularly useful when aligning
 * the printed byte indices of a text-rendered buffer to match those of a
 * spec, i.e., for convenience when debugging (think Barry's RCB spec).
 * @return The string showing the bytes in \p buf.
 */
std::string BinaryDataToString (const void* buf, size_t size,
                                ssize_t zero_byte_offset = 0);

/*
std::string ArrayToString (...); NOTE: This function is provided via StrImplode
 */

template <typename MapPairIterType>
inline std::string MapToString (const std::string& delimiter,
    MapPairIterType begin_pair_iter,
    MapPairIterType end_pair_iter,
    std::function<std::string (const typename MapPairIterType::value_type::first_type&)> key_to_string_func,
    std::function<std::string (const typename MapPairIterType::value_type::second_type&)> value_to_string_func)
{
  if (begin_pair_iter == end_pair_iter) {
    return "";
  }

  MapPairIterType pair_iter = begin_pair_iter;
  std::stringstream strm;
  strm << pair_iter->first << " => " << pair_iter->second;
  for (++pair_iter; pair_iter != end_pair_iter; ++pair_iter) {
    strm << delimiter << key_to_string_func(pair_iter->first)
         << " => " << value_to_string_func(pair_iter->second);
  }

  return strm.str();
}

/** TODO: documentation
  MapPairIterType MUST be a pair<keyType, valueType>, which is used by
  std::map and std::unordered_map.
  The types of both the first and second components of the pair must support
  operator<<.
 */
template <typename MapPairIterType>
inline std::string MapToString (const std::string& delimiter,
                                MapPairIterType begin_pair_iter,
                                MapPairIterType end_pair_iter)
{
  auto ostream_compatible_first_type_to_string =
      [](const typename MapPairIterType::value_type::first_type& arg) {
        std::stringstream strm;
        strm << arg;
        return strm.str();
      };
  auto ostream_compatible_second_type_to_string =
      [](const typename MapPairIterType::value_type::second_type& arg) {
        std::stringstream strm;
        strm << arg;
        return strm.str();
      };

  return MapToString<MapPairIterType>(delimiter, begin_pair_iter, end_pair_iter,
      ostream_compatible_first_type_to_string,
      ostream_compatible_second_type_to_string
      );
  /*
  return MapToString<MapPairIterType>(delimiter, begin_pair_iter, end_pair_iter,
      static_cast<std::string (*)(const typename MapPairIterType::value_type::first_type&)>(&std::to_string),
      static_cast<std::string (*)(const typename MapPairIterType::value_type::second_type&)>(&std::to_string)
      );
  */
}

template <typename MapPairIterType>
inline std::string MapToString (MapPairIterType begin_pair_iter,
                                MapPairIterType end_pair_iter)
{
  return MapToString<MapPairIterType>(", ", begin_pair_iter, end_pair_iter);
}

/** Trims leading whitespace from a std::string.  Partially copied from
 * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring.
 * @param str_io A string.
 * @param whitespace_out A string that is set to the string that was trimmed
 * off the beginning of \p str_io.
 * @return A reference to \p str_io.
 */
std::string& TrimLeadingWhitespace (std::string* str_io,
                                    std::string* whitespace_out = nullptr);

/** Trims trailing whitespace from a std::string. Partially copied from
 * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring.
 * @param str_io A string.
 * @param whitespace_out A string that is set to the string that was trimmed
 * off the beginning of \p str_io.
 * @return A reference to \p str_io.
 */
std::string& TrimTrailingWhitespace (std::string* str_io,
                                     std::string* whitespace_out = nullptr);

/** Trims both leading and trailing whitespace from a std::string.
 * @param str_io A string.
 * @return A reference to \p str_io.
 */
std::string& TrimWhitespace (std::string* str_io);

/** Attempt to convert given string to type T (typically a numeric type).
 @param str String to be converted.
 @param value Output that receives the converted value.
 @return SUCCESS if the conversion was successful, PARSE_FAILED otherwise.
 */
template <class T>
inline evo::Result ParseValue(const std::string &str, T *value)
{
  std::stringstream stream(str);

  stream >> *value;

  if(stream.fail()) {
    return std_results::PARSE_FAILED;
  }

  return std_results::SUCCESS;
}

/** Specialization of ParseValue for bool that recognizes "true" and "false".
 */
template <>
evo::Result ParseValue(const std::string &str, bool *value);

namespace regex {

/** Attempt to match given regex pattern against given subject.
 @param contents Subject against which to match the given pattern.
 @param pattern Regex pattern to be matched.
 @param value_capture_index The index of the capture within the match to be
     returned.  '0' is the entire match, '1' is the first parenthetical capture, etc.
 @param match_out The first match discovered, unmodified if no match is found.
 @return SUCCESS if all goes well, otherwise:
     INDEX_OUT_OF_RANGE if at least one match was found, but
         value_capture_index >= the total # captures;
     PARSE_FAILED if there were no matches.
 */
evo::Result GetMatch(const std::string &contents, const std::string &pattern,
                      int value_capture_index, std::string *match_out);

/** A simpler version of GetMatch() that returns the entire match, i.e., the
 zeroth capture.
 */
evo::Result GetMatch(const std::string &contents, const std::string &pattern,
                      std::string *match_out);

/** Convert first match of given regex pattern in given subject to given type.
 @param contents The subject within which to match the pattern.
 @param pattern The regex pattern to be matched.
 @param value_capture_index The index of the capture within the match on which
     the conversion is to be performed; '0' is the entire match, '1' is the
     first parenthetical capture, etc.
 @param value Output that receives the converted value.
 @return SUCCESS on success, or the error return of GetMatch() or ParseValue()
     on failure.
 */
template <class T>
inline evo::Result GetValue(const std::string &contents,
                             const std::string &pattern,
                             int value_capture_index,
                             T *value)
{
  std::string str;
  evo::Result res = GetMatch(contents, pattern, value_capture_index, &str);

  if(res.is_error()) {
    return res;
  }

  res = ParseValue(str, value);

  if(res.is_error()) {
    return res;
  }

  return std_results::SUCCESS;
}

/** A simpler version of GetValue() that converts the entire match, i.e.,
 the zeroth capture.
 */
template <class T>
inline evo::Result GetValue(const std::string &contents,
                             const std::string &pattern,
                             T *value)
{
  return GetValue(contents, pattern, 0, value);
}

}

}

#endif
#ifndef HPP_ps_string
#define HPP_ps_string

#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <boost/regex.hpp>

#include "result.hpp"

namespace evo {

/** Strings containing certain ASCII subsets, currently used in
 * pacsi for validation.
 */
namespace charsets {

/** Set of all lowercase alphebetic characters.
 */
static const std::string kAlphaLower = "abcdefghijklmnopqrstuvwxyz";

/** Set of all uppercase alphebetic characters.
 */
static const std::string kAlphaUpper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/** Set of all alphebetic characters.
 */
static const std::string kAlpha = kAlphaLower + kAlphaUpper;

/** Set of all decimal digits.
 */
static const std::string kDigits = "0123456789";

/** Set of all characters that could represent a positive integer.
 */
static const std::string kPositiveInteger = "+0123456789";

/** Set of all characters that could represent an integer.
 */
static const std::string kInteger = "+-0123456789";

/** Set of all characters that could represent a real-valued number
 (no exponent, e.g. "9200.0" matches but "9.2e3" does not).
 */
static const std::string kNumericReal_NoExp = kDigits + "+-.";

/** Set of all characters that could represent a real-valued number
 (with exponent component, e.g. "9.2e3").
 */
static const std::string kNumericReal_Exp = kNumericReal_NoExp + "eE";

/** Set of non-numeric punctuation characters ('.' is omitted).
 */
static const std::string kNonNumericPunctuation =
    ",<>/?;:'\"[{]}\\|`~!@#$%^&*()=";

/** Set of all punctuation characters.
 */
static const std::string kPunctuation = kNonNumericPunctuation + ".";

/** Set of all whitespace characters.
 */
static const std::string kWhitespace = " \t\v\n\r\f";

/** Set of all printable characters.
 */
static const std::string kPrintable =
    kAlpha + kDigits + kPunctuation + kWhitespace;

/** Set of all non-numeric printable characters.
 */
static const std::string kNonNumericPrintable =
    kAlpha + kNonNumericPunctuation + kWhitespace;
}

namespace regex {

/** Regex pattern that matches a real-valued number.
 */
static const std::string kDoublePattern = R"([eE\+\.\-\d]+)";

/** @return A regex pattern that matches the assignment of a real-valued
 number to a variable of given name.
 @param field_name The name of the variable/parameter/field to which a
 real-valued number is being assigned.
 */
inline std::string MakeDoubleAssignmentPattern (const std::string &field_name) {
  return R"(\b)" + field_name + R"(\s*=\s*()" + kDoublePattern + R"()\s*$)";
}

}

/** Gets the file name from a file path.
 * @param file_path The full or relative path to a file.
 * @return If there is one or more '/' characters in \p file_path, the portion
 * of the path following the final '/' character is returned. If there is no
 * '/' character present, the entire string is returned.
 */
std::string GetFileNameFromPath (const std::string& file_path);

/** True if the call strtod(str) would succeed, see man strtod.
 */
bool StrIsNumeric (const std::string& str);

/** Converts the string to a double using strtod. This performs no checking,
 * so call StrIsNumeric first.
 * @param str A string that hopefully can be parsed as a double.
 * @return The double parsed from \p str.
 */
double StrToNum (const std::string& str);

bool StrBeginsWith (const std::string& str, const std::string& beginning);

bool StrEndsWith (const std::string& str, const std::string& ending);

inline std::string BoolToString (bool val) {
  return val ? "true" : "false";
}

/** Performs a case-insensitive comparison between two strings.
 * @param str1 The first string.
 * @param str2 The second string.
 * @return < 0 if \p str1 < \p str2, 0 if \p str1 == \p str2, and > 0 if
 * \p str1 > \p str2.
 */
int StrCaseCmp (const std::string& str1, const std::string& str2);

/** Converts a string to all lowercase characters.
 * @param str A string.
 * @return A lowercase representation of \p str.
 */
std::string  StrToLower (const std::string& str);

/** Converts a string to all lowercase characters.
 * @param str A string.
 * @param out A string where the lowercase output will be written.
 * @return A reference to \p out.
 */
std::string& StrToLower (const std::string& str, std::string* out);

/** Converts a string to all uppercase characters.
 * @param str A string.
 * @return The uppercase representation of \p str.
 */
std::string  StrToUpper (const std::string& str);

/** Converts a string to all uppercase characters.
 * @param str A string.
 * @param out A string where the uppercase output will be written.
 * @return A reference to \p out.
 */
std::string& StrToUpper (const std::string& str, std::string* out);

/** Joins an array of things that define operator<< together.
 * @param delim A delimeter that will be placed between each item
 * in the string.
 * @param c_array A c-style array of a type whose members are supported
 * by stringstream.
 * @param n_items The number of items in \p c_array.
 * @return A string that is the result of all items in \p c_array being
 * streamed together, with each item separated by \p delim.
 */
template <typename ArrayThing>
inline std::string StrImplode (const std::string& delim,
                               const ArrayThing* c_array, std::size_t n_items)
{
  if (0 == n_items) {
    return "";
  }
  std::stringstream strm;
  strm << c_array[0];
  for (int i = 1; i < n_items; ++i) {
    strm << delim << c_array[i];
  }
  return strm.str();
}

/** Joins a vector of things that define operator<< together.
 * @param delim A delimeter that will be placed between items in
 * the string.
 * @param array A vector of a type whose members are supported by
 * stringstream.
 * @return A string that is the result of all items in \p array being
 * streamed together, with each item separated by \p delim.
 */
template <typename ArrayThing>
inline std::string StrImplode (const std::string& delim,
                               const std::vector<ArrayThing>& array)
{
  // The elements of any std::vector implementation are guaranteed to be stored
  // contiguously in memory, as per the spec, so passing &array[0] should
  // always be correct (additionally, [] doesn't bounds check, so no worries
  // about whether array.empty(), as the other overload will handle that case).
  return StrImplode(delim, &array[0], array.size());
}

/** Joins a collection of things that define operator<< together.
 * @param delim A delimeter that will be placed between items in
 * the string.
 * @param iter_begin The iterator at which the collection begins.
 * @param iter_end The iterator at which the collection ends.
 * @return A string that is the result of all items in \p array being
 * streamed together, with each item separated by \p delim.
 */
template <class IterType>
inline std::string StrImplode (const std::string& delim,
                               IterType iter_begin,
                               IterType iter_end)
{
  if (iter_begin == iter_end) {
    return "";
  }
  std::stringstream strm;
  // An iterator is required to implement unary operator++, but not binary operator+
  auto iter = iter_begin;
  strm << *iter;
  for (++iter; iter != iter_end; ++iter) {
    strm << delim << *iter;
  }
  return strm.str();
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @return A vector of substrings of \p str.
 */
std::vector<std::string> StrExplode (const std::string& str, char delim);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out A vector where the substrings of \p str will be stored.
 * @return A reference to \p tokens_out.
 */
std::vector<std::string>& StrExplode (const std::string& str, char delim,
                                      std::vector<std::string>* tokens_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out A vector where the substrings of \p str will be stored.
 * @param delimeters_out A vector where the found delimeters will be stored.
 */
void StrExplode (const std::string& str, char delim, bool group_delimiters,
                 std::vector<std::string>* tokens_out,
                 std::vector<std::string>* delimiters_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out A set where the substrings of \p str will be stored.
 * @return A reference to \p tokens_out.
 */
std::unordered_set<std::string>& StrExplode (const std::string& str,
    char delim, std::unordered_set<std::string>* tokens_out);

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param delim A character that will be used as the delimeter for the string
 * splitting.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str, char delim,
                                       TokenOutputIterator tokens_out_iter)
{
  // std::getline() was originally used to split the input str at delimiters,
  // but it behaved unexpectedly in certain rare use cases;
  // valgrind --track-origins=yes reports invalid reads and access of
  // uninitial data in the buffer section of the string object that getline()
  // works on.  This is likely due to a misunderstanding on my part of what
  // getline() really does.

  if (str.empty()) {
    return tokens_out_iter;
  }

  // Note that 'pos' is either npos or the index of a delimiter, and prev_pos
  // is the index of the first char of the next token
  std::string::size_type pos = str.find_first_of(delim);
  std::string::size_type prev_pos = 0;

  while (std::string::npos != pos)
  {
    *(tokens_out_iter++) = str.substr(prev_pos, pos - prev_pos);
    prev_pos = pos+1;
    pos = str.find_first_of(delim, prev_pos);
  }

  // write the final token
  *(tokens_out_iter++) = str.substr(prev_pos);

  return tokens_out_iter;
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param is_delimeter_predicate A function that returns true if the indicated
 * character is a delimeter.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str,
    std::function<bool(char, int)> is_delimiter_predicate,
    bool group_delimiters,
    TokenOutputIterator tokens_out_iter)
{
  int offset = 0;

  for (int i = 0; i < str.length(); ++i)
  {
    if (is_delimiter_predicate(str[i], i)) {
      if (!group_delimiters || i > offset) {
        *(tokens_out_iter++) = str.substr(offset, i - offset);
      }
      offset = i + 1;
    }
  }

  // note that if offset == str.length()-1, then the final token is a single
  // character in size, and hasn't yet been pushed to the output.
  if(offset < str.length()) {
    *tokens_out_iter = str.substr(offset);
  }

  return tokens_out_iter;
}

/** Splits a string into a vector of string-parts.
 * @param str A string.
 * @param is_delimeter_predicate A function that returns true if the indicated
 * character is a delimeter.
 * @param group_delimeters Whether or not a sequence of \p delim will be
 * considered a single delimeter. If false, empty strings will be added to
 * \p tokens_out representing the string between two instances of \p delim.
 * @param tokens_out_iter An Output iterator where the substrings of \p str
 * will be stored.
 * @param delimeters_out_iter An output iterator where the delimeters
 * used to split \p str will be stored.
 * @return A reference to \p tokens_out_iter.
 */
template <class TokenOutputIterator, class DelimiterOutputIterator>
inline TokenOutputIterator StrExplode (const std::string& str,
    std::function<bool(char, int)> is_delimiter_predicate,
    bool group_delimiters,
    TokenOutputIterator tokens_out_iter,
    DelimiterOutputIterator delimiters_out_iter)
{
  int token_offset = 0;

  if (group_delimiters)
  {
    int delim_offset = 0;
    for (int i = 0; i < str.length(); ++i)
    {
      if (is_delimiter_predicate(str[i], i)) {
        if (i > token_offset) {
          *(tokens_out_iter++) = str.substr(token_offset, i - token_offset);
        }
        token_offset = i + 1;
      }
      else {
        if (i > delim_offset) {
          *(delimiters_out_iter++) = str.substr(delim_offset, i - delim_offset);
        }
        delim_offset = i + 1;
      }
    }
    // if group_delimiters, final token should not be an empty string
    if (str.length() > token_offset) {
      *tokens_out_iter = str.substr(token_offset);
    }
  }
  else
  {
    for (int i = 0; i < str.length(); ++i)
    {
      if (is_delimiter_predicate(str[i], i)) {
        *(tokens_out_iter++) = str.substr(token_offset, i - token_offset);
        *(delimiters_out_iter++) = std::string(1, str[i]);
        token_offset = i + 1;
      }
    }
    // if !group_delimiters, final token may be an empty string
    *tokens_out_iter = str.substr(token_offset);
  }

  return tokens_out_iter;
}

/** Splits a string on a specified delimeter and attempts to parse the
 * resulting string-parts as doubles. This currently doesn't really work
 * properly since any substrings that failed to parse will be returned
 * as zeroes, which are not discernible from actual zeroes.
 * @param str A string.
 * @param delim A delimeter that \p str will be split on.
 * @param tokens_out A vector where the parsed values will be stored.
 * @return A reference to \p tokens_out.
 */
template <typename NumericType>
std::vector<NumericType>& StrExplodeNum (const std::string& str, char delim,
                                         std::vector<NumericType>* tokens_out);

/** Splits a string on a specified delimeter and attempts to parse the
 * resulting string-parts as doubles. This currently doesn't really work
 * properly since any substrings that failed to parse will be returned
 * as zeroes, which are not discernible from actual zeroes.
 * @param str A string.
 * @param delim A delimeter that \p str will be split on.
 * @return A vector where the parsed values will be stored.
 */
template <typename NumericType>
std::vector<NumericType> StrExplodeNum (const std::string& str, char delim);

/** Counts the number of instances of \p ch in a string.
 * @param begin_iter An iterator pointing to the start of a string.
 * @param end_iter An iterator pointing to the end of a string.
 * @param ch The character being counted.
 * @return The number of instances of \p ch that were counted.
 */
int StrCountChars (std::string::const_iterator begin_iter,
                   std::string::const_iterator end_iter, char ch);

/** Counts the number of instances of \p ch in a string.
 * @param string A string.
 * @param ch The character being counted.
 * @return The number of instances of \p ch that were counted.
 */
int StrCountChars (const std::string& str, char ch);


/** Render given binary data buffer to human-readable text.
 * @param buf The buffer where the binary data is stored.
 * @param size The number of bytes in \p buf.
 * @param zero_byte_offset Index of the byte to be labeled the zeroth ("0:")
 * relative to buf[0]; can be negative.  Particularly useful when aligning
 * the printed byte indices of a text-rendered buffer to match those of a
 * spec, i.e., for convenience when debugging (think Barry's RCB spec).
 * @return The string showing the bytes in \p buf.
 */
std::string BinaryDataToString (const void* buf, size_t size,
                                ssize_t zero_byte_offset = 0);

/*
std::string ArrayToString (...); NOTE: This function is provided via StrImplode
 */

template <typename MapPairIterType>
inline std::string MapToString (const std::string& delimiter,
    MapPairIterType begin_pair_iter,
    MapPairIterType end_pair_iter,
    std::function<std::string (const typename MapPairIterType::value_type::first_type&)> key_to_string_func,
    std::function<std::string (const typename MapPairIterType::value_type::second_type&)> value_to_string_func)
{
  if (begin_pair_iter == end_pair_iter) {
    return "";
  }

  MapPairIterType pair_iter = begin_pair_iter;
  std::stringstream strm;
  strm << pair_iter->first << " => " << pair_iter->second;
  for (++pair_iter; pair_iter != end_pair_iter; ++pair_iter) {
    strm << delimiter << key_to_string_func(pair_iter->first)
         << " => " << value_to_string_func(pair_iter->second);
  }

  return strm.str();
}

/** TODO: documentation
  MapPairIterType MUST be a pair<keyType, valueType>, which is used by
  std::map and std::unordered_map.
  The types of both the first and second components of the pair must support
  operator<<.
 */
template <typename MapPairIterType>
inline std::string MapToString (const std::string& delimiter,
                                MapPairIterType begin_pair_iter,
                                MapPairIterType end_pair_iter)
{
  auto ostream_compatible_first_type_to_string =
      [](const typename MapPairIterType::value_type::first_type& arg) {
        std::stringstream strm;
        strm << arg;
        return strm.str();
      };
  auto ostream_compatible_second_type_to_string =
      [](const typename MapPairIterType::value_type::second_type& arg) {
        std::stringstream strm;
        strm << arg;
        return strm.str();
      };

  return MapToString<MapPairIterType>(delimiter, begin_pair_iter, end_pair_iter,
      ostream_compatible_first_type_to_string,
      ostream_compatible_second_type_to_string
      );
  /*
  return MapToString<MapPairIterType>(delimiter, begin_pair_iter, end_pair_iter,
      static_cast<std::string (*)(const typename MapPairIterType::value_type::first_type&)>(&std::to_string),
      static_cast<std::string (*)(const typename MapPairIterType::value_type::second_type&)>(&std::to_string)
      );
  */
}

template <typename MapPairIterType>
inline std::string MapToString (MapPairIterType begin_pair_iter,
                                MapPairIterType end_pair_iter)
{
  return MapToString<MapPairIterType>(", ", begin_pair_iter, end_pair_iter);
}

/** Trims leading whitespace from a std::string.  Partially copied from
 * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring.
 * @param str_io A string.
 * @param whitespace_out A string that is set to the string that was trimmed
 * off the beginning of \p str_io.
 * @return A reference to \p str_io.
 */
std::string& TrimLeadingWhitespace (std::string* str_io,
                                    std::string* whitespace_out = nullptr);

/** Trims trailing whitespace from a std::string. Partially copied from
 * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring.
 * @param str_io A string.
 * @param whitespace_out A string that is set to the string that was trimmed
 * off the beginning of \p str_io.
 * @return A reference to \p str_io.
 */
std::string& TrimTrailingWhitespace (std::string* str_io,
                                     std::string* whitespace_out = nullptr);

/** Trims both leading and trailing whitespace from a std::string.
 * @param str_io A string.
 * @return A reference to \p str_io.
 */
std::string& TrimWhitespace (std::string* str_io);

/** Attempt to convert given string to type T (typically a numeric type).
 @param str String to be converted.
 @param value Output that receives the converted value.
 @return SUCCESS if the conversion was successful, PARSE_FAILED otherwise.
 */
template <class T>
inline evo::Result ParseValue(const std::string &str, T *value)
{
  std::stringstream stream(str);

  stream >> *value;

  if(stream.fail()) {
    return std_results::PARSE_FAILED;
  }

  return std_results::SUCCESS;
}

/** Specialization of ParseValue for bool that recognizes "true" and "false".
 */
template <>
evo::Result ParseValue(const std::string &str, bool *value);

namespace regex {

/** Attempt to match given regex pattern against given subject.
 @param contents Subject against which to match the given pattern.
 @param pattern Regex pattern to be matched.
 @param value_capture_index The index of the capture within the match to be
     returned.  '0' is the entire match, '1' is the first parenthetical capture, etc.
 @param match_out The first match discovered, unmodified if no match is found.
 @return SUCCESS if all goes well, otherwise:
     INDEX_OUT_OF_RANGE if at least one match was found, but
         value_capture_index >= the total # captures;
     PARSE_FAILED if there were no matches.
 */
evo::Result GetMatch(const std::string &contents, const std::string &pattern,
                      int value_capture_index, std::string *match_out);

/** A simpler version of GetMatch() that returns the entire match, i.e., the
 zeroth capture.
 */
evo::Result GetMatch(const std::string &contents, const std::string &pattern,
                      std::string *match_out);

/** Convert first match of given regex pattern in given subject to given type.
 @param contents The subject within which to match the pattern.
 @param pattern The regex pattern to be matched.
 @param value_capture_index The index of the capture within the match on which
     the conversion is to be performed; '0' is the entire match, '1' is the
     first parenthetical capture, etc.
 @param value Output that receives the converted value.
 @return SUCCESS on success, or the error return of GetMatch() or ParseValue()
     on failure.
 */
template <class T>
inline evo::Result GetValue(const std::string &contents,
                             const std::string &pattern,
                             int value_capture_index,
                             T *value)
{
  std::string str;
  evo::Result res = GetMatch(contents, pattern, value_capture_index, &str);

  if(res.is_error()) {
    return res;
  }

  res = ParseValue(str, value);

  if(res.is_error()) {
    return res;
  }

  return std_results::SUCCESS;
}

/** A simpler version of GetValue() that converts the entire match, i.e.,
 the zeroth capture.
 */
template <class T>
inline evo::Result GetValue(const std::string &contents,
                             const std::string &pattern,
                             T *value)
{
  return GetValue(contents, pattern, 0, value);
}

}

}

#endif

