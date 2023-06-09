/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_THREAD_H
#define ECMASCRIPT_JS_THREAD_H

#include <atomic>

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/frames.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
class EcmaHandleScope;
class EcmaVM;
class HeapRegionAllocator;
class PropertiesCache;

enum class MarkStatus : uint8_t {
    READY_TO_MARK,
    MARKING,
    MARK_FINISHED,
};

struct BCStubEntries {
    static constexpr size_t EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT =
        kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS;
    // The number of bytecodes.
    static constexpr size_t BC_HANDLER_STUB_ENTRIES_COUNT = 0x100;
    static_assert(EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT <= BC_HANDLER_STUB_ENTRIES_COUNT);
    Address stubEntries_[BC_HANDLER_STUB_ENTRIES_COUNT] = {0};

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * BC_HANDLER_STUB_ENTRIES_COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * BC_HANDLER_STUB_ENTRIES_COUNT;

    void Set(size_t index, Address addr)
    {
        assert(index < BC_HANDLER_STUB_ENTRIES_COUNT);
        stubEntries_[index] = addr;
    }

    void SetUnrealizedBCHandlerStubEntries(Address addr)
    {
        for (size_t i = 0; i < EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT; i++) {
            if (stubEntries_[i] == 0) {
                stubEntries_[i] = addr;
            }
        }
    }
    void SetNonexistentBCHandlerStubEntries(Address addr)
    {
        for (size_t i = EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT; i < BC_HANDLER_STUB_ENTRIES_COUNT; i++) {
            if (stubEntries_[i] == 0) {
                stubEntries_[i] = addr;
            }
        }
    }

    Address* GetAddr()
    {
        return reinterpret_cast<Address*>(stubEntries_);
    }

    static int32_t GetStubEntryOffset(int32_t stubId)
    {
#ifdef PANDA_TARGET_32
        return stubId * sizeof(uint32_t);
#else
        return stubId * sizeof(uint64_t);
#endif
    }

    Address Get(size_t index)
    {
        assert(index < BC_HANDLER_STUB_ENTRIES_COUNT);
        return stubEntries_[index];
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(BCStubEntries), BCStubEntries::SizeArch32, BCStubEntries::SizeArch64);

struct RTStubEntries {
    static constexpr size_t COUNT = kungfu::RuntimeStubCSigns::NUM_OF_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        assert(index < COUNT);
        stubEntries_[index] = addr;
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(RTStubEntries), RTStubEntries::SizeArch32, RTStubEntries::SizeArch64);

struct COStubEntries {
    static constexpr size_t COUNT = kungfu::CommonStubCSigns::NUM_OF_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        assert(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index)
    {
        assert(index < COUNT);
        return stubEntries_[index];
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(COStubEntries), COStubEntries::SizeArch32, COStubEntries::SizeArch64);

class JSThread {
public:
    static constexpr int CONCURRENT_MARKING_BITFIELD_NUM = 2;
    static constexpr uint32_t RESERVE_STACK_SIZE = 128;
    using MarkStatusBits = BitField<MarkStatus, 0, CONCURRENT_MARKING_BITFIELD_NUM>;
    using ThreadId = uint32_t;

    JSThread(EcmaVM *vm);

    PUBLIC_API ~JSThread();

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    static JSThread *Create(EcmaVM *vm);

    int GetNestedLevel() const
    {
        return nestedLevel_;
    }

    void SetNestedLevel(int level)
    {
        nestedLevel_ = level;
    }

    const JSTaggedType *GetCurrentSPFrame() const
    {
        return glueData_.currentFrame_;
    }

    void SetCurrentSPFrame(JSTaggedType *sp)
    {
        glueData_.currentFrame_ = sp;
    }

    const JSTaggedType *GetLastLeaveFrame() const
    {
        return glueData_.leaveFrame_;
    }

    void SetLastLeaveFrame(JSTaggedType *sp)
    {
        glueData_.leaveFrame_ = sp;
    }

    bool DoStackOverflowCheck(const JSTaggedType *sp);

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_;
    }

    HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_;
    }

    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1);

    uintptr_t* PUBLIC_API ExpandHandleStorage();
    void PUBLIC_API ShrinkHandleStorage(int prevIndex);

    JSTaggedType *GetHandleScopeStorageNext() const
    {
        return handleScopeStorageNext_;
    }

    void SetHandleScopeStorageNext(JSTaggedType *value)
    {
        handleScopeStorageNext_ = value;
    }

    JSTaggedType *GetHandleScopeStorageEnd() const
    {
        return handleScopeStorageEnd_;
    }

    void SetHandleScopeStorageEnd(JSTaggedType *value)
    {
        handleScopeStorageEnd_ = value;
    }

    int GetCurrentHandleStorageIndex()
    {
        return currentHandleStorageIndex_;
    }

    void HandleScopeCountAdd()
    {
        handleScopeCount_++;
    }

    void HandleScopeCountDec()
    {
        handleScopeCount_--;
    }

    void SetException(JSTaggedValue exception);

    JSTaggedValue GetException() const
    {
        return glueData_.exception_;
    }

    bool HasPendingException() const
    {
        return !glueData_.exception_.IsHole();
    }

    void ClearException();

    EcmaGlobalStorage *GetEcmaGlobalStorage() const
    {
        return globalStorage_;
    }

    void SetGlobalObject(JSTaggedValue globalObject)
    {
        glueData_.globalObject_ = globalObject;
    }

    const GlobalEnvConstants *GlobalConstants() const
    {
        return &glueData_.globalConst_;
    }

    void NotifyStableArrayElementsGuardians(JSHandle<JSObject> receiver);

    bool IsStableArrayElementsGuardiansInvalid() const
    {
        return !stableArrayElementsGuardians_;
    }

    void ResetGuardians();

    JSTaggedValue GetCurrentLexenv() const;
    void SetCurrentLexenv(JSTaggedValue env);

    void RegisterRTInterface(size_t id, Address addr)
    {
        ASSERT(id < kungfu::RuntimeStubCSigns::NUM_OF_STUBS);
        glueData_.rtStubEntries_.Set(id, addr);
    }

    Address GetFastStubEntry(uint32_t id)
    {
        return glueData_.coStubEntries_.Get(id);
    }

    void SetFastStubEntry(size_t id, Address entry)
    {
        glueData_.coStubEntries_.Set(id, entry);
    }

    Address *GetBytecodeHandler()
    {
        return glueData_.bcStubEntries_.GetAddr();
    }

    void LoadStubsFromFile(std::string &fileName);

    void CheckSwitchDebuggerBCStub();

    ThreadId GetThreadId() const
    {
        return id_.load(std::memory_order_relaxed);
    }

    static ThreadId GetCurrentThreadId()
    {
        return os::thread::GetCurrentThreadId();
    }

    void IterateWeakEcmaGlobalStorage(const WeakRootVisitor &visitor);

    PropertiesCache *GetPropertiesCache() const
    {
        return propertiesCache_;
    }

    void SetMarkStatus(MarkStatus status)
    {
        MarkStatusBits::Set(status, &glueData_.threadStateBitField_);
    }

    bool IsReadyToMark() const
    {
        auto status = MarkStatusBits::Decode(glueData_.threadStateBitField_);
        return status == MarkStatus::READY_TO_MARK;
    }

    bool IsMarking() const
    {
        auto status = MarkStatusBits::Decode(glueData_.threadStateBitField_);
        return status == MarkStatus::MARKING;
    }

    bool IsMarkFinished() const
    {
        auto status = MarkStatusBits::Decode(glueData_.threadStateBitField_);
        return status == MarkStatus::MARK_FINISHED;
    }

    bool CheckSafepoint() const;

    void SetGetStackSignal(bool isParseStack)
    {
        getStackSignal_ = isParseStack;
    }

    bool GetStackSignal() const
    {
        return getStackSignal_;
    }

    void SetGcState(bool gcState)
    {
        gcState_ = gcState;
    }

    bool GetGcState() const
    {
        return gcState_;
    }

    VmThreadControl *GetVmThreadControl() const
    {
        return vmThreadControl_;
    }

    static constexpr size_t GetGlueDataOffset()
    {
        return MEMBER_OFFSET(JSThread, glueData_);
    }

    uintptr_t GetGlueAddr() const
    {
        return reinterpret_cast<uintptr_t>(this) + GetGlueDataOffset();
    }

    static JSThread *GlueToJSThread(uintptr_t glue)
    {
        // very careful to modify here
        return reinterpret_cast<JSThread *>(glue - GetGlueDataOffset());
    }

    struct GlueData : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                 JSTaggedValue,
                                                 JSTaggedValue,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 BCStubEntries,
                                                 RTStubEntries,
                                                 COStubEntries,
                                                 BCStubEntries,
                                                 base::AlignedUint64,
                                                 base::AlignedPointer,
                                                 GlobalEnvConstants> {
        enum class Index : size_t {
            ExceptionIndex = 0,
            GlobalObjIndex,
            CurrentFrameIndex,
            LeaveFrameIndex,
            BCStubEntriesIndex,
            RTStubEntriesIndex,
            COStubEntriesIndex,
            BCDebuggerStubEntriesIndex,
            StateBitFieldIndex,
            FrameBaseIndex,
            GlobalConstIndex,
            NumOfMembers
        };
        static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

        static size_t GetExceptionOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::ExceptionIndex)>(isArch32);
        }

        static size_t GetGlobalObjOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::GlobalObjIndex)>(isArch32);
        }

        static size_t GetGlobalConstOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::GlobalConstIndex)>(isArch32);
        }

        static size_t GetStateBitFieldOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::StateBitFieldIndex)>(isArch32);
        }

        static size_t GetCurrentFrameOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::CurrentFrameIndex)>(isArch32);
        }

        static size_t GetLeaveFrameOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::LeaveFrameIndex)>(isArch32);
        }

        static size_t GetBCStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::BCStubEntriesIndex)>(isArch32);
        }

        static size_t GetRTStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::RTStubEntriesIndex)>(isArch32);
        }

        static size_t GetCOStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::COStubEntriesIndex)>(isArch32);
        }

        static size_t GetBCDebuggerStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::BCDebuggerStubEntriesIndex)>(isArch32);
        }

        static size_t GetFrameBaseOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::FrameBaseIndex)>(isArch32);
        }

        alignas(EAS) JSTaggedValue exception_ {JSTaggedValue::Hole()};
        alignas(EAS) JSTaggedValue globalObject_ {JSTaggedValue::Hole()};
        alignas(EAS) JSTaggedType *currentFrame_ {nullptr};
        alignas(EAS) JSTaggedType *leaveFrame_ {nullptr};
        alignas(EAS) BCStubEntries bcStubEntries_;
        alignas(EAS) RTStubEntries rtStubEntries_;
        alignas(EAS) COStubEntries coStubEntries_;
        alignas(EAS) BCStubEntries bcDebuggerStubEntries_;
        alignas(EAS) volatile uint64_t threadStateBitField_ {0ULL};
        alignas(EAS) JSTaggedType *frameBase_ {nullptr};
        alignas(EAS) GlobalEnvConstants globalConst_;
    };
    static_assert(MEMBER_OFFSET(GlueData, rtStubEntries_) == ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET);
    static_assert(MEMBER_OFFSET(GlueData, currentFrame_) == ASM_GLUE_CURRENT_FRAME_OFFSET);
    static_assert(MEMBER_OFFSET(GlueData, leaveFrame_) == ASM_GLUE_LEAVE_FRAME_OFFSET);
