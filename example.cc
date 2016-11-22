#include <cstring>
#include <string>
#include <vector>

#include "xflags.h"

enum class MyCustomType { A, B };

namespace xflags {

// Parse MyCustomType; either "A" or "B".
template <>
struct Parser<MyCustomType> : public ScalarParser {
  static bool parse(void* target, const char* string, const char** endptr) {
    auto& t = *reinterpret_cast<MyCustomType*>(target);
    if (!std::strcmp(string, "A"))
      t = MyCustomType::A;
    else if (!std::strcmp(string, "B"))
      t = MyCustomType::B;
    else
      return false;

    *endptr = string + 1;  // Tell the caller we parsed one char.

    return true;
  }
};

}  // namespace xflags

uint16_t cols = 80;
XFLAGS_EXPORT(cols, "COLS", "set window width to COLS");

std::string date_format;
XFLAGS_EXPORT(date_format, "FORMAT",
              "set date format:\n"
              "iso8601: YYYY-MM-DDTHH:MM:SS\n"
              "rfc2822: Day, DD Mon YYYY HH:MM:SS TZ\n"
              "+FORMAT: custom format");

std::vector<time_t> times;
XFLAGS_EXPORT(times, "LIST", "times to display");

float weight;
XFLAGS_EXPORT(weight, "WEIGHT", "set weight to WEIGHT");

MyCustomType my_var;
XFLAGS_EXPORT(my_var, "VAL", "assign custom variable");

int main(int argc, char** argv) {
  xflags::parse(argc, argv);

  // ...
}
