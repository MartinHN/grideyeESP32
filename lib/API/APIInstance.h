#pragma once
#include "APIBase.h"

#define USE_CEREAL 1

#if USE_CEREAL
#include <cereal/archives/json.hpp>
// #include <cereal/types/memory.hpp>
// #include <cereal/types/unordered_map.hpp>

#endif

struct APIInstanceBase {
  virtual const APIBase &getAPI() = 0;
  virtual QResult get(const Identifier &n) = 0;
  virtual bool set(const Identifier &n, const TypedArgList &args) = 0;
  virtual QResult call(const Identifier &n, const TypedArgList &args) = 0;
  virtual bool canGet(const Identifier &id) const = 0;
  virtual bool canSet(const Identifier &id) const = 0;
  virtual bool canCall(const Identifier &id) const = 0;
  virtual std::string toString() const = 0;
  virtual QResult fromString(const std::string &) = 0;
};

template <typename C> struct APIInstance : public APIInstanceBase {

  APIInstance(C &o, const API<C> &a) : obj(o), api(a) {}
  const APIBase &getAPI() override { return api; }
  QResult get(const Identifier &n) override {
    if (auto *getter = getOrNull(api.getters, n)) {
      QResult r;
      bool success =
          tryGet<C, float>(obj, getter, r) || tryGet<C, int>(obj, getter, r);
      if (success) {
        return r;
      }
    }
    return QResult::err("cannot get ");
  }

  bool set(const Identifier &n, const TypedArgList &args) override {
    if (auto *member = getOrNull(api.members, n)) {
      if (args.size() == 1) {
        auto *v = args[0];
        return trySet<C, float>(obj, member, v) ||
               trySet<C, int>(obj, member, v);
      }
    }
    return false;
  }

  QResult call(const Identifier &n, const TypedArgList &args) override {
    if (auto *fun = api.getFunction(n)) {
      return tryCall<C>(obj, *fun, args);
    }
    return QResult::err("no function found");
  }

  bool canGet(const Identifier &id) const override { return api.canGet(id); }
  bool canSet(const Identifier &id) const override { return api.canSet(id); }
  bool canCall(const Identifier &id) const override { return api.canCall(id); }

#if USE_CEREAL

  template <class AR> void serialize(AR &ar) const {
    bool success = true;
    for (auto &m : api.members) {
      success &= (chkAdd<AR, float>(ar, m));
    }
  }

  template <class AR, typename T, typename IT>
  bool chkAdd(AR &ar, const IT &m) const {
    if (auto *mv = dynamic_cast<const Member<C, T> *>(m.second)) {
      ar(cereal::make_nvp(m.first, mv->get(obj)));
      return true;
    }
    return false;
  }
#endif
  std::string toString() const override {
    std::string res;

#if USE_CEREAL
    std::ostringstream stream;
    {
      cereal::JSONOutputArchive archive(stream);
      archive(cereal::make_nvp("members", *this));
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

  QResult fromString(const std::string &s) override {
    auto res = QResult::ok<std::string>("success");
#if USE_CEREAL
    {
      std::istringstream stream(s);
      cereal::JSONInputArchive archive(stream);
      archive(cereal::make_nvp("members", *this));
      // need delete archive to flush
    }

#else
#endif
    return res;
  }
  C &obj;
  const API<C> &api;
};