#ifdef ECMASCRIPT_ENABLE_TEST_STUB
    static_assert(MEMBER_OFFSET(GlueData, coStubEntries_) == ASM_GLUE_COMSTUB_ENTRY_OFFSET);
#endif
    STATIC_ASSERT_EQ_ARCH(sizeof(GlueData), GlueData::SizeArch32, GlueData::SizeArch64);

private:
    NO_COPY_SEMANTIC(JSThread);
    NO_MOVE_SEMANTIC(JSThread);

    void DumpStack() DUMP_API_ATTR;

    static constexpr uint32_t MAX_STACK_SIZE = 512 * 1024;
    static const uint32_t NODE_BLOCK_SIZE_LOG2 = 10;
    static const uint32_t NODE_BLOCK_SIZE = 1U << NODE_BLOCK_SIZE_LOG2;
    static constexpr int32_t MIN_HANDLE_STORAGE_SIZE = 2;
    std::atomic<ThreadId> id_;
    EcmaVM *vm_ {nullptr};

    // MM: handles, global-handles, and aot-stubs.
    int nestedLevel_ = 0;
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};
    JSTaggedType *handleScopeStorageNext_ {nullptr};
    JSTaggedType *handleScopeStorageEnd_ {nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> handleStorageNodes_ {};
    int32_t currentHandleStorageIndex_ {-1};
    int32_t handleScopeCount_ {0};
    JSTaggedValue stubCode_ {JSTaggedValue::Hole()};

    PropertiesCache *propertiesCache_ {nullptr};
    EcmaGlobalStorage *globalStorage_ {nullptr};

    // Run-time state
    bool getStackSignal_ {false};
    bool gcState_ {false};
    VmThreadControl *vmThreadControl_ {nullptr};

    bool stableArrayElementsGuardians_ {true};
    GlueData glueData_;

    friend class EcmaHandleScope;
    friend class GlobalHandleCollection;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_THREAD_H
