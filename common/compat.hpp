#ifndef COMMON_COMPAT_HPP
#define COMMON_COMPAT_HPP

// For gcc <= 3.6
#if __GNUC__ <= 3 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 6)
  #define override ;
#endif

#endif
