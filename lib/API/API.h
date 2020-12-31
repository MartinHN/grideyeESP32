#pragma once
#include "TypedArgs.h"
#include "log.h"
#include <experimental/tuple> // for std::apply
#include <functional>

#include <map>
#include <string>
using std::function;
typedef std::string Identifier;

struct MemberBase {
  virtual ~MemberBase() = default;
};

template <typename C, typename T> struct Member : public MemberBase {
  typedef T C::*Ptr;
  Member(Ptr mPtr) : ptr(mPtr) {}
  void set(C &owner, const T &v) { owner.*ptr = v; }
  const T &get(const C &owner) const { return owner.*ptr; }
  Ptr ptr;
};

struct GetterBase {
  virtual ~GetterBase() = default;
};

template <typename C, typename T> struct Getter : public GetterBase {
  using FType = function<T(C &)>;
  Getter(FType _f) : f(_f) {}
  T get(C &owner) { return f(owner); }
  FType f;
};

struct FunctionBase {
  virtual ~FunctionBase() = default;
  virtual int getNumArgs() const = 0;
};

template <typename C> struct FunctionOfInstance : public FunctionBase {
  virtual TypedArgBase::UPtr call(C &owner, const TypedArgList &l) = 0;
};

template <typename C, typename T, typename... Args>
struct Function : public FunctionOfInstance<C> {
  using FType = function<T(C &, Args...)>;
  Function(FType _f) : f(_f) {}
  T apply(C &owner, Args... args) { return f(owner, args...); }
  int getNumArgs() const override { return sizeof...(Args); };
  TypedArgBase::UPtr call(C &owner, const TypedArgList &l) override {
    bool valid;
    TypedArgBase::UPtr res;
    auto tuple = TupleFiller::fillTuple<Args...>(l, valid);
    PRINTLN("insideCall");
    PRINT("tuple arg : ");
    PRINTLN(std::get<0>(tuple));
    if (valid) {
      std::experimental::apply(
          [this, &owner, &res](auto &&...args) {
            res.reset(new TypedArg<T>(this->apply(owner, args...)));
          },
          tuple);
    }
    return res;
  };

  // memo
  // void logType() { std::cout << __PRETTY_FUNCTION__ << ": " << f <<
  // std::endl; }

  FType f;
};

template <typename C> struct API {

  using Action = std::function<void(C &)>;

  // register functions
  void rTrig(const Identifier &name, Action act) { trig.emplace(name, act); };

  template <typename T> void rMember(const Identifier &name, T C::*ptr) {
    members.emplace(name, new Member<C, T>(ptr));
  };

  template <typename T>
  void rGetter(const Identifier &name, function<T(C &)> f) {
    getters.emplace(name, new Getter<C, T>(f));
  };

  template <typename T, typename... Args>
  void rFunction(const Identifier &name, function<T(C &, Args...)> f) {
    functions.emplace(name, new Function<C, T, Args...>(f));
  };

  template <typename T, typename... Args>
  void rFunction(const Identifier &name, T (C::*f)(Args...)) {
    functions.emplace(name, new Function<C, T, Args...>(f));
  };

  // do functions
  template <typename T>
  bool set(C &instance, const Identifier &id, const T &v) const {
    MemberBase *resolved = members.at(id);
    if (auto m = dynamic_cast<Member<C, T> *>(resolved)) {
      m->set(instance, v);
      return true;
    }
    return false;
  }

  bool doAction(C &instance, const Identifier &id) const {
    const Action *resolved = getOrNull(trig, id);
    if (resolved) {
      resolved(instance);
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
    FunctionBase *fb = functions.at(id);
    if (auto f = dynamic_cast<Function<C, T, Args...> *>(fb)) {
      return fb(instance, args...);
    }
    return {};
  }

  bool canGet(const Identifier &id) const {
    return members.at(id) || getters.at(id);
  }
  bool canSet(const Identifier &id) const { return members.at(id); }
  bool canCall(const Identifier &id) const { return functions.at(id); }

  template <typename T>
  T *getOrNull(const std::map<Identifier, T *> &m, const Identifier &i) const {
    auto it = m.find(i);
    if (it != m.end()) {
      return it->second;
    }
    return {};
  }
  template <typename T>
  T *getOrNull(const std::map<Identifier, T> &m, const Identifier &i) const {
    auto it = m.find(i);
    if (it != m.end()) {
      return &it->second;
    }
    return {};
  }

  std::map<Identifier, Action> trig;
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
    PRINTLN("insideTryCall");
    TypedArgBase::UPtr res = fun.call(owner, v);
    return QResult(std::move(res));
  }
  return QResult::err("wrong num of args");
}

template <typename C> struct APIInstance : public APIInstanceBase {

  APIInstance(C &o, const API<C> &a) : obj(o), api(a) {}
  QResult get(const Identifier &n) override {
    if (auto *getter = api.getters.at(n)) {
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
    if (auto *member = api.members.at(n)) {
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
