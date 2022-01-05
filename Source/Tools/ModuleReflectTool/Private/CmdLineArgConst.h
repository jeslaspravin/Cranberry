/*!
 * \file CmdLineArgConst.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "String/String.h"


namespace ReflectToolCmdLineConst
{
    CONST_EXPR StringLiteralStore<"--generatedList"> GENERATED_TU_LIST;
    CONST_EXPR StringLiteralStore<"--generatedDir"> GENERATED_DIR;
    CONST_EXPR StringLiteralStore<"--moduleSrcDir"> MODULE_SRC_DIR;
    CONST_EXPR StringLiteralStore<"--moduleExportMacro"> MODULE_EXP_MACRO;
    CONST_EXPR StringLiteralStore<"--intermediateDir"> INTERMEDIATE_DIR;
    CONST_EXPR StringLiteralStore<"--includeList"> INCLUDE_LIST_FILE;
    CONST_EXPR StringLiteralStore<"--compileDefList"> COMPILE_DEF_LIST_FILE;
    CONST_EXPR StringLiteralStore<"--sampleCode"> SAMPLE_CODE;
    CONST_EXPR StringLiteralStore<"--filterDiagnostics"> FILTER_DIAGNOSTICS;
    CONST_EXPR StringLiteralStore<"--noDiagnostics"> NO_DIAGNOSTICS;
}