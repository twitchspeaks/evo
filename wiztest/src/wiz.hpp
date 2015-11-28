#ifndef WIZTEST_SRC_WIZ_HPP
#define WIZTEST_SRC_WIZ_HPP

#include <string>
#include <sstream>

#include "common/spherical_thing.hpp"
#include "common/util.hpp"

namespace evo {

class Wiz : public SphericalThing
{
public:

  Wiz ();

  virtual ~Wiz ();

  std::string ToString () const;

private:

  float energy_;
};

}

#endif
