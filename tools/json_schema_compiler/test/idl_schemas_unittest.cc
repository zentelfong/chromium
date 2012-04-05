// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/values.h"
#include "tools/json_schema_compiler/any.h"
#include "tools/json_schema_compiler/test/idl_basics.h"
#include "tools/json_schema_compiler/test/idl_object_types.h"

#include "testing/gtest/include/gtest/gtest.h"

using test::api::idl_basics::MyType1;
using test::api::idl_object_types::BarType;
using test::api::idl_object_types::FooType;

namespace Function2 = test::api::idl_basics::Function2;
namespace Function3 = test::api::idl_basics::Function3;
namespace Function4 = test::api::idl_basics::Function4;
namespace Function5 = test::api::idl_basics::Function5;
namespace Function6 = test::api::idl_basics::Function6;
namespace ObjectFunction1 = test::api::idl_object_types::ObjectFunction1;

TEST(IdlCompiler, Basics) {
  // Test MyType1.
  MyType1 a;
  a.x = 5;
  a.y = std::string("foo");
  scoped_ptr<DictionaryValue> serialized = a.ToValue();
  MyType1 b;
  EXPECT_TRUE(MyType1::Populate(*serialized.get(), &b));
  EXPECT_EQ(a.x, b.x);
  EXPECT_EQ(a.y, b.y);

  // Test Function2, which takes an integer parameter.
  ListValue list;
  list.Append(Value::CreateIntegerValue(5));
  scoped_ptr<Function2::Params> f2_params = Function2::Params::Create(list);
  EXPECT_EQ(5, f2_params->x);

  // Test Function3, which takes a MyType1 parameter.
  list.Clear();
  DictionaryValue* tmp = new DictionaryValue();
  tmp->SetInteger("x", 17);
  tmp->SetString("y", "hello");
  list.Append(tmp);
  scoped_ptr<Function3::Params> f3_params = Function3::Params::Create(list);
  EXPECT_EQ(17, f3_params->arg.x);
  EXPECT_EQ("hello", f3_params->arg.y);

  // Test functions that take a callback function as a parameter, with varying
  // callback signatures.
  scoped_ptr<Value> f4_result(Function4::Result::Create());
  EXPECT_TRUE(f4_result->IsType(Value::TYPE_NULL));

  scoped_ptr<Value> f5_result(Function5::Result::Create(13));
  EXPECT_TRUE(f5_result->IsType(Value::TYPE_INTEGER));

  scoped_ptr<Value> f6_result(Function6::Result::Create(a));
  MyType1 c;
  EXPECT_TRUE(MyType1::Populate(*f6_result, &c));
  EXPECT_EQ(a.x, c.x);
  EXPECT_EQ(a.y, c.y);
}

TEST(IdlCompiler, ObjectTypes) {
  // Test the FooType type.
  FooType f1;
  f1.x = 3;
  scoped_ptr<DictionaryValue> serialized_foo = f1.ToValue();
  FooType f2;
  EXPECT_TRUE(FooType::Populate(*serialized_foo.get(), &f2));
  EXPECT_EQ(f1.x, f2.x);

  // Test the BarType type.
  BarType b1;
  base::FundamentalValue seven(7);
  b1.x.Init(seven);
  scoped_ptr<DictionaryValue> serialized_bar = b1.ToValue();
  BarType b2;
  EXPECT_TRUE(BarType::Populate(*serialized_bar.get(), &b2));
  int tmp_int = 0;
  EXPECT_TRUE(b2.x.value().GetAsInteger(&tmp_int));
  EXPECT_EQ(7, tmp_int);

  // Test the params to the ObjectFunction1 function.
  scoped_ptr<DictionaryValue> icon_props(new DictionaryValue());
  icon_props->SetString("hello", "world");
  ObjectFunction1::Params::Icon icon;
  EXPECT_TRUE(ObjectFunction1::Params::Icon::Populate(*(icon_props.get()),
                                                      &icon));
  ListValue list;
  list.Append(icon_props.release());
  scoped_ptr<ObjectFunction1::Params> params =
    ObjectFunction1::Params::Create(list);
  ASSERT_TRUE(params.get() != NULL);
  std::string tmp;
  EXPECT_TRUE(params->icon.additional_properties.GetString("hello", &tmp));
  EXPECT_EQ("world", tmp);
}
