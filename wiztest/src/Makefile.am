#AM_CPPFLAGS = -ggdb3 -std=c++0x
AM_CXXFLAGS = -ggdb3 -std=c++0x

bin_PROGRAMS = wiztest

#AM_CXXFLAGS = $(INTI_CFLAGS)

wiztest_SOURCES = wiz.cpp wiztest.cpp
wiztest_LDADD = $(INTI_LIBS) ../../common/libevo.a -lGL -lGLU -lglut -lboost_system
