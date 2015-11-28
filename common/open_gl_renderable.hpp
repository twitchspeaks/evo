#ifndef COMMON_OPEN_GL_RENDERABLE_HPP
#define COMMON_OPEN_GL_RENDERABLE_HPP

#include "common/result.hpp"
#include "common/time_measures.hpp"
#include "common/i_stringable.hpp"

namespace evo {

class OpenGlRenderable : public IStringable
{
public:

  OpenGlRenderable ();

  virtual ~OpenGlRenderable ();

  TimePoint prev_render_time () const {
    return prev_render_tp_;
  }

  Result RenderMain (TimePoint tp);

  virtual Result Render (TimePoint tp) = 0;

  virtual std::string ToString ();

private:

  TimePoint prev_render_tp_;
};

}

#endif

