#pragma once

#include <functional>
#include <string>

namespace StringHelpers {

template <typename T> //,
                      // std::enable_if_t<std::is_arithmetic<T>::value, bool> =
                      // true>
std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value, std::string>
toString(const T &v) {
  return std::to_string(v);
  // return "unknown";
};
// template <typename T>
// toString(const T &v) {
//   return std::to_string((T)v);
// };

template <typename T>
std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value, std::string>
toString(const T &v) {
  return std::string(v);
}

template <> std::string toString<float>(const float &v) {
  return std::to_string(v);
}

template <typename AT, typename ET = std::remove_reference_t<
                           decltype(*std::begin(std::declval<AT &>()))>>
static std::string joinIntoString(
    const AT &alist,
    std::function<std::string(const ET &e)> toStr =
        [](const ET &e) { return e; },
    const std::string &delim = ",") {

  bool isFirst = true;
  std::string res;
  for (const auto &e : alist) {
    if (!isFirst) {
      res += delim;
    }
    isFirst = false;
    res += toStr(e);
  }
  return res;
}

} // namespace StringHelpers
