struct GEMock {
  template <typename T> T echo(T f) { return f; }
  void echovoid() {}
  template <typename... Args> void trigWithParams(Args... args){};
};

struct MockAPI : public API<GEMock> {

  MockAPI() {
    rFunction<float, float>("echof", &GEMock::echo<float>);
    rFunction<int, int>("echoi", &GEMock::echo<int>);
    rFunction<double, double>("echod", &GEMock::echo<double>);
    // rFunction<void, void>("echovoid", &GEMock::echovoid);
    // rFunction<void>("echovoidNoArg", &GEMock::echovoid);
    rFunction<void, int>("trigWithParami", &GEMock::trigWithParams<int>);
    rFunction<void, int, int>("trigWithParamii",
                              &GEMock::trigWithParams<int, int>);
    // rFunction<void, int, float>("trigWithParamif",
    //                             &GEMock::trigWithParams<int, float>);
    rTrig("echovoidNoArg", &GEMock::echovoid);
  }
};

/////////////////
// TESTS
#include "gtest/gtest.h"
MockAPI api;
GEMock inst;

template <typename T> void testType(const std::string &fname, double val) {
  FunctionOfInstance<GEMock> *f = api.getFunction(fname);
  ASSERT_TRUE(f != nullptr);
  T testVal(val);
  auto res = f->call(inst, {testVal});
  ASSERT_TRUE(res != nullptr);
  EXPECT_TRUE(res->template is<T>());
  EXPECT_TRUE(res->template get<T>() == testVal);
}

template <typename T>
void testTrigWithParam(const std::string &fname, double val) {
  FunctionOfInstance<GEMock> *f = api.getFunction(fname);
  ASSERT_TRUE(f != nullptr);
  T testVal(val);
  auto res = f->call(inst, {testVal});
  ASSERT_TRUE(res != nullptr);
  EXPECT_TRUE(res->template is<void>());
}

template <typename T, typename T2>
void testTrigWithParam(const std::string &fname, double val, double val2) {
  FunctionOfInstance<GEMock> *f = api.getFunction(fname);
  ASSERT_TRUE(f != nullptr);
  T testVal(val);
  T2 testVal2(val2);
  auto res = f->call(inst, {testVal, testVal2});
  ASSERT_TRUE(res != nullptr);
  EXPECT_TRUE(res->template is<void>());
  auto types = f->getArgTypes();
  ASSERT_TRUE(types.size() == 2);
  EXPECT_TRUE(types[0] == TypeOf<T>::name());
  EXPECT_TRUE(types[1] == TypeOf<T2>::name());
}

void testTrig(const std::string &fname) {
  FunctionOfInstance<GEMock> *f = api.getFunction(fname);
  ASSERT_TRUE(f != nullptr);
  auto res = f->call(inst, {});
  ASSERT_TRUE(res != nullptr);
  EXPECT_TRUE(res->is<void>());
}

void testEchoAllTypes(double val) {
  testType<float>("echof", val);
  testType<int>("echoi", val);
  testType<double>("echod", val);
}

TEST(APITest, canFetch) {
  ASSERT_TRUE(api.getFunction("echof") != nullptr);
  ASSERT_TRUE(api.getFunction("echofoo") == nullptr);
}

TEST(APITest, canCall) {

  testEchoAllTypes(0);
  testEchoAllTypes(1);
  testTrig("echovoidNoArg");
  testTrigWithParam<int>("trigWithParami", 1);
  // testTrigWithParam<int, float>("trigWithParamif", 1, 1.f);
  testTrigWithParam<int, int>("trigWithParamii", 1, 1.f);
}
