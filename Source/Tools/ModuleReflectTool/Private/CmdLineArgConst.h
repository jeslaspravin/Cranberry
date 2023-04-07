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

#include "String/StringLiteral.h"

namespace ReflectToolCmdLineConst
{
CONST_EXPR StringLiteralStore<TCHAR("--generatedList")> GENERATED_TU_LIST;
CONST_EXPR StringLiteralStore<TCHAR("--generatedDir")> GENERATED_DIR;
CONST_EXPR StringLiteralStore<TCHAR("--moduleSrcDir")> MODULE_SRC_DIR;
CONST_EXPR StringLiteralStore<TCHAR("--moduleName")> MODULE_NAME;
CONST_EXPR StringLiteralStore<TCHAR("--moduleExportMacro")> MODULE_EXP_MACRO;
CONST_EXPR StringLiteralStore<TCHAR("--intermediateDir")> INTERMEDIATE_DIR;
CONST_EXPR StringLiteralStore<TCHAR("--includeList")> INCLUDE_LIST_FILE;
CONST_EXPR StringLiteralStore<TCHAR("--compileDefList")> COMPILE_DEF_LIST_FILE;
CONST_EXPR StringLiteralStore<TCHAR("--sampleCode")> SAMPLE_CODE;
CONST_EXPR StringLiteralStore<TCHAR("--filterDiagnostics")> FILTER_DIAGNOSTICS;
CONST_EXPR StringLiteralStore<TCHAR("--noDiagnostics")> NO_DIAGNOSTICS;
CONST_EXPR StringLiteralStore<TCHAR("--logVerbose")> LOG_VERBOSE;
} // namespace ReflectToolCmdLineConst