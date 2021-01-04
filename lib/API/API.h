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

struct QResult {
  QResult() = default;
  QResult(QResult &&o) = default;
  QResult(TypedArgBase *r) : valid(true), res(r){};
  QResult(TypedArgBase::UPtr &&r) : valid(true), res(std::move(r)){};

  explicit QResult(bool hasErr) : valid(hasErr){};
  operator bool() const { return valid; }
  // ~QResult() = default;
  // template <typename T> QResult(T r) : valid(true), res(r) {}

  template <typename T> void set(T r) {
    valid = true;
    res = std::make_unique<TypedArg<T>>(r);
    errMsg.clear();
  }

  // void setResult(const TypedArgBase &r) {
  //   valid = true;
  //   res = r;
  //   errMsg.clear();
  // }

  static QResult err(const std::string &err) { return QResult(err); }
  template <typename T> static QResult ok(const T &r) {
    return QResult(std::make_unique<TypedArg<T>>(r));
  }
  static QResult ok() { return QResult(true); };
  std::string toString() {
    return valid ? (res ? res->toString() : "valid but empty") : errMsg;
  }
  bool valid = false;
  TypedArgBase::UPtr res = {}; // TypedArgBase::none();
  std::string errMsg = {};

private:
  QResult(const std::string &err) : valid(false), errMsg(err){};
};
// static_assert(std::is_aggregate<QResult>::value);
struct APIInstanceBase {
  virtual const APIBase &getAPI() = 0;
  virtual QResult get(const Identifier &n) = 0;
  virtual bool set(const Identifier &n, const TypedArgList &args) = 0;
  virtual QResult call(const Identifier &n, const TypedArgList &args) = 0;
  virtual bool canGet(const Identifier &id) const = 0;
  virtual bool canSet(const Identifier &id) const = 0;
  virtual bool canCall(const Identifier &id) const = 0;
};

template <typename C, typename T>
bool tryGet(C &o, GetterBase *g, QResult &res) {
  if (auto getter = dynamic_cast<Getter<C, T> *>(g)) {
    res.set(getter->get(o));
    return true;
  }
  return false;
}

template <typename C, typename T>
bool trySet(C &o, MemberBase *g, const TypedArgBase *v) {
  if (auto m = dynamic_cast<Member<C, T> *>(g)) {
    m->set(o, v->get<T>());
    return true;
  }
  return false;
}

template <typename C>
QResult tryCall(C &owner, FunctionOfInstance<C> &fun, const TypedArgList &v) {
  if (fun.getNumArgs() == v.size()) {
    // PRINTLN("insideTryCall");
    TypedArgBase::UPtr res = fun.call(owner, v);
    return QResult(std::move(res));
  }
  return QResult::err("wrong num of args");
}

template <typename C> struct APIInstance : public APIInstanceBase {

  APIInstance(C &o, const API<C> &a) : obj(o), api(a) {}
  const APIBase &getAPI() { return api; }
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

  C &obj;
  const API<C> &api;
};
