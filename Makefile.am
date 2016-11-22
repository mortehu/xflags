bin_PROGRAMS = xflags-complete
include_HEADERS = xflags.h
lib_LIBRARIES = libxflags.a
man3_MANS = xflags.3
pkgdata_DATA = 0_xflags_before.cc
noinst_PROGRAMS = example

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xflags.pc

AM_CXXFLAGS = -std=c++11

libxflags_a_SOURCES = xflags_after.cc xflags.cc xflags.h xflags-internal.h

xflags_complete_SOURCES = 0_xflags_before.cc xflags-complete.cc
xflags_complete_LDADD = libxflags.a

example_SOURCES = 0_xflags_before.cc example.cc
example_LDADD = libxflags.a
