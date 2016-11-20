#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "cflags.h"

#include <err.h>
#include <getopt.h>
#include <sysexits.h>

float benny_the_floating_point_variable;
int32_t x = 1;
uint64_t y;
std::wstring time_style;
std::vector<int> v;

CFLAGS_EXPORT(x, "INT", "my first variable");
CFLAGS_EXPORT(y, "INT", "my second variable");
CFLAGS_EXPORT(time_style, "STYLE",
              "show times using style STYLE:\n"
              "full-iso: YYYY-MM-DDTHH:MM:SS\n"
              "+FORMAT: custom format");
CFLAGS_EXPORT(v, "LIST", "my vector");

int main(int argc, char** argv) {
  cflags::parse(argc, argv);
  std::cout << "v=";
  for (auto&& value : v) std::cout << value << ',';
  std::cout << '\n';
  std::cout << "x=" << x << '\n';
  std::cout << "y=" << y << '\n';
  std::wcout << "time_style=" << time_style << '\n';
  std::cout << "benny_the_floating_point_variable="
            << benny_the_floating_point_variable << '\n';
}
