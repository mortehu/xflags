#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

#include <elf.h>
#include <err.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <unistd.h>

#include "xflags.h"

namespace {

// Print help and exit.
bool help;

// Print version information and exit.
bool version;

}  // namespace

XFLAGS_EXPORT(help, nullptr, "print this help and exit");
XFLAGS_EXPORT(version, nullptr, "print version information and exit");

template <typename ElfType>
struct ElfClasses {};

template <>
struct ElfClasses<Elf32_Ehdr> {
  using Section = Elf32_Shdr;
};

template <>
struct ElfClasses<Elf64_Ehdr> {
  using Section = Elf64_Shdr;
};

template <typename ElfType>
std::vector<std::string> parse_elf(const ElfType* data) {
  using Section = typename ElfClasses<ElfType>::Section;

  auto base = reinterpret_cast<const char*>(data);

  auto string_table = reinterpret_cast<const Section*>(
      base + data->e_shoff + data->e_shentsize * data->e_shstrndx);
  if (string_table->sh_type != SHT_STRTAB)
    errx(EX_DATAERR, "Corrupt string table");
  auto strings = base + string_table->sh_offset;

  auto section = reinterpret_cast<const Section*>(base + data->e_shoff);

  for (size_t i = 0; i < data->e_shnum; ++i) {
    if (0 == std::strcmp(strings + section->sh_name, ".xflags-names")) {
      const char* begin = base + section->sh_offset;
      const char* end = begin + section->sh_size;

      std::vector<std::string> result;

      while (begin != end) {
        if (*begin == '\0') {
          ++begin;
          continue;
        }

        const char* segment_end = begin + 1;
        while (segment_end != end && *segment_end != '\0') ++segment_end;

        std::string argument = "--";
        argument.append(begin, segment_end);
        result.emplace_back(std::move(argument));

        begin = segment_end;
      }

      return result;
    }

    section = reinterpret_cast<const Section*>(
        reinterpret_cast<const char*>(section) + data->e_shentsize);
  }

  return {};
}

int main(int argc, char** argv) {
  std::string executable, filter;
  std::string prev_argument;

  if (getenv("COMP_LINE")) {
    executable = getenv("COMP_LINE");

    auto space = executable.find(' ');
    if (space != std::string::npos)
      executable.erase(space);

    bool found = true;

    if (executable.find('/') == std::string::npos) {
      const char* path_str = getenv("PATH");
      found = false;

      if (path_str) {
        std::istringstream path(path_str);

        std::string path_element;
        while (std::getline(path, path_element, ':')) {
          if (path_element.back() != '/') path_element.push_back('/');
          path_element += executable;

          if (0 == ::access(path_element.c_str(), F_OK)) {
            found = true;
            executable = std::move(path_element);
            break;
          }
        }
      }
    }

    if (!found) return EXIT_FAILURE;

    if (argc > 2)
      filter = argv[2];

    if (argc > 3)
      prev_argument = argv[3];
  } else {
    auto options = xflags::get_options();
    options.emplace_back(option{nullptr, 0, nullptr, 0});

    int i;
    while (-1 != (i = getopt_long_only(argc, argv, "", options.data(), 0))) {
      if (i == 0) break;

      if (i == '?')
        errx(EX_USAGE, "Try '%s --help' for more information.", argv[0]);

      xflags::parse_flag(i, optarg);
    }

    if (help) {
      std::cout << "Usage: " << argv[0] << " [OPTION]... ELF-PROGRAM\n\n"
                << "Prints the command line flags supported by ELF-PROGRAM.\n"
                << "\n";
      xflags::print_help();
      std::cout << "\n"
                << "Using with bash:\n"
                << "\n"
                << "  complete -C xflags-complete COMMAND\n"
                << "\n"
                << "COMMAND is a command using the xflags library.  xflags-complete itself\n"
                << "is such a command, so the following example works:\n"
                << "\n"
                << "  complete -C xflags-complete xflags-complete\n"
                << "\n"
                << "Report bugs to: morten.hustveit@gmail.com\n";

      return EXIT_SUCCESS;
    }

    if (version) {
      std::cout << PACKAGE_STRING << '\n';
      return EXIT_SUCCESS;
    }

    if (optind + 1 != argc)
      errx(EX_USAGE, "Usage: %s [OPTION]... EXECUTABLE", argv[0]);
    executable = argv[optind];
  }

  int fd = open(executable.c_str(), O_RDONLY);
  if (fd == -1)
    err(EX_NOINPUT, "Could not open '%s' for reading", executable.c_str());

  off_t length = lseek(fd, 0, SEEK_END);
  if (length == -1)
    err(EX_IOERR, "Failed to seek to end of '%s'", executable.c_str());

  void* map = mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED)
    err(EX_IOERR, "Failed to memory-map '%s'", executable.c_str());

  auto elf32 = reinterpret_cast<const Elf32_Ehdr*>(map);
  if (0 != std::memcmp(elf32->e_ident, ELFMAG, SELFMAG))
    errx(EX_DATAERR, "Not an ELF file");

  std::vector<std::string> arguments;

  switch (elf32->e_ident[EI_CLASS]) {
    case ELFCLASS32:
      arguments = parse_elf(elf32);
      break;

    case ELFCLASS64:
      arguments = parse_elf(reinterpret_cast<const Elf64_Ehdr*>(map));
      break;

    default:
      errx(EX_DATAERR, "Unrecognized ELF class %d", elf32->e_ident[EI_CLASS]);
  }

  if (arguments.size() == 1 && prev_argument == arguments[0]) return EXIT_SUCCESS;

  std::sort(arguments.begin(), arguments.end());

  for (auto&& argument : arguments) {
    if (argument.size() < filter.size() ||
        0 != argument.compare(0, filter.size(), filter))
      continue;

    std::cout << argument << '\n';
  }
}
