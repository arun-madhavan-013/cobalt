/*
 * Copyright 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// clang-format off

// This file has been auto-generated by bindings/code_generator_cobalt.py. DO NOT MODIFY!
// Auto-generated from template: bindings/templates/dictionary.h.template

#ifndef DerivedDictionary_h
#define DerivedDictionary_h

#include <string>

#include "base/optional.h"
#include "cobalt/script/script_value.h"
#include "cobalt/script/sequence.h"
#include "cobalt/script/value_handle.h"
#include "cobalt/bindings/testing/test_dictionary.h"

using cobalt::bindings::testing::TestDictionary;

namespace cobalt {
namespace bindings {
namespace testing {

class DerivedDictionary : public cobalt::bindings::testing::TestDictionary {
 public:
  DerivedDictionary() {
    additional_member_ = false;
  }

  DerivedDictionary(const DerivedDictionary& other)
    : cobalt::bindings::testing::TestDictionary(other) {
    additional_member_ = other.additional_member_;
  }

  DerivedDictionary& operator=(const DerivedDictionary& other) {
    cobalt::bindings::testing::TestDictionary::operator=(other);
    additional_member_ = other.additional_member_;
    return *this;
  }

  bool has_additional_member() const {
    return true;
  }
  bool additional_member() const {
    return additional_member_;
  }
  void set_additional_member(bool value) {
    additional_member_ = value;
  }

 private:
  bool additional_member_;
};

// This ostream override is necessary for MOCK_METHODs commonly used
// in idl test code
inline std::ostream& operator<<(
    std::ostream& stream, const cobalt::bindings::testing::DerivedDictionary& in) {
  UNREFERENCED_PARAMETER(in);
  stream << "[DerivedDictionary]";
  return stream;
}

}  // namespace cobalt
}  // namespace bindings
}  // namespace testing

#endif  // DerivedDictionary_h
