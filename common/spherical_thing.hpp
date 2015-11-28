#ifndef COMMON_SPHERICAL_THING_HPP
#define COMMON_SPHERICAL_THING_HPP

#include <string>
#include <sstream>

#include "common/open_gl_renderable.hpp"
#include "common/util.hpp"

namespace evo {

class SphericalThing : public OpenGlRenderable
{
public:

  SphericalThing ();

  virtual ~SphericalThing ();

  float mass () const {
    return mass_;
  }

  void set_mass (float val) {
    mass_ = val;
  }

  float radius () const {
    return radius_;
  }

  void set_radius (float val) {
    radius_ = val;
  }

  const Coords3& pos () const {
    return pos_;
  }

  Coords3& pos () {
    return pos_;
  }

  void set_pos (const Coords3& val) {
    pos_ = val;
  }

  const Coords3& vel () const {
    return vel_;
  }

  Coords3& vel () {
    return vel_;
  }

  void set_vel (const Coords3& val) {
    vel_ = val;
  }

  const Coords3& accel () const {
    return accel_;
  }

  Coords3& accel () {
    return accel_;
  }

  void set_accel (const Coords3& val) {
    accel_ = val;
  }

  virtual std::string ToString () const;

private:

  float mass_;
  float radius_;

  Coords3 pos_;
  Coords3 vel_;
  Coords3 accel_;
};

}

#endif

