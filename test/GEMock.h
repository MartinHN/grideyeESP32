struct GEMock {

  template <typename T> T echo(T f) { return f; }
};
struct MockAPI : public API<GEMock> {

  MockAPI() {
    rFunction<float, float>("echof", &GEMock::echo<float>);
    rFunction<int, int>("echoi", &GEMock::echo<int>);
    rFunction<double, double>("echod", &GEMock::echo<double>);
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
  PRINTLN("had res");
  EXPECT_TRUE(res->template get<T>() == testVal);
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
}
