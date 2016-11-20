#ifndef CFLAGS_H_
#define CFLAGS_H_ 1

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <getopt.h>

namespace cflags {

// Pointer to the function the cflags library will use to handle fatal error
// errors.  It is supposed to never return, although this is not a requirement.
//
// By default, this variable points to `errx`.
extern void (*error_handler)(int eval, const char* fmt, ...);

// Parses a command line.  If you use this function, you don't need to call
// `get_options`, `parse_flag`, or `print_help` yourself.
//
// This function will exit if one of the command line arguments is `--help`.
void parse(int argc, char** argv);

// Returns all configured flags for use with getopt_long().
//
// This function does not add the final terminating option structure.  To add
// this yourself, call
//
//     options.emplace_back(option{nullptr, 0, nullptr, 0});
//
// where `options` is the value returned from this function.
std::vector<option> get_options(int val_base = 0);

// Parses a single flag, as returned from getopt_long().
//
// The value passed as `val` should be the value returned by `getopt_long`,
// minus the value of `val_base` passwd to `get_options`.
//
// The `optarg` value should be the global value declared in the GNU getopt
// library.
void parse_flag(int val, const char* optarg);

// Prints help output.
//
// Handles basic word-wrapping and line breaks.
void print_help();

#define CFLAGS_SECTION __attribute__((section("cflags")))

// Exports a variable so that it can be set from the command line.
//
// Example:
//
//     int width;
//     std::string style;
//     CFLAGS_EXPORT(width, "COLS", "set output width to COLS");
//     CFLAGS_EXPORT(time_style, "STYLE",
//                   "show times using style STYLE:\n"
//                   "full-iso: YYYY-MM-DDTHH:MM:SS\n"
//                   "+FORMAT: custom format");
#define CFLAGS_EXPORT(var_name, var_placeholder, var_description)    \
  static_assert(::cflags::Parser<decltype(var_name)>::ok,            \
                "No parser for type");                               \
  static const ::cflags::FlagInfo cflags__info_##var_name = {        \
      .name = #var_name,                                             \
      .parse = ::cflags::Parser<decltype(var_name)>::parse,          \
      .description = var_description,                                \
      .placeholder = var_placeholder,                                \
      .file = __FILE__,                                              \
      .data = reinterpret_cast<void*>(&var_name)};                   \
  const ::cflags::FlagInfo* const cflags_##var_name CFLAGS_SECTION = \
      &cflags__info_##var_name;

struct FlagInfo {
  const char* name;
  bool (*parse)(void* target, const char* string, const char** endptr);
  const char* description;
  const char* placeholder;
  const char* file;
  void* data;
};

template <typename T>
struct Parser {
  static constexpr bool ok = false;
  static constexpr bool scalar = false;
  static bool parse(void* target, const char* string, const char** endptr) {
    return false;
  }
};

#define CFLAGS_DECLARE_PARSER(type)                                           \
  template <>                                                                 \
  struct Parser<type> {                                                       \
    static constexpr bool ok = true;                                          \
    static constexpr bool scalar = true;                                      \
    static bool parse(void* target, const char* string, const char** endptr); \
  }

CFLAGS_DECLARE_PARSER(float);
CFLAGS_DECLARE_PARSER(double);
CFLAGS_DECLARE_PARSER(long double);
CFLAGS_DECLARE_PARSER(int8_t);
CFLAGS_DECLARE_PARSER(uint8_t);
CFLAGS_DECLARE_PARSER(int16_t);
CFLAGS_DECLARE_PARSER(uint16_t);
CFLAGS_DECLARE_PARSER(int32_t);
CFLAGS_DECLARE_PARSER(uint32_t);
CFLAGS_DECLARE_PARSER(int64_t);
CFLAGS_DECLARE_PARSER(uint64_t);
CFLAGS_DECLARE_PARSER(std::string);

// Parser for other string types.
template <typename Char, typename Traits, typename Allocator>
struct Parser<std::basic_string<Char, Traits, Allocator>> {
  static constexpr bool ok = true;
  static constexpr bool scalar = true;

  static bool parse(void* target, const char* string, const char** endptr) {
    const auto length = std::char_traits<char>::length(string);
    reinterpret_cast<std::basic_string<Char, Traits, Allocator>*>(target)
        ->assign(string, string + length);
    *endptr = string + length;
    return true;
  }
};

template <typename Container>
inline bool parse_into_container(Container& target, const char* string,
                                 const char** endptr) {
  using ValueType = typename Container::value_type;

  static_assert(Parser<ValueType>::scalar, "No parser for vector value type");

  for (;;) {
    target.emplace_back();
    const auto ok = Parser<ValueType>::parse(&target.back(), string, endptr);
    if (!ok) return false;
    if (**endptr != ',') return true;
    string = *endptr + 1;
  }
}

// Parser for vector types.  To add multiple values to a vector, use the same
// option multiple times, e.g. --foo=1 --foo=2 --foo=3.
//
// If the underlying data type cannot contain commas, --foo=1,2,3 will also
// work.
template <typename U>
struct Parser<std::vector<U>> {
  static constexpr bool ok = true;
  static constexpr bool scalar = false;

  static bool parse(void* target, const char* string, const char** endptr) {
    auto& vector = *reinterpret_cast<std::vector<U>*>(target);
    return parse_into_container(*reinterpret_cast<std::vector<U>*>(target),
                                string, endptr);
  }
};

}  // namespace cflags

#endif  // !CFLAGS_H_
