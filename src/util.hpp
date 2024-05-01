#pragma once

#include <array>
#include <charconv>
#include <cstddef>
#include <limits>
#include <source_location>
#include <string>
#include <system_error>
#include <vector>

namespace util {

using source_location = std::source_location;

void debug(const std::string& s, source_location location = source_location::current());

void fatal(const std::string& s, source_location location = source_location::current());

void critical(const std::string& s, source_location location = source_location::current());

void warning(const std::string& s, source_location location = source_location::current());

void info(const std::string& s, source_location location = source_location::current());

template <typename T>
auto to_string(const T& num, const std::string def = "0") -> std::string {
  // This is used to replace `std::to_string` as a locale independent
  // number conversion using `std::to_chars`.
  // An additional string parameter could be eventually provided with a
  // default value to return in case the conversion fails.

  // Max buffer length:
  // number of base-10 digits that can be represented by the type T without change +
  // number of base-10 digits that are necessary to uniquely represent all distinct
  // values of the type T (meaningful only for real numbers) +
  // room for other characters such as "+-e,."
  const size_t max = std::numeric_limits<T>::digits10 + std::numeric_limits<T>::max_digits10 + 10U;

  std::array<char, max> buffer;

  const auto p_init = buffer.data();

  const auto result = std::to_chars(p_init, p_init + max, num);

  return (result.ec == std::errc()) ? std::string(p_init, result.ptr - p_init) : def;
}

template <typename T>
auto str_to_num(const std::string& str, T& num) -> bool {
  // This is a more robust implementation of `std::from_chars`
  // so that we don't have to do every time with `std::from_chars_result` structure.
  // We don't care of error types, so a simple bool is returned on success/fail.
  // A left trim is performed on strings so that the conversion could success
  // even if there are leading whitespaces and/or the plus sign.

  auto first_char = str.find_first_not_of(" +\n\r\t");

  if (first_char == std::string::npos) {
    return false;
  }

  const auto result = std::from_chars(str.data() + first_char, str.data() + str.size(), num);

  return (result.ec == std::errc());
}

}  // namespace util
