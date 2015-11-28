#ifndef COMMON_I_CLONEABLE_HPP
#define COMMON_I_CLONEABLE_HPP

#include <memory>

namespace evo {

/**
 *  An interface for classes that can be heap copied.
 */
class ICloneable
{
public:

  virtual ~ICloneable ()
  {}

  /**
   * Creates a newly allocated copy of the object.
   * @return A shared_ptr pointing to the object copy.
   */
  virtual std::shared_ptr<ICloneable> Clone () const = 0;
};

}

#endif
