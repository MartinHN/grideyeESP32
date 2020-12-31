#pragma once
#include "log.h"
#include <string>

// using std::string;
#include <vector>
// using std::vector;
#include <memory>
#include <tuple>
#include <typeindex>
typedef std::type_index TypeID;

namespace StringHelpers {

template <typename T> std::string toString(const T &v) { return "unknown"; };
// template <typename T>
// toString(const T &v) {
//   return std::to_string((T)v);
// };

template <> std::string toString<std::string>(const std::string &v) {
  return std::string(v);
}

template <> std::string toString<float>(const float &v) {
  return std::to_string(v);
}
} // namespace StringHelpers

struct Void {};
struct TypedArgBase {
  typedef std::unique_ptr<TypedArgBase> UPtr;
  static const TypedArgBase &none() {
    static TypedArgBase n;
    return n;
  };
  TypedArgBase() = default;

protected:
  TypedArgBase(const TypeID &t) : typeId(t), typeStr(t.name()) {}

public:
  // TypedArgBase(TypedArgBase &&) = default;
  // TypedArgBase &operator=(const TypedArgBase &) = default;
  virtual ~TypedArgBase() = default;
  const TypeID typeId = typeid(Void);
  const std::string typeStr = "void";
  template <typename T> T get() const;
  template <typename T> bool set(const T &) { return false; }
  virtual bool fromString(const std::string &s) { return false; }
  virtual std::string toString() { return "none"; }
};

template <typename T> struct TypedArg : public TypedArgBase {
  TypedArg(const T &source) : TypedArgBase(typeid(T)), v(source) {}
  const T &get() const { return v; }
  bool set(const T &e) {
    v = e;
    return true;
  }
  std::string toString() { return StringHelpers::toString<T>(v); }

  T v;
};

template <typename T> T TypedArgBase::get() const {
  if (auto d = dynamic_cast<const TypedArg<T> *>(this)) {
    return d->get();
  }
  std::string tname(typeid(T).name());
  PRINT("getting : ");
  PRINTLN(tname.c_str());
  // PRINT(" from : ");
  // try {
  //   PRINTLN(this->typeStr.size() ? this->typeStr.c_str() : "none");
  // } catch (std::exception e) {
  //   PRINT("exception : ");
  //   PRINTLN(e.what());
  // }
  if (std::is_floating_point<T>::value) {
    PRINTLN("try cast float/double");
    if (auto d = dynamic_cast<const TypedArg<int> *>(this)) {
      return T(d->get());
    }
  }
  if (std::is_integral<T>::value) {
    PRINTLN("try cast int");
    if (auto d = dynamic_cast<const TypedArg<float> *>(this)) {
      return T(d->get());
    }
    if (auto d = dynamic_cast<const TypedArg<double> *>(this)) {
      return T(d->get());
    }
  }
  PRINTLN("getting from base");
  return {};
}
namespace TupleHelpers {
// template <typename Tuple,
//           typename Indices =
//               std::make_index_sequence<std::tuple_size<Tuple>::value>>
// struct runtime_get_func_table;

// template <typename Tuple, size_t... Indices>
// struct runtime_get_func_table<Tuple, std::index_sequence<Indices...>> {
//   using return_type = typename std::tuple_element<0, Tuple>::type &;
//   using get_func_ptr = return_type (*)(Tuple &) noexcept;
//   static constexpr get_func_ptr table[std::tuple_size<Tuple>::value] = {
//       &std::get<Indices>...};
// };

// template <typename Tuple, size_t... Indices>
// constexpr typename runtime_get_func_table<
//     Tuple, std::index_sequence<Indices...>>::get_func_ptr
//     runtime_get_func_table<Tuple, std::index_sequence<Indices...>>::table
//         [std::tuple_size<Tuple>::value];

// template <typename Tuple>
// constexpr typename std::tuple_element<
//     0, typename std::remove_reference<Tuple>::type>::type &
// runtime_get(Tuple &&t, size_t index) {
//   using tuple_type = typename std::remove_reference<Tuple>::type;
//   // if (index >= std::tuple_size<tuple_type>::value)
//   //   throw std::runtime_error("Out of range");
//   return runtime_get_func_table<tuple_type>::table[index](t);
// }

////////////

// template <typename... T, std::size_t... I>
// auto subtuple_(const std::tuple<T...> &t, std::index_sequence<I...>) {
//   return std::make_tuple(std::get<I>(t)...);
// }

// template <int Trim, typename... T> auto subtuple(const std::tuple<T...> &t) {
//   return subtuple_(t, std::make_index_sequence<sizeof...(T) - Trim>());
// }

///////////////////

// template <typename F, size_t... Is>
// auto gen_tuple_impl(F func, std::index_sequence<Is...>) {
//   return std::make_tuple(func(Is)...);
// }

// template <size_t N, typename F> auto gen_tuple(F func) {
//   return gen_tuple_impl(func, std::make_index_sequence<N>{});
// }

/////////////////////////////

// template <typename Tuple, unsigned n> struct Arg {
//   template <class X, class... Xs> constexpr auto operator()(X x, Xs... xs)
//   {
//     return Arg<n - 1>{}(xs...);
//   }
// };
// template <> struct Arg<0> {
//   template <class X, class... Xs> constexpr auto operator()(X x, Xs...) {
//     return x;
//   }
// };
// template <unsigned n> constexpr auto arg = Arg<n>{};
// arg<2>(0,1,2,3,4,5) == 2;
} // namespace TupleHelpers

