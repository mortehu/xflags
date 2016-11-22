#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <type_traits>

#include "xflags.h"

#include <err.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sysexits.h>
#include <unistd.h>

namespace xflags {

void (*error_handler)(int eval, const char* fmt, ...) = errx;

namespace {

const char nul = '\0';

// Parses signed integers.
template <typename T>
typename std::enable_if<std::numeric_limits<T>::is_signed, bool>::type
parse_integer(void* target, const char* string, const char** endptr) {
  errno = 0;
  auto result = std::strtoll(string, const_cast<char**>(endptr), 0);
  if (errno != 0 || result > std::numeric_limits<T>::max() ||
      result < std::numeric_limits<T>::min() || *endptr == string)
    return false;
  *reinterpret_cast<T*>(target) = result;
  return true;
}

// Parses unsigned integers.
template <typename T>
typename std::enable_if<!std::numeric_limits<T>::is_signed, bool>::type
parse_integer(void* target, const char* string, const char** endptr) {
  errno = 0;
  auto result = std::strtoull(string, const_cast<char**>(endptr), 0);
  if (errno != 0 || result > std::numeric_limits<T>::max() ||
      result < std::numeric_limits<T>::min() || *endptr == string)
    return false;
  *reinterpret_cast<T*>(target) = result;
  return true;
}

}  // namespace

// Parses float values.
bool Parser<float>::parse(void* target, const char* string,
                          const char** endptr) {
  errno = 0;
  *reinterpret_cast<float*>(target) =
      std::strtof(string, const_cast<char**>(endptr));
  if (errno != 0 || *endptr == string) return false;
  return true;
}

// Parses double values.
bool Parser<double>::parse(void* target, const char* string,
                           const char** endptr) {
  errno = 0;
  *reinterpret_cast<double*>(target) =
      std::strtod(string, const_cast<char**>(endptr));
  if (errno != 0 || *endptr == string) return false;
  return true;
}

// Parses long double values.
bool Parser<long double>::parse(void* target, const char* string,
                                const char** endptr) {
  errno = 0;
  *reinterpret_cast<long double*>(target) =
      std::strtold(string, const_cast<char**>(endptr));
  if (errno != 0 || *endptr == string) return false;
  return true;
}

// All integer types can be parsed by the `parse_integer` functions, so we use
// a macro to avoid too much repetition.
#define DEFINE_INT_PARSER(int_type)                              \
  bool Parser<int_type>::parse(void* target, const char* string, \
                               const char** endptr) {            \
    return parse_integer<int_type>(target, string, endptr);      \
  }

DEFINE_INT_PARSER(int8_t);
DEFINE_INT_PARSER(uint8_t);
DEFINE_INT_PARSER(int16_t);
DEFINE_INT_PARSER(uint16_t);
DEFINE_INT_PARSER(int32_t);
DEFINE_INT_PARSER(uint32_t);
DEFINE_INT_PARSER(int64_t);
DEFINE_INT_PARSER(uint64_t);

bool Parser<bool>::parse(void* target, const char* string,
                         const char** endptr) {
  if (string == nullptr) {
    *reinterpret_cast<bool*>(target) = true;
    *endptr = &nul;
    return true;
  }

  const auto length = std::char_traits<char>::length(string);

  if ((1 == length &&
       0 == std::char_traits<char>::compare(string, "1", length)) ||
      (4 == length &&
       0 == std::char_traits<char>::compare(string, "true", length))) {
    *reinterpret_cast<bool*>(target) = true;
    *endptr = string + length;
    return true;
  }

  if ((1 == length &&
       0 == std::char_traits<char>::compare(string, "0", length)) ||
      (5 == length &&
       0 == std::char_traits<char>::compare(string, "false", length))) {
    *reinterpret_cast<bool*>(target) = false;
    *endptr = string + length;
    return true;
  }

  return false;
}

bool Parser<std::string>::parse(void* target, const char* string,
                                const char** endptr) {
  reinterpret_cast<std::string*>(target)->assign(string);
  *endptr = "";
  return true;
}

std::vector<option> get_options(int val_base) {
  std::vector<option> options;
  options.reserve(&end - &begin);

  int option_idx = 1;
  for (int option_idx = 1; &begin + option_idx != &end; ++option_idx) {
    const FlagInfo& info = **(&begin + option_idx);

    options.emplace_back(
        option{info.name,
               info.requires_argument ? required_argument : optional_argument,
               nullptr, option_idx + static_cast<int>(val_base)});
  }

  return options;
}

void parse_flag(int val, const char* optarg) {
  static const auto option_count = &end - &begin - 1;

  if (val < 1 || &begin + val >= &end) {
    error_handler(EXIT_FAILURE, "Invalid option value");
    return;
  }

  const FlagInfo& info = **(&begin + val);

  const char* endptr = nullptr;
  if (!info.parse(info.data, optarg, &endptr))
    error_handler(EX_USAGE, "Invalid value --%s=%s", info.name, optarg);

  if (*endptr != '\0')
    error_handler(EX_USAGE, "Garbage in value --%s=%s: %s", info.name, optarg,
                  endptr);
}

void parse(int argc, char** argv) {
  if (argc == 0) return;

  int do_print_help = 0;

  auto options = get_options();
  options.emplace_back(option{"help", no_argument, &do_print_help, 1});
  options.emplace_back(option{nullptr, 0, nullptr, 0});

  int i;
  while (-1 != (i = getopt_long_only(argc, argv, "", options.data(), 0))) {
    if (i == 0) break;

    if (i == '?') {
      error_handler(EX_USAGE, "Try '%s --help' for more information.", argv[0]);
      return;
    }

    parse_flag(i, optarg);
  }

  if (do_print_help) {
    std::cout << "Usage: " << argv[0] << " [OPTION]...\n\n";
    print_help();
    std::cout << "      --help                 display this help and exit\n";
    std::exit(EXIT_SUCCESS);
  }
}

void print_help() {
  static const auto option_count = &end - &begin - 1;
  if (option_count == 0) return;

  uint16_t column_count = 80;

  winsize ws;
  if (0 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) {
    column_count = ws.ws_col;
    if (column_count > 100) column_count = 100;
  }

  const char* file = nullptr;
  const bool multiple_files = (*(&begin + 1))->file != (*(&end - 1))->file;

  for (auto fp = &begin + 1; fp != &end; ++fp) {
    const FlagInfo& info = **fp;

    if (info.file != file && multiple_files) {
      if (file != nullptr) std::cout.put('\n');
      std::cout << "Options in " << info.file << ":\n";
      file = info.file;
    }
    const auto name_length = std::char_traits<char>::length(info.name);

    std::cout << "      --";
    std::cout.write(info.name, name_length);

    size_t column = name_length + 8;

    if (info.placeholder) {
      const auto placeholder_length =
          std::char_traits<char>::length(info.placeholder);
      std::cout.put('=');
      std::cout.write(info.placeholder, placeholder_length);

      column += placeholder_length + 1;
    }

    if (column >= 28) {
      std::cout.put('\n');
      column = 0;
    }

    const char* range_begin = info.description;
    size_t description_column = 29;

    while (*range_begin) {
      while (column < description_column) {
        std::cout.put(' ');
        ++column;
      }

      const char* range_end = range_begin;

      // We put at least one word per line, regardless of length.
      while (*range_end && *range_end != ' ') ++range_end;

      // Try to fit more words on the line.
      for (const char* c = range_end; c - range_begin + column < column_count;
           ++c) {
        if (*c == '\0' || std::isspace(*c)) {
          range_end = c;
          if (*c == '\0' || *c == '\n') break;
        }
      }

      std::cout.write(range_begin, range_end - range_begin);
      std::cout.put('\n');
      column = 0;

      while (*range_end != '\0' && std::isspace(*range_end)) ++range_end;

      range_begin = range_end;
      description_column = 31;
    }

    if (column > 0) std::cout.put('\n');
  }

  if (multiple_files) std::cout.put('\n');
}

}  // namespace
