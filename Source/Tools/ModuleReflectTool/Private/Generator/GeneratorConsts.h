/*!
 * \file GeneratorConsts.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

namespace GeneratorConsts
{
    // Common to both source and header templates
    CONST_EXPR StringLiteralStore<"HeaderFileId"> HEADERFILEID_TAG;
    CONST_EXPR StringLiteralStore<"TypeName"> TYPENAME_TAG;

    // Header tags
    CONST_EXPR StringLiteralStore<"ReflectTypes"> REFLECTTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"ExportSymbolMacro"> EXPORT_SYMBOL_MACRO;
    // ReflectTypes section inner tags
    CONST_EXPR StringLiteralStore<"LineNumber"> LINENUMBER_TAG;
    CONST_EXPR StringLiteralStore<"NoExport"> NOEXPORT_BRANCH_TAG;
    CONST_EXPR StringLiteralStore<"IsClass"> ISCLASS_BRANCH_TAG;
    CONST_EXPR StringLiteralStore<"IsBaseType"> ISBASETYPE_BRANCH_TAG;

    // Source tags
    CONST_EXPR StringLiteralStore<"HeaderInclude"> INCLUDEHEADER_TAG;
    CONST_EXPR StringLiteralStore<"AllRegisterTypes"> ALLREGISTERTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"QualifiedTypes"> QUALIFIEDTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"PairTypes"> PAIRTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"ContainerTypes"> CONTAINERTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"MapTypes"> MAPTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"EnumTypes"> ENUMTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"Classes"> CLASSTYPES_SECTION_TAG;
    // Sections common tags
    CONST_EXPR StringLiteralStore<"SanitizedName"> SANITIZEDNAME_TAG;
    CONST_EXPR StringLiteralStore<"TypeMetaFlags"> TYPEMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<"TypeMetaData"> TYPEMETADATA_TAG;
    // AllRegisterTypes section tags
    CONST_EXPR StringLiteralStore<"PropertyTypeName"> PROPERTYTYPENAME_TAG;
    CONST_EXPR StringLiteralStore<"RegisterFunctionName"> REGISTERFUNCNAME_TAG;
    CONST_EXPR StringLiteralStore<"NoInit"> NOINIT_BRANCH_TAG;
    // EnumTypes section tags
    CONST_EXPR StringLiteralStore<"CanUseAsFlags"> CANUSEASFLAGS_TAG;
    CONST_EXPR StringLiteralStore<"EnumFields"> ENUMFIELDS_SECTION_TAG;
    // EnumFields section tags
    CONST_EXPR StringLiteralStore<"EnumFieldName"> ENUMFIELDNAME_TAG;
    CONST_EXPR StringLiteralStore<"EnumFieldValue"> ENUMFIELDVALUE_TAG;
    CONST_EXPR StringLiteralStore<"EnumFieldMetaFlags"> ENUMFIELDMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<"EnumFieldMetaData"> ENUMFIELDMETADATA_TAG;
    // Classes section tags
    CONST_EXPR StringLiteralStore<"BaseClasses"> BASECLASSES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"IsAbstract"> ISABSTRACT_TAG;
    CONST_EXPR StringLiteralStore<"Ctors"> CONSTRUCTORS_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"MemberFuncs"> MEMBERFUNCS_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"StaticFuncs"> STATICFUNCS_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"MemberFields"> MEMBERFIELDS_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"StaticFields"> STATICFIELDS_SECTION_TAG;
    // Class section tags common
    CONST_EXPR StringLiteralStore<"AccessSpecifier"> ACCESSSPECIFIER_TAG;
    // Class section function tags common
    CONST_EXPR StringLiteralStore<"ParamsListContext"> PARAMSLISTCONTEXT_SECTION_TAG;
    CONST_EXPR StringLiteralStore<"ParamTypeName"> PARAMTYPENAME_TAG;
    CONST_EXPR StringLiteralStore<"ParamName"> PARAMNAME_TAG;
    CONST_EXPR StringLiteralStore<"ParamsList"> PARAMLIST_TAG;
    CONST_EXPR StringLiteralStore<"ReturnTypeName"> RETURNTYPENAME_TAG;
    CONST_EXPR StringLiteralStore<"FuncMetaFlags"> FUNCMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<"FuncMetaData"> FUNCMETADATA_TAG;
    CONST_EXPR StringLiteralStore<"FuncConst"> FUNCCONST_BRANCH_TAG;
    CONST_EXPR StringLiteralStore<"FunctionName"> FUNCTIONNAME_TAG;
    // Class section base class section tags
    CONST_EXPR StringLiteralStore<"BaseClassTypeName"> BASECLASSTYPENAME_TAG;
    // Constructor section tags
    CONST_EXPR StringLiteralStore<"CtorMetaFlags"> CONSTRUCTORMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<"CtorMetaData"> CONSTRUCTORMETADATA_TAG;
    // Class section field tags common
    CONST_EXPR StringLiteralStore<"FieldMetaFlags"> FIELDMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<"FieldMetaData"> FIELDMETADATA_TAG;
    CONST_EXPR StringLiteralStore<"FieldTypeName"> FIELDTYPENAME_TAG;
    CONST_EXPR StringLiteralStore<"FieldName"> FIELDNAME_TAG;

    // Allowed properties types and register function names
    CONST_EXPR StringLiteralStore<"BaseProperty"> BASEPROPERTY;
    CONST_EXPR StringLiteralStore<"ClassProperty"> CLASSPROPERTY;
    CONST_EXPR StringLiteralStore<"EnumProperty"> ENUMPROPERTY;
    CONST_EXPR StringLiteralStore<"registerClassFactory"> REGISTERCLASSFACTORY_FUNC;
    CONST_EXPR StringLiteralStore<"registerStructFactory"> REGISTERSTRUCTFACTORY_FUNC;
    CONST_EXPR StringLiteralStore<"registerEnumFactory"> REGISTERENUMFACTORY_FUNC;
    CONST_EXPR StringLiteralStore<"registerTypeFactory"> REGISTERTYPEFACTORY_FUNC;

    // Template names 
    CONST_EXPR StringLiteralStore<"ReflectedHeader"> REFLECTHEADER_TEMPLATE;
    CONST_EXPR StringLiteralStore<"ReflectedSource"> REFLECTSOURCE_TEMPLATE;

    // Class build flags
    // Marks a class type as reflected BaseClass for further extended reflection
    CONST_EXPR StringLiteralStore<"BaseType"> BASETYPE_FLAG;
    CONST_EXPR StringLiteralStore<"NoExport"> NOEXPORT_FLAG;
}
