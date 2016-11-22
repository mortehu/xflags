noinst_PROGRAMS = example
lib_LIBRARIES = libxflags_after.a
include_HEADERS = xflags.h
pkgdata_DATA = xflags_before.cc

AM_CXXFLAGS = -std=c++11

libxflags_after_a_SOURCES = xflags_after.cc xflags.cc xflags.h xflags-internal.h

example_SOURCES = xflags_before.cc example.cc
example_LDADD = libxflags_after.a
