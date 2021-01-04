#pragma once
#include "TypedArgs.h"
#include "log.h"

#include <experimental/tuple> // for std::apply
#include <functional>
using std::function;
typedef std::string Identifier;

struct MemberBase {
  virtual ~MemberBase() = default;
  virtual std::string getTypeName() const = 0;
  virtual void setFromString(void *owner, const std::string &s) = 0;
  virtual std::string toString(void *owner) const = 0;
  template <typename T> bool is() const;
};

template <typename T> struct MemberBaseT : public MemberBase {
  virtual ~MemberBaseT() = default;
  std::string getTypeName() const override { return TypeOf<T>::name(); }
};

template <typename T> bool MemberBase::is() const {
  return dynamic_cast<const MemberBaseT<T> *>(this);
}

template <typename C, typename T> struct Member : public MemberBaseT<T> {
  typedef T C::*Ptr;
  Ptr ptr;
  Member(Ptr mPtr) : ptr(mPtr) {}
  void set(C &owner, const T &v) { owner.*ptr = v; }
  T get(const C &owner) const { return owner.*ptr; }
  const T &getRef(const C &owner) const { return owner.*ptr; }

  void setFromString(void *owner, const std::string &s) override {
    reinterpret_cast<C *>(owner)->*ptr = StringHelpers::fromString<T>(s);
  }
  std::string toString(void *owner) const override {
    return StringHelpers::toString<T>(reinterpret_cast<C *>(owner)->*ptr);
  }
};

struct GetterBase {
  virtual ~GetterBase() = default;
  virtual std::string getTypeName() const = 0;
};

template <typename C, typename T> struct Getter : public GetterBase {
  using FType = function<T(C &)>;
  Getter(FType _f) : f(_f) {}
  T get(C &owner) { return f(owner); }
  std::string getTypeName() const override { return TypeOf<T>::name(); }

  FType f;
};

struct FunctionBase {
  virtual ~FunctionBase() = default;
  virtual int getNumArgs() const = 0;
  virtual std::vector<std::string> getArgTypes() const = 0;
  virtual std::string getReturnType() const = 0;
  std::string getSignature() {
    return getReturnType() + "(" +
           StringHelpers::joinIntoString(getArgTypes()) + ")";
  }
};

template <typename C> struct FunctionOfInstance : public FunctionBase {
  virtual TypedArgBase::UPtr call(C &owner, const TypedArgList &l) = 0;
};

template <typename C, typename T, typename... Args>
struct Function : public FunctionOfInstance<C> {
  using FType = function<T(C &, Args...)>;

  static constexpr bool ReturnsValue = !std::is_void<T>::value;
  static constexpr bool HasArgs = sizeof...(Args) > 0;

  Function(FType _f) : f(_f) {}

  T apply(C &owner, Args... args) { return f(owner, args...); }

  int getNumArgs() const override { return sizeof...(Args); };
  std::vector<std::string> getArgTypes() const override {
    std::vector<std::string> res;
    getArgAtPos<Args...>(res);
    return res;
  };

  std::string getReturnType() const override { return TypeOf<T>::name(); }

  template <typename A = void>
  void getArgAtPos(std::vector<std::string> &types) const {
    if (!(std::is_void<A>::value)) {
      types.push_back(TypeOf<A>::name());
    } else {
      PRINTLN("!!!! adding void to arg");
    }
  }

  template <typename A, typename... Remaining>
  std::enable_if_t<(sizeof...(Remaining) > 0), void>
  getArgAtPos(std::vector<std::string> &types) const {
    if (!(std::is_void<A>::value)) {
      types.push_back(TypeOf<A>::name());
    } else {
      PRINTLN("!!!! adding void to arg");
    }
    getArgAtPos<Remaining...>(types);
  }

  TypedArgBase::UPtr call(C &owner, const TypedArgList &l) override {
    return callInternal(owner, l);
  }

  // nonvoid ()
  template <bool RV = ReturnsValue, bool hasArg = HasArgs,
            std::enable_if_t<RV> * = nullptr,
            std::enable_if_t<!hasArg> * = nullptr>
  TypedArgBase::UPtr callInternal(C &owner, const TypedArgList &l) {
    TypedArgBase::UPtr res(new TypedArg<T>(this->apply(owner)));
    return res;
  };

  // void()
  template <bool RV = ReturnsValue, bool hasArg = HasArgs,
            std::enable_if_t<!RV> * = nullptr,
            std::enable_if_t<!hasArg> * = nullptr>
  TypedArgBase::UPtr callInternal(C &owner, const TypedArgList &l) {
    this->apply(owner);
    TypedArgBase::UPtr res(new TypedArg<void>());
    return res;
  };

  // void(args...)
  template <bool RV = ReturnsValue, bool hasArg = HasArgs,
            std::enable_if_t<!RV> * = nullptr,
            std::enable_if_t<hasArg> * = nullptr>
  TypedArgBase::UPtr callInternal(C &owner, const TypedArgList &l) {
    bool valid;
    TypedArgBase::UPtr res;
    auto tuple = TupleFiller::fillTuple<Args...>(l, valid);
    if (valid) {
      std::experimental::apply(
          [this, &owner, &res](auto &&...args) {
            this->apply(owner, args...);
            res.reset(new TypedArg<void>());
          },
          tuple);
    }
    return res;
  };

  // nonvoid(args...)
  template <bool RV = ReturnsValue, bool hasArg = HasArgs,
            std::enable_if_t<RV> * = nullptr,
            std::enable_if_t<hasArg> * = nullptr>
  TypedArgBase::UPtr callInternal(C &owner, const TypedArgList &l) {
    bool valid;
    TypedArgBase::UPtr res;
    auto tuple = TupleFiller::fillTuple<Args...>(l, valid);
    if (valid) {
      std::experimental::apply(
          [this, &owner, &res](auto &&...args) {
            res.reset(new TypedArg<T>(this->apply(owner, args...)));
          },
          tuple);
    }
    return res;
  };

  FType f;
};

//////////
// Helpers

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
