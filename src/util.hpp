#ifndef UTIL_HPP
#define UTIL_HPP

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <ranges>
#include <source_location>
#include <thread>
#include <vector>

#ifdef __clang__
#include <experimental/source_location>
#endif

namespace util {

constexpr float minimum_db_level = -100.0F;
constexpr double minimum_db_d_level = -100.0;
constexpr float minimum_linear_level = 0.00001F;
constexpr double minimum_linear_d_level = 0.00001;

#ifdef __clang__
using source_location = std::experimental::source_location;
#else
using source_location = std::source_location;
#endif

void debug(const std::string& s, source_location location = source_location::current());
void error(const std::string& s, source_location location = source_location::current());
void critical(const std::string& s, source_location location = source_location::current());
void warning(const std::string& s, source_location location = source_location::current());
void info(const std::string& s, source_location location = source_location::current());

void reset_all_keys_except(GSettings* settings, const std::vector<std::string>& blocklist = std::vector<std::string>());

void idle_add(std::function<void()> cb);

template <class T>
concept Number = std::is_integral<T>::value || std::is_floating_point<T>::value;

template <Number T>
auto logspace(const T& start, const T& stop, const uint& npoints) -> std::vector<T> {
  std::vector<T> output;

  if (stop <= start || npoints < 2) {
    return output;
  }

  auto log10_start = std::log10(start);
  auto log10_stop = std::log10(stop);

  const T delta = (log10_stop - log10_start) / static_cast<T>(npoints - 1);

  output.push_back(start);

  T v = log10_start;

  while (output.size() < npoints - 1) {
    v += delta;

    if constexpr (std::is_same_v<T, float>) {
      output.push_back(std::pow(10.0F, v));
    } else {
      output.push_back(std::pow(10.0, v));
    }
  }

  output.push_back(stop);

  return output;
}

template <Number T>
auto linspace(const T& start, const T& stop, const uint& npoints) -> std::vector<T> {
  std::vector<T> output;

  if (stop <= start || npoints < 2) {
    return output;
  }

  const T delta = (stop - start) / static_cast<T>(npoints - 1);

  output.push_back(start);

  T v = start;

  while (output.size() < npoints - 1) {
    v += delta;

    output.push_back(v);
  }

  output.push_back(stop);

  return output;
}

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

}  // namespace util

#endif
