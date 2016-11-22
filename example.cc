#include <string>
#include <vector>

#include "xflags.h"

uint16_t cols = 80;
XFLAGS_EXPORT(cols, "COLS", "set window width to COLS");

std::string date_format;
XFLAGS_EXPORT(date_format, "FORMAT", "set date format:\n"
    "iso8601: YYYY-MM-DDTHH:MM:SS\n"
    "rfc2822: Day, DD Mon YYYY HH:MM:SS TZ\n"
    "+FORMAT: custom format");

std::vector<time_t> times;
XFLAGS_EXPORT(times, "LIST", "times to display");

float weight;
XFLAGS_EXPORT(weight, "WEIGHT", "set weight to WEIGHT");

int main(int argc, char** argv) {
  xflags::parse(argc, argv);

  // ...
}
