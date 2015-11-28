#ifndef WIZTEST_SRC_WIZ_HPP
#define WIZTEST_SRC_WIZ_HPP

#include <string>
#include <sstream>

#include "common/util.hpp"

namespace evo {

class EvoUniverse
{
public:

  EvoUniverse ();

  ~EvoUniverse ();

  std::string ToString () const;

private:

  /** Gravitational constant
  */
  double G_;

  std::vector<SphericalThing> things_;
};

}

#endif

