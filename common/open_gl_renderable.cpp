#include <string>
#include <sstream>
#include <cstring>

#include "common/result.hpp"
#include "common/open_gl_renderable.hpp"

using namespace std;
using namespace evo;
using namespace std_results;

OpenGlRenderable :: OpenGlRenderable ()
{
}

OpenGlRenderable :: ~OpenGlRenderable ()
{
}

Result OpenGlRenderable :: RenderMain (TimePoint tp)
{
  Result res = Render(tp);
  
  prev_render_tp_ = tp;
  
  return res;
}

string OpenGlRenderable :: ToString () const
{
  return "";
}

