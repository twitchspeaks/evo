#ifndef COMMON_I_STRINGABLE_HPP
#define COMMON_I_STRINGABLE_HPP

#include <iostream>
#include <string>

namespace evo {

/**
 * An interface for classes that have human-readable string representations.
 */
class IStringable
{
public:

  virtual ~IStringable ()
  {}

  /**
   * Gets the string representation of the object.
   * @return The string representation of the object.
   */
  virtual std::string ToString () const = 0;
};

/**
 * Implementation of ostream operator<< for IStringable objects.
 * @param ostrm The ostream the string will be streamed onto.
 * @param stringable {The object whose string representation will be
 * streamed onto \p ostrm.}
 * @return A reference to \p ostrm.
 */
template <typename Traits>
inline std::basic_ostream<char, Traits>& operator << (
    std::basic_ostream<char, Traits>& ostrm, const IStringable& stringable )
{ return std::operator<<(ostrm, stringable.ToString()); }

}

#endif
