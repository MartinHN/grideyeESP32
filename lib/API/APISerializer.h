#pragma once

#include <string>

namespace APISerializer {
template <typename C> static std::string APIMembersToString(const API<C> &api) {
  std::vector<std::string> res;
  for (auto &m : api.members)
    res.push_back(m.first.toString() + " : " + m.second->getTypeName());

  return joinIntoString(res);
}
} // namespace APISerializer
