#pragma once

#if USE_SERIALIZER
#if USE_CEREAL
#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>

#endif

#include "APIInstance.h"
#include <algorithm>
#include <string>

namespace APISerializer {

#if USE_CEREAL

#define SKIP_OBJECT_DEFS 1
template <typename T> struct SimpleMap {
  SimpleMap(T &ob) : ob(ob) {}
  template <class AR> void serialize(AR &ar) const {
    for (const auto &m : ob) {
      ar(cereal::make_nvp(m.first, m.second));
    }
  }
  T &ob;
};

#if !SKIP_OBJECT_DEFS
template <typename T> struct TypeCont {
  template <typename... Args> TypeCont(Args... args) : ob(args...) {}
  template <class AR> void serialize(AR &ar) const {

    ar(cereal::make_nvp("type", std::string("object")));
    ar(cereal::make_nvp("properties", ob));
  }
  T ob;
};
#else
template <typename T> struct TypeCont : public T {
  template <typename... Args> TypeCont(Args... args) : T(args...) {}
};
#endif

struct APIMemberBase {
  APIMemberBase(MemberBase *m) : member(m) {}
  template <class AR> void serialize(AR &ar) const {
    bool s;
    s = chkAddM<std::string>(ar) || chkAddM<float>(ar) || chkAddM<bool>(ar) ||
        chkAddM<double>(ar) || chkAddM<int>(ar);
    if (!s) {
      PRINTLN("!!!serialization error");
    }
  }
  template <typename T, typename AR> bool chkAddM(AR &ar) const {
    if (auto *mv = dynamic_cast<const MemberBaseT<T> *>(member)) {
      ar(cereal::make_nvp("type", StringHelpers::getTypeName<T>()));
      return true;
    }
    return false;
  }
  MemberBase *member;
};

struct APISchema {
  APISchema(APIBase &api) : api(api) {}
  template <class AR> void serialize(AR &ar) const {

    std::map<std::string, APIMemberBase> mMap = {};
    for (auto &m : api.members) {
      mMap.emplace(m.first, APIMemberBase(m.second));
    }
    ar(cereal::make_nvp("members", TypeCont<SimpleMap<decltype(mMap)>>(mMap)));
  }

  APIBase &api;
};

struct APISchemaNodeSerializer {

  APISchemaNodeSerializer(NodeBase *node, std::vector<NodeBase *> &usedNodes)
      : node(node), usedNodes(usedNodes) {
    usedNodes.push_back(node);
  }

  template <class AR> void serialize(AR &ar) {
    if (auto api = dynamic_cast<APIBase *>(node)) {
      ar(cereal::make_nvp("api", TypeCont<APISchema>(*api)));
    }

    std::map<std::string, TypeCont<APISchemaNodeSerializer>> serCh;

    if (auto nm = dynamic_cast<MapNode *>(node)) {
      for (auto &c : nm->m) {
        PRINT("used");
        PRINTLN(usedNodes.size());
        if (std::find(usedNodes.begin(), usedNodes.end(), c.second) !=
            std::end(usedNodes)) {
          PRINTLN("cyclic dep");
          continue;
        }
        PRINT("adding ");
        PRINTLN(c.first.c_str());

        serCh.emplace(c.first,
                      TypeCont<APISchemaNodeSerializer>(c.second, usedNodes));
      }
    }
    if (serCh.size()) {
      ar(cereal::make_nvp("/", SimpleMap<decltype(serCh)>(serCh)));
    }
  }
  NodeBase *node;
  std::vector<NodeBase *> &usedNodes;
};
#endif

static std::string schemaFromNode(NodeBase *n) {
  std::string res;
#if USE_CEREAL
  std::ostringstream stream;
  {
    cereal::JSONOutputArchive archive(stream);
    std::vector<NodeBase *> usedNodes;
    archive(cereal::make_nvp("type", std::string("object")));
    archive(
        cereal::make_nvp("properties", APISchemaNodeSerializer(n, usedNodes)));
    // need delete archive to flush
  }
  res = stream.str();
#else
#error "not supported"
#endif

  return res;
}

} // namespace APISerializer

#endif // USE_SERIALIZER
