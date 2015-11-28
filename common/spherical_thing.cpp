#include <string>
#include <sstream>
#include <cstring>

#include "common/result.hpp"
#include "wiztest/src/wiz.hpp"

using namespace std;
using namespace evo;
using namespace std_results;

SphericalThing :: SphericalThing ()
{
}

string SphericalThing :: ToString () const
{
  std::stringstream strm;
  strm << "mass = " << mass_
       << ", radius = " << radius_
       << ", pos = { " << pos_.ToString()
       << " }, vel = { " << vel_.ToString()
       << " }, accel = { " << accel_.ToString() << " }";
  return strm.str();
}

