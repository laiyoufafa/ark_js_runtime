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

#include "gate_accessor.h"

namespace panda::ecmascript::kungfu {
size_t GateAccessor::GetNumIns(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetNumIns();
}

OpCode GateAccessor::GetOpCode(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetOpCode();
}

GateId GateAccessor::GetId(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetId();
}

void GateAccessor::SetOpCode(GateRef gate, OpCode::Op opcode)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    gatePtr->SetOpCode(OpCode(opcode));
}

GateRef GateAccessor::GetValueIn(GateRef gate, size_t idx) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetInValueCount());
    size_t valueIndex = gatePtr->GetStateCount() + gatePtr->GetDependCount();
    return circuit_->GetIn(gate, valueIndex + idx);
}

size_t GateAccessor::GetNumValueIn(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetInValueCount();
}

GateRef GateAccessor::GetIn(GateRef gate, size_t idx) const
{
    return circuit_->GetIn(gate, idx);
}

GateRef GateAccessor::GetState(GateRef gate, size_t idx) const
{
    ASSERT(idx < circuit_->LoadGatePtr(gate)->GetStateCount());
    return circuit_->GetIn(gate, idx);
}

GateRef GateAccessor::GetDep(GateRef gate, size_t idx) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetDependCount());
    size_t dependIndex = gatePtr->GetStateCount();
    return circuit_->GetIn(gate, dependIndex + idx);
}

void GateAccessor::SetDep(GateRef gate, GateRef depGate, size_t idx)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetDependCount());
    size_t dependIndex = gatePtr->GetStateCount();
    gatePtr->ModifyIn(dependIndex + idx, circuit_->LoadGatePtr(depGate));
}

void GateAccessor::ReplaceIn(UsesIterator &useIt, GateRef replaceGate)
{
    Gate *curGatePtr = circuit_->LoadGatePtr(*useIt);
    Gate *replaceGatePtr = circuit_->LoadGatePtr(replaceGate);
    curGatePtr->ModifyIn(useIt.GetIndex(), replaceGatePtr);
    useIt.SetChanged();
}

GateType GateAccessor::GetGateType(GateRef gate)
{
    return circuit_->LoadGatePtr(gate)->GetGateType();
}

void GateAccessor::SetGateType(GateRef gate, GateType gt)
{
    circuit_->LoadGatePtr(gate)->SetGateType(gt);
}

void GateAccessor::DeleteExceptionDep(UsesIterator &useIt)
{
    ASSERT(GetOpCode(*useIt) == OpCode::RETURN || GetOpCode(*useIt) == OpCode::DEPEND_SELECTOR);
    if (GetOpCode(*useIt) == OpCode::RETURN) {
        // 0 : the index of CONSTANT
        circuit_->DeleteGate(GetValueIn(*useIt, 0));
        DeleteGate(useIt);
    } else {
        size_t idx = useIt.GetIndex();
        auto merge = GetState(*useIt, 0);
        circuit_->DecreaseIn(merge, idx - 1);
        auto valueSelector = *(Uses(merge).begin());
        circuit_->DecreaseIn(valueSelector, idx);
        DecreaseIn(useIt);
    }
}

void GateAccessor::DeleteIn(UsesIterator &useIt)
{
    size_t idx = useIt.GetIndex();
    Gate *curGatePtr = circuit_->LoadGatePtr(*useIt);
    curGatePtr->DeleteIn(idx);
    useIt.SetChanged();
}

void GateAccessor::DeleteGate(UsesIterator &useIt)
{
    circuit_->DeleteGate(*useIt);
    useIt.SetChanged();
}

void GateAccessor::DecreaseIn(UsesIterator &useIt)
{
    size_t idx = useIt.GetIndex();
    circuit_->DecreaseIn(*useIt, idx);
    useIt.SetChanged();
}
}