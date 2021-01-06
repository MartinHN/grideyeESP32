#pragma once
#include "APIBase.h"


#if USE_CEREAL
#include <cereal/archives/json.hpp>
#endif

struct APIInstanceBase {

  virtual ~APIInstanceBase() = default;
  virtual const APIBase &getAPI() = 0;
  virtual QResult get(const Identifier &n) = 0;
  virtual QResult set(const Identifier &n, const TypedArgList &args) = 0;
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
    PRINT("try get : ");
    PRINTLN(n.c_str());
    if (auto *getter = getOrNull(api.getters, n)) {
      PRINTLN("try getters");
      QResult r;
      bool success = tryGet<C, float>(obj, getter, r) ||
                     tryGet<C, int>(obj, getter, r) ||
                     tryGet<C, bool>(obj, getter, r) ||
                     tryGet<C, std::string>(obj, getter, r);
      if (success) {
        return r;
      }
    }
    if (auto *getter = getOrNull(api.members, n)) {
      PRINTLN("try members");
      QResult r;
      bool success = tryGet<C, float>(obj, getter, r) ||
                     tryGet<C, int>(obj, getter, r) ||
                     tryGet<C, bool>(obj, getter, r) ||
                     tryGet<C, std::string>(obj, getter, r);
      if (success) {
        return r;
      }
    }
    return QResult::err("cannot get ");
  }

  QResult set(const Identifier &n, const TypedArgList &args) override {
    PRINTLN("try set  ");
    if (auto *member = getOrNull(api.members, n)) {
      if (args.size() == 1) {
        auto *v = args[0];
        bool success = trySet<C, float>(obj, member, v) ||
                       trySet<C, int>(obj, member, v) ||
                       trySet<C, bool>(obj, member, v) ||
                       trySet<C, std::string>(obj, member, v);
        return QResult(!success);
      }
      return QResult::err("cannot set wrong num args");
    }
    return QResult::err("cannot set no member");
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
      success &= (chkAdd<AR, float>(ar, m) || chkAdd<AR, int>(ar, m) ||
                  chkAdd<AR, bool>(ar, m) || chkAdd<AR, std::string>(ar, m));
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

// defines both API and instanciation
template <typename C>
struct APIAndInstance : public API<C>, public APIInstance<C> {
  APIAndInstance(C &obj) : APIInstance<C>(obj, *(API<C> *)(this)) {}
};
