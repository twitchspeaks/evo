#include <cstring>
#include <strings.h>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <boost/regex.hpp>

#include "common/result.hpp"
#include "common/string.hpp"
#include "common/null_output_iterator.hpp"
#include "common/util.hpp"

using namespace std;
using namespace evo;
using namespace std_results;
using boost::regex;
using boost::smatch;
using boost::sregex_iterator;
using boost::regex_search;

string evo::GetFileNameFromPath (const string& file_path)
{
  auto final_slash_index = file_path.find_last_of('/');
  if (string::npos == final_slash_index) {
    return file_path; // contains no directory component
  } else {
    return file_path.substr(final_slash_index+1);
  }
}

bool evo::StrIsNumeric (const string& str)
{
  char* endptr = nullptr;
  return (0 != strtod(str.c_str(), &endptr) || str.c_str() != endptr);
}

double evo::StrToNum (const string& str)
{
  return strtod(str.c_str(), nullptr);
}

bool evo::StrBeginsWith (const string& str, const string& beginning)
{
  if (beginning.length() > str.length()) {
    return false;
  }

  return str.substr(0, beginning.length()) == beginning;
}

bool evo::StrEndsWith (const string& str, const string& ending)
{
  if (ending.length() > str.length()) {
    return false;
  }

  return str.substr(str.length() - ending.length()) == ending;
}

int evo::StrCaseCmp (const string& str1, const string& str2)
{
  return strcasecmp(str1.c_str(), str2.c_str());
}

string evo::StrToLower (const string& str)
{
  string out (str.length(), '\0');
  std::transform(str.begin(), str.end(), out.begin(), ::tolower);
  return out;
}

string& evo::StrToLower (const string& str, string* out)
{
  out->resize(str.length());
  std::transform(str.begin(), str.end(), out->begin(), ::tolower);
  return *out;
}

string evo::StrToUpper (const string& str)
{
  string out (str.length(), '\0');
  std::transform(str.begin(), str.end(), out.begin(), ::toupper);
  return out;
}

string& evo::StrToUpper (const string& str, string* out)
{
  out->resize(str.length());
  std::transform(str.begin(), str.end(), out->begin(), ::toupper);
  return *out;
}

vector<string> evo::StrExplode (const string& str, char delim)
{
  vector<string> tokens;
  return StrExplode(str, delim, &tokens);
}

vector<string>& evo::StrExplode (const string& str, char delim,
                                  vector<string>* tokens_out)
{
  StrExplode(str, delim, back_inserter<vector<string>>(*tokens_out));
  return *tokens_out;
}

void evo::StrExplode (const string& str, char delim, bool group_delimiters,
                       vector<string>* tokens_out,
                       vector<string>* delimiters_out)
{
  function<bool(char, int)> is_delimiter =
        [delim](char ch, int)->bool { return (delim == ch); };

  if (tokens_out && delimiters_out) {
    StrExplode( str, is_delimiter, group_delimiters,
        back_inserter<vector<string>>(*tokens_out),
        back_inserter<vector<string>>(*delimiters_out) );
  }
  else if (tokens_out) {
    StrExplode( str, is_delimiter, group_delimiters,
        back_inserter<vector<string>>(*tokens_out),
        NullOutputIterator<string>() );
  }
  else if (delimiters_out) {
    StrExplode( str, is_delimiter, group_delimiters,
        NullOutputIterator<string>(),
        back_inserter<vector<string>>(*delimiters_out) );
  }
  else {
    // why would anyone ever call this void function and disregard both outputs?? augh
    StrExplode( str, is_delimiter, group_delimiters,
        NullOutputIterator<string>(), NullOutputIterator<string>() );
  }
}

unordered_set<string>& evo::StrExplode (const string& str, char delim,
                                         unordered_set<string>* tokens_out)
{
  vector<string> tokens;

  StrExplode(str, delim, back_inserter<vector<string>>(tokens));

  for (const string& token : tokens) {
    tokens_out->insert(token);
  }

  return *tokens_out;
}

