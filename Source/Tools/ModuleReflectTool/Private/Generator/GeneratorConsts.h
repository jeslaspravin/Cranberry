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

#include "String/TCharString.h"

namespace GeneratorConsts
{
    // Common to both source and header templates
    CONST_EXPR StringLiteralStore<TCHAR("HeaderFileId")> HEADERFILEID_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("TypeName")> TYPENAME_TAG;

    // Header tags
    CONST_EXPR StringLiteralStore<TCHAR("ReflectTypes")> REFLECTTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("ExportSymbolMacro")> EXPORT_SYMBOL_MACRO;
    // ReflectTypes section inner tags
    CONST_EXPR StringLiteralStore<TCHAR("LineNumber")> LINENUMBER_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("NoExport")> NOEXPORT_BRANCH_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("SimpleTypeName")> SIMPLE_TYPENAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("IsClass")> ISCLASS_BRANCH_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("IsBaseType")> ISBASETYPE_BRANCH_TAG;

    // Source tags
    CONST_EXPR StringLiteralStore<TCHAR("HeaderInclude")> INCLUDEHEADER_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("AllRegisterTypes")> ALLREGISTERTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("QualifiedTypes")> QUALIFIEDTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("PairTypes")> PAIRTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("ContainerTypes")> CONTAINERTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("MapTypes")> MAPTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("EnumTypes")> ENUMTYPES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("Classes")> CLASSTYPES_SECTION_TAG;
    // Sections common tags
    CONST_EXPR StringLiteralStore<TCHAR("SanitizedName")> SANITIZEDNAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("TypeMetaFlags")> TYPEMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("TypeMetaData")> TYPEMETADATA_TAG;
    // AllRegisterTypes section tags
    CONST_EXPR StringLiteralStore<TCHAR("PropertyTypeName")> PROPERTYTYPENAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("RegisterFunctionName")> REGISTERFUNCNAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("NoInit")> NOINIT_BRANCH_TAG;
    // EnumTypes section tags
    CONST_EXPR StringLiteralStore<TCHAR("CanUseAsFlags")> CANUSEASFLAGS_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("EnumFields")> ENUMFIELDS_SECTION_TAG;
    // EnumFields section tags
    CONST_EXPR StringLiteralStore<TCHAR("EnumFieldName")> ENUMFIELDNAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("EnumFieldValue")> ENUMFIELDVALUE_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("EnumFieldMetaFlags")> ENUMFIELDMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("EnumFieldMetaData")> ENUMFIELDMETADATA_TAG;
    // Classes section tags
    CONST_EXPR StringLiteralStore<TCHAR("BaseClasses")> BASECLASSES_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("IsAbstract")> ISABSTRACT_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("Ctors")> CONSTRUCTORS_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("MemberFuncs")> MEMBERFUNCS_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("StaticFuncs")> STATICFUNCS_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("MemberFields")> MEMBERFIELDS_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("StaticFields")> STATICFIELDS_SECTION_TAG;
    // Class section tags common
    CONST_EXPR StringLiteralStore<TCHAR("AccessSpecifier")> ACCESSSPECIFIER_TAG;
    // Class section function tags common
    CONST_EXPR StringLiteralStore<TCHAR("ParamsListContext")> PARAMSLISTCONTEXT_SECTION_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("ParamTypeName")> PARAMTYPENAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("ParamName")> PARAMNAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("ParamsList")> PARAMLIST_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("ReturnTypeName")> RETURNTYPENAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("FuncMetaFlags")> FUNCMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("FuncMetaData")> FUNCMETADATA_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("FuncConst")> FUNCCONST_BRANCH_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("FunctionName")> FUNCTIONNAME_TAG;
    // Class section base class section tags
    CONST_EXPR StringLiteralStore<TCHAR("BaseClassTypeName")> BASECLASSTYPENAME_TAG;
    // Constructor section tags
    CONST_EXPR StringLiteralStore<TCHAR("CtorMetaFlags")> CONSTRUCTORMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("CtorMetaData")> CONSTRUCTORMETADATA_TAG;
    // Class section field tags common
    CONST_EXPR StringLiteralStore<TCHAR("FieldMetaFlags")> FIELDMETAFLAGS_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("FieldMetaData")> FIELDMETADATA_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("FieldTypeName")> FIELDTYPENAME_TAG;
    CONST_EXPR StringLiteralStore<TCHAR("FieldName")> FIELDNAME_TAG;

    // Allowed properties types and register function names
    CONST_EXPR StringLiteralStore<TCHAR("BaseProperty")> BASEPROPERTY;
    CONST_EXPR StringLiteralStore<TCHAR("ClassProperty")> CLASSPROPERTY;
    CONST_EXPR StringLiteralStore<TCHAR("EnumProperty")> ENUMPROPERTY;
    CONST_EXPR StringLiteralStore<TCHAR("registerClassFactory")> REGISTERCLASSFACTORY_FUNC;
    CONST_EXPR StringLiteralStore<TCHAR("registerStructFactory")> REGISTERSTRUCTFACTORY_FUNC;
    CONST_EXPR StringLiteralStore<TCHAR("registerEnumFactory")> REGISTERENUMFACTORY_FUNC;
    CONST_EXPR StringLiteralStore<TCHAR("registerTypeFactory")> REGISTERTYPEFACTORY_FUNC;

    // Template names 
    CONST_EXPR StringLiteralStore<TCHAR("ReflectedHeader")> REFLECTHEADER_TEMPLATE;
    CONST_EXPR StringLiteralStore<TCHAR("ReflectedSource")> REFLECTSOURCE_TEMPLATE;

    // Class build flags
    // Marks a class type as reflected BaseClass for further extended reflection
    CONST_EXPR StringLiteralStore<TCHAR("BaseType")> BASETYPE_FLAG;
    CONST_EXPR StringLiteralStore<TCHAR("NoExport")> NOEXPORT_FLAG;
}
