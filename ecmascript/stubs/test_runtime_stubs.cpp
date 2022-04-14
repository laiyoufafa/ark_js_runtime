/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "runtime_stubs.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/frames.h"
#include "ecmascript/global_env.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define DEF_RUNTIME_STUBS(name) \
JSTaggedType RuntimeStubs::name(uintptr_t argGlue, uint32_t argc, uintptr_t argv) \

#define RUNTIME_STUBS_HEADER(name)                        \
    auto thread = JSThread::GlueToJSThread(argGlue);      \
    RUNTIME_TRACE(thread, name);                          \
    [[maybe_unused]] EcmaHandleScope handleScope(thread)  \

#define CONVERT_ARG_TAGGED_TYPE_CHECKED(name, index) \
    ASSERT((index) < argc);                          \
    JSTaggedType name = *(reinterpret_cast<JSTaggedType *>(argv) + (index))

#define CONVERT_ARG_TAGGED_CHECKED(name, index) \
    ASSERT((index) < argc);                     \
    JSTaggedValue name = JSTaggedValue(*(reinterpret_cast<JSTaggedType *>(argv) + (index)))

#define CONVERT_ARG_HANDLE_CHECKED(type, name, index) \
    ASSERT((index) < argc);                           \
    JSHandle<type> name(&(reinterpret_cast<JSTaggedType *>(argv)[index]))

#define CONVERT_ARG_PTR_CHECKED(type, name, index) \
    ASSERT((index) < argc);                        \
    type name = reinterpret_cast<type>(*(reinterpret_cast<JSTaggedType *>(argv) + (index)))

DEF_RUNTIME_STUBS(DefineAotFunc)
{
    RUNTIME_STUBS_HEADER(DefineAotFunc);
    CONVERT_ARG_TAGGED_CHECKED(funcIndex, 0);
    CONVERT_ARG_TAGGED_CHECKED(numArgs, 1);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    auto codeEntry = thread->GetFastStubEntry(funcIndex.GetInt());
    JSMethod *method = ecmaVm->GetMethodForNativeFunction(reinterpret_cast<void *>(codeEntry));
    method->SetAotCodeBit(true);
    method->SetNativeBit(false);
    method->SetNumArgsWithCallField(numArgs.GetInt());
    JSHandle<JSFunction> jsfunc = factory->NewJSFunction(env, method, FunctionKind::NORMAL_FUNCTION);
    jsfunc->SetCodeEntry(reinterpret_cast<uintptr_t>(codeEntry));
    return jsfunc.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(GetPrintFunc)
{
    RUNTIME_STUBS_HEADER(GetPrintFunc);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    auto env = ecmaVm->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject(thread, env->GetGlobalObject());
    JSHandle<JSTaggedValue> printString(thread, factory->NewFromStdString("print").GetTaggedValue());
        
    return JSObject::GetProperty(thread, globalObject, printString).
        GetValue().GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(GetBindFunc)
{
    RUNTIME_STUBS_HEADER(GetBindFunc);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, target, 0);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> bindString(thread, factory->NewFromStdString("bind").GetTaggedValue());

    return JSObject::GetProperty(thread, target, bindString).GetValue().GetTaggedValue().GetRawData();
}
}  // namespace panda::ecmascript