struct TypedArgList {

  typedef std::unique_ptr<TypedArgBase> Ptr;
  typedef std::vector<Ptr> ArgVecType;
  static const TypedArgList &empty() {
    static TypedArgList e;
    return e;
  };
  TypedArgList() = default;

  template <typename... Args> TypedArgList(Args &&...a) { appendOne(a...); }
  template <typename T, typename... Args> void appendOne(T &e, Args &&...a) {
    args.push_back(std::make_unique<TypedArg<T>>(e));
    appendOne(a...);
  }
  template <typename T> void appendOne(T &e) {
    args.push_back(std::make_unique<TypedArg<T>>(e));
  }

  template <typename T>
  TypedArgList(const T &e) : args({std::make_unique<TypedArg<T>>(e)}) {}
  TypedArgList(TypedArgBase *b) { args.push_back(Ptr(b)); }
  TypedArgList(Ptr &&b) { args.push_back(std::move(b)); }
  template <typename T> void set(const T &e) { args = {e}; }
  template <typename T> void set(int i, const T &e) {
    if (i >= args.size()) {
      args.resize(i + 1);
    }
    args[i] = {e};
  }

  typename ArgVecType::iterator begin() { return args.begin(); }
  typename ArgVecType::const_iterator begin() const { return args.begin(); }
  typename ArgVecType::const_iterator cbegin() const { return begin(); }
  typename ArgVecType::iterator end() { return args.end(); }
  typename ArgVecType::const_iterator end() const { return args.end(); }
  typename ArgVecType::const_iterator cend() const { return end(); }

  const TypedArgBase *operator[](int i) const { return args[i].get(); }
  TypedArgBase *operator[](int i) { return args[i].get(); }

  // template <typename... Args> std::tuple<Args...> fillTuple(bool &valid)
  // const {
  //   if (size() != sizeof...(Args)) {
  //     valid = false;
  //     return {};
  //   }
  //   valid = true;
  //   // size_t i = 0;
  //   // for (const auto &v : args) {
  //   //   TupleHelpers::runtime_get(t, i) =
  //   //       (decltype(TupleHelpers::runtime_get(t, i)))v;
  //   //   i++;
  //   // }

  //   // return TupleHelpers::gen_tuple<sizeof...(Args)>([this](size_t i) {
  //   //   return (std::tuple_element<i, std::tuple<Args...>>::type)args[i];
  //   // });
  //   std::tuple<Args...> t;
  //   // fillOneInTuple<std::tuple<Args...>, sizeof...(Args) - 1>(t);
  //   TupleFiller::fillOne<sizeof...(Args) - 1>(t, args);
  //   return t;
  // }

  // template <typename Tuple, int I> void fillOneInTuple(Tuple &tuple) const {
  //   // constexpr int i = std::tuple_size<Tuple>::value -
  //   sizeof...(Remaining); std::get<I>(tuple) =
  //       args[I].get<typename std::tuple_element<I, Tuple>::type>();
  //   if constexpr (I > 0) {
  //     fillOneInTuple<Tuple, I - 1>(tuple);
  //   }
  // }

  void resize(int i) { args.resize(i); }
  int size() const { return args.size(); }
  ArgVecType args;
};

namespace TupleFiller {

template <typename T>
using BaseType = std::remove_cv_t<std::remove_reference_t<T>>;

// TupleFiller(Tuple &t, TypedArgList a) : args(a), tuple(t) {}
template <typename Tuple, int I>
inline typename std::enable_if<(I > 0), void>::type
fillOne(Tuple &tuple, const TypedArgList &args) {
  std::get<I>(tuple) =
      args[I]->get<BaseType<typename std::tuple_element<I, Tuple>::type>>();
  fillOne<Tuple, I - 1>(tuple);
}
template <typename Tuple, int I>
inline typename std::enable_if<(I == 0), void>::type
fillOne(Tuple &tuple, const TypedArgList &args) {
  // constexpr int i = std::tuple_size<Tuple>::value - sizeof...(Remaining);
  std::get<0>(tuple) =
      args[0]->get<BaseType<typename std::tuple_element<0, Tuple>::type>>();
}

template <typename... Args>
std::tuple<Args...> fillTuple(const TypedArgList &args, bool &valid) {
  if (args.size() != sizeof...(Args)) {
    valid = false;
    return {};
  }
  valid = true;
  std::tuple<Args...> t;
  // fillOneInTuple<std::tuple<Args...>, sizeof...(Args) - 1>(t);
  TupleFiller::fillOne<std::tuple<Args...>, sizeof...(Args) - 1>(t, args);
  return t;
}

}; // namespace TupleFiller
// template <typename Tuple>
// void TypedArgList::fillOneInTuple<Tuple, 0>(Tuple &tuple) const {
//   // constexpr int i = std::tuple_size<Tuple>::value - sizeof...(Remaining);
//   std::get<0>(tuple) =
//       args[0].get<typename std::tuple_element<0, Tuple>::type>();
// }
