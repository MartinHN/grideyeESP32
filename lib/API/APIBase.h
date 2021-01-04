#pragma once
#include "APIMemberTypes.h"

#include <map>
#include <string>

template <typename T>
T *getOrNull(const std::map<Identifier, T *> &m, const Identifier &i) {
  auto it = m.find(i);
  if (it != m.end()) {
    return it->second;
  }
  return {};
}
template <typename T>
T *getOrNull(const std::map<Identifier, T> &m, const Identifier &i) {
  auto it = m.find(i);
  if (it != m.end()) {
    return &it->second;
  }
  return {};
}

struct APIBase {
  virtual ~APIBase() = default;
  virtual std::string toString() const = 0;
};
template <typename C> struct API : public APIBase {

  using Action = std::function<void(C &)>;

  template <typename T> void rMember(const Identifier &name, T C::*ptr) {
    members.emplace(name, new Member<C, T>(ptr));
  };

  template <typename T>
  void rGetter(const Identifier &name, function<T(C &)> f) {
    getters.emplace(name, new Getter<C, T>(f));
  };

  template <typename T>
  void rFunction(const Identifier &name, function<T(C &)> f) {
    functions.emplace(name, new Function<C, T>(f));
  };

  template <typename T, typename... Args>
  void rFunction(const Identifier &name, function<T(C &, Args...)> f) {
    functions.emplace(name, new Function<C, T, Args...>(f));
  };

  template <typename T, typename... Args>
  void rFunction(const Identifier &name, T (C::*f)(Args...)) {
    functions.emplace(name, new Function<C, T, Args...>(f));
  };

  // register functions
  void rTrig(const Identifier &name, Action act) {
    rFunction<void>(name, act);
  };

  // do functions
  template <typename T>
  bool set(C &instance, const Identifier &id, const T &v) const {
    MemberBase *resolved = getOrNull(members, id);
    if (auto m = dynamic_cast<Member<C, T> *>(resolved)) {
      m->set(instance, v);
      return true;
    }
    return false;
  }

  template <typename T> Getter<C, T> *getGetter(const Identifier &id) const {
    GetterBase *resolved = getOrNull(getters, id);
    return dynamic_cast<Getter<C, T> *>(resolved);
  }

  template <typename T> Member<C, T> *getMember(const Identifier &id) const {
    MemberBase *resolved = getOrNull(members, id);
    return dynamic_cast<Member<C, T> *>(resolved);
  }

  FunctionOfInstance<C> *getFunction(const Identifier &id) const {
    FunctionBase *resolved = getOrNull(functions, id);
    return dynamic_cast<FunctionOfInstance<C> *>(resolved);
  }

  template <typename T> T get(C &instance, const Identifier &id) const {
    if (auto m = getMember<T>(id))
      return m->get(instance);
    if (auto m = getGetter<T>(id))
      return m->get(instance);
    return {};
  }

  template <typename T, typename... Args>
  T apply(C &instance, const Identifier &id, Args... args) const {
    FunctionBase *fb = getOrNull(functions, id);
    if (auto f = dynamic_cast<Function<C, T, Args...> *>(fb)) {
      return fb(instance, args...);
    }
    return {};
  }
  bool doAction(C &instance, const Identifier &id) const {
    return apply<C, void>(instance, id);
  }

  bool canGet(const Identifier &id) const {
    return getOrNull(members, id) || getOrNull(getters, id);
  }
  bool canSet(const Identifier &id) const { return getOrNull(members, id); }
  bool canCall(const Identifier &id) const { return getOrNull(functions, id); }

  std::string toString() const override {
    std::string res;
    std::vector<std::string> tmp;
    for (const auto &m : members) {
      tmp.push_back(m.first + ":" + m.second->getTypeName());
    };
    if (tmp.size())
      res += std::string(res.size() ? "," : "") + "members:{" +
             StringHelpers::joinIntoString(tmp) + "}";
    tmp.clear();
    for (const auto &m : functions) {
      tmp.push_back(m.first + ":" + m.second->getSignature());
    };
    if (tmp.size())
      res += std::string(res.size() ? "," : "") + "functions:{" +
             StringHelpers::joinIntoString(tmp) + "}";

    tmp.clear();
    for (const auto &m : getters) {
      tmp.push_back(m.first + ":" + m.second->getTypeName());
    };
    if (tmp.size())
      res += std::string(res.size() ? "," : "") + "getters:{" +
             StringHelpers::joinIntoString(tmp) + "}";
    return res;
  }
  std::map<Identifier, GetterBase *> getters;
  std::map<Identifier, MemberBase *> members;
  std::map<Identifier, FunctionBase *> functions;
};
