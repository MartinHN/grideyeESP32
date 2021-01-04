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
};

template <typename C, typename T> struct Member : public MemberBase {
  typedef T C::*Ptr;
  Ptr ptr;
  Member(Ptr mPtr) : ptr(mPtr) {}
  void set(C &owner, const T &v) { owner.*ptr = v; }
  const T &get(const C &owner) const { return owner.*ptr; }

  std::string getTypeName() const override { return TypeOf<T>::name(); }

  //   void setFromString(C &owner, const std::string &s) {
  //     owner.*ptr = StringHelpers::fromString<T>(v);
  //   }
  //   std::string toString(C &owner) const {
  //     return StringHelpers::toString<T>(owner.*ptr);
  //   }
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
    if (!(std::is_void<A>::value))
      types.push_back(TypeOf<T>::name());
  }

  template <typename A, typename... Remaining>
  std::enable_if_t<(sizeof...(Remaining) > 0), void>
  getArgAtPos(std::vector<std::string> &types) const {
    types.push_back(TypeOf<A>::name());
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
