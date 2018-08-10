
// Copyright 2018 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// clang-format off

// This file has been auto-generated by bindings/code_generator_cobalt.py. DO NOT MODIFY!
// Auto-generated from template: bindings/v8c/templates/callback-interface.h.template

#ifndef V8cSingleOperationInterface_h
#define V8cSingleOperationInterface_h

#include "cobalt/script/callback_interface_traits.h"
// Headers for other bindings wrapper classes
#include "cobalt/bindings/testing/single_operation_interface.h"

#include "cobalt/script/v8c/scoped_persistent.h"


namespace cobalt {
namespace bindings {
namespace testing {

class V8cSingleOperationInterface : public script::v8c::ScopedPersistent<v8::Value>,
                          public SingleOperationInterface {
 public:
  typedef SingleOperationInterface BaseType;

  V8cSingleOperationInterface() {}
  V8cSingleOperationInterface(v8::Isolate* isolate, v8::Local<v8::Value> handle)
      : ScopedPersistent(isolate, handle), isolate_(isolate) {}

  base::optional<int32_t > HandleCallback(
      const scoped_refptr<script::Wrappable>& callback_this,
      const scoped_refptr<ArbitraryInterface>& value,
      bool* had_exception) const override;

 private:
  v8::Isolate* isolate_;
};

}  // namespace testing
}  // namespace bindings
}  // namespace cobalt

namespace cobalt {
namespace script {

// Explicit instantiation of CallbackInterfaceTraits struct so we can infer
// the type of the generated class from the type of the callback interface.
template<>
struct CallbackInterfaceTraits<cobalt::bindings::testing::SingleOperationInterface> {
  typedef cobalt::bindings::testing::V8cSingleOperationInterface V8cCallbackInterfaceClass;
};

}  // namespace script
}  // namespace cobalt


#endif  // V8cSingleOperationInterface_h
