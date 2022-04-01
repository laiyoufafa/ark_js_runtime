/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JSPANDAFILE_DEBUG_INFO_EXTRACTOR_H
#define ECMASCRIPT_JSPANDAFILE_DEBUG_INFO_EXTRACTOR_H

#include <vector>
#include <unordered_map>

#include "ecmascript/common.h"
#include "libpandafile/file.h"

namespace panda::ecmascript {
struct LineTableEntry {
    uint32_t offset;
    size_t line;
};

struct ColumnTableEntry {
    uint32_t offset;
    size_t column;
};

using LineNumberTable = std::vector<LineTableEntry>;
using ColumnNumberTable = std::vector<ColumnTableEntry>;

/*
 * LocalVariableInfo define in frontend, now only use name and regNumber:
 *   std::string name
 *   std::string type
 *   std::string typeSignature
 *   int32_t regNumber
 *   uint32_t startOffset
 *   uint32_t endOffset
 */
using LocalVariableTable = std::unordered_map<std::string, int32_t>;

// public for debugger
class PUBLIC_API DebugInfoExtractor {
public:
    explicit DebugInfoExtractor(const panda_file::File *pf);

    ~DebugInfoExtractor() = default;

    DEFAULT_COPY_SEMANTIC(DebugInfoExtractor);
    DEFAULT_MOVE_SEMANTIC(DebugInfoExtractor);

    const LineNumberTable &GetLineNumberTable(panda_file::File::EntityId methodId) const;

    const ColumnNumberTable &GetColumnNumberTable(panda_file::File::EntityId methodId) const;

    const LocalVariableTable &GetLocalVariableTable(panda_file::File::EntityId methodId) const;

    const char *GetSourceFile(panda_file::File::EntityId methodId) const;

    const char *GetSourceCode(panda_file::File::EntityId methodId) const;

    std::vector<panda_file::File::EntityId> GetMethodIdList() const;

private:
    void Extract(const panda_file::File *pf);

    struct MethodDebugInfo {
        std::string sourceFile;
        std::string sourceCode;
        LineNumberTable lineNumberTable;
        ColumnNumberTable columnNumberTable;
        LocalVariableTable localVariableTable;
    };

    std::unordered_map<uint32_t, MethodDebugInfo> methods_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSPANDAFILE_DEBUG_INFO_EXTRACTOR_H