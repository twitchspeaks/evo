#!/bin/bash

#autoreconf --force --install -I config -I m4
autoreconf --verbose --force --install -I m4
sed -i -r 's/^\@SET_MAKE\@$/# \0/' Makefile.in
