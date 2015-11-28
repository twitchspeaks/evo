#include <string>
#include <sstream>
#include <cstring>

#include "common/result.hpp"
#include "wiztest/src/wiz.hpp"

using namespace std;
using namespace evo;
using namespace std_results;

Wiz :: Wiz ()
{
}

Wiz :: ~Wiz ()
{
}

string Wiz :: ToString () const
{
  return SphericalThing::ToString();
}