template <typename NumericType>
vector<NumericType>& evo::StrExplodeNum (const string& str, char delim,
                                          vector<NumericType>* tokens_out)
{
  vector<string> tokens;

  StrExplode(str, delim, back_inserter<vector<string>>(tokens));

  for (const string& token : tokens) {
    tokens_out->push_back((NumericType)strtod(token.c_str(), nullptr));
  }

  return *tokens_out;
}

template <typename NumericType>
vector<NumericType> evo::StrExplodeNum (const string& str, char delim)
{
  vector<NumericType> tokens_out;
  StrExplodeNum(str, delim, &tokens_out);
  return tokens_out;
}

int evo::StrCountChars (const string& str, char ch)
{
  return std::count(str.begin(), str.end(), ch);
}

int evo::StrCountChars (string::const_iterator begin_iter,
                         string::const_iterator end_iter, char ch)
{
  return std::count(begin_iter, end_iter, ch);
}

string evo::BinaryDataToString (const void* buf, size_t size,
                                 ssize_t zero_byte_offset)
{
  stringstream strm;
  ssize_t print_count = 0;

  for (size_t i = 0; i < size; ++i)
  {
    ++print_count;
    if (zero_byte_offset == 1+i && zero_byte_offset < size) {
      print_count = 0;
    }

    strm << std::setw(2) << std::dec
         << (i - zero_byte_offset) << ":"
         << std::setw(2) << std::hex << (int)((const byte*)buf)[i]
         << (0 == print_count % 8 ? " \n" : " ");
  }

  return strm.str();
}

string& evo::TrimLeadingWhitespace (string* str_io, string* whitespace_out)
{
  function<bool(int)> lambda_isspace =
      [](int ch)->bool { return (bool)isspace(ch); };

  auto end_iter = find_if_not(str_io->begin(), str_io->end(), lambda_isspace);
  if (whitespace_out) {
    whitespace_out->assign(str_io->begin(), end_iter);
  }
  str_io->erase(str_io->begin(), end_iter);

  return *str_io;
}

string& evo::TrimTrailingWhitespace (string* str_io, string* whitespace_out)
{
  function<bool(int)> lambda_isspace =
        [](int ch)->bool { return (bool)isspace(ch); };

  auto begin_iter =
      find_if_not(str_io->rbegin(), str_io->rend(), lambda_isspace).base();
  if (whitespace_out) {
    whitespace_out->assign(begin_iter, str_io->end());
  }
  str_io->erase(begin_iter, str_io->end());

  return *str_io;
}

string& evo::TrimWhitespace (string* str_io)
{
  return TrimLeadingWhitespace(&TrimTrailingWhitespace(str_io));
}

Result evo::regex::GetMatch (const string &contents, const string &pattern,
                              int value_capture_index, string *match)
{
  boost::regex r(pattern);
  smatch m;

  if (!regex_search(contents, m, r)) {
    return NOT_FOUND.Prepend(
        "Regex pattern didn't match any portion of the subject");
  }

  if (value_capture_index >= m.size()) {
    return INDEX_OUT_OF_RANGE.Prepend( "Capture index '"
        + to_string(value_capture_index) + "' is too large, # captures = "
        + to_string(m.size()) );
  }

  *match = m[value_capture_index];
  return SUCCESS;
}

Result evo::regex::GetMatch (const string &contents, const string &pattern,
                              string *match)
{
  return GetMatch(contents, pattern, 0, match);
}

template <>
Result evo::ParseValue(const string &str, bool *value)
{
  int intval = 0;
  if (SUCCESS == ParseValue(str, &intval))
  {
    *value = (0 != intval);
  }
  else
  {
    const string& lcstr = StrToLower(str);
    if (lcstr == "true") {
      *value = true;
    } else if (lcstr == "false") {
      *value = false;
    } else {
      return PARSE_FAILED;
    }
  }

  return SUCCESS;
}

