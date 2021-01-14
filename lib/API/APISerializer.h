#pragma once

#if USE_SERIALIZER
#if USE_CEREAL
#include <cereal/archives/json.hpp>
#endif

#include "APIInstance.h"
#include <string>

namespace APISerializer {

#if USE_CEREAL
template <typename C> struct SerializableAPI {
  SerializableAPI(APIInstance<C> &api) : apiI(api) {}
  template <class AR> void serialize(AR &ar) const {
    bool success = true;
    for (auto &m : apiI.api.members) {
      success &= chkAdd<AR, float>(ar, m) || chkAdd<AR, int>(ar, m) ||
                 chkAdd<AR, double>(ar, m) || chkAdd<AR, std::string>(ar, m);
    }
    if (!success) {
      PRINTLN("!!!serialization error");
    }
  }

  template <class AR, typename T, typename IT>
  bool chkAdd(AR &ar, const IT &m) const {
    if (auto *mv = dynamic_cast<const Member<C, T> *>(m.second)) {
      ar(cereal::make_nvp(m.first, mv->get(apiI.obj)));
      return true;
    }
    return false;
  }
  APIInstance<C> &apiI;
};
#endif

template <typename C> static std::string membersToString(APIInstance<C> &api) {

  std::string res;

#if USE_CEREAL
  std::ostringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    archive(cereal::make_nvp("members", SerializableAPI<C>(api)));
    // need delete archive to flush
  }
  res = stream.str();
#else
  std::vector<std::string> tmp;
  for (const auto &m : api.members) {
    tmp.push_back(m.first + ":" + m.second->toString(&obj));
  };
  if (tmp.size())
    res += std::string(res.size() ? "," : "") + "members:{" +
           StringHelpers::joinIntoString(tmp) + "}";
#endif

  return res;
}

template <typename C>
static QResult membersFromString(const API<C> &api, const std::string &s) {
  auto res = QResult::ok<std::string>("success");
#if USE_CEREAL
  {
    std::istringstream stream(s);
    cereal::JSONInputArchive archive(stream);
    archive(cereal::make_nvp("members", SerializableAPI<C>(api)));
    // need delete archive to flush
  }

#else
#error " not implemented"
#endif
  return res;
}

} // namespace APISerializer

#endif // USE_SERIALIZER
