# ModuleReflectTool
CPP Reflection parser that parses the module's header files and creates reflection code

This command line tool takes parameter as either a flag or key value pair. 
* Each key value pair must have key starting with double hyphen(`--key`) and followed by a value seperated by space from key
* Each flag must start with either double hyphen for expanded form(`--flag`) or start with a hyphen for short form(`-f`) followed by no value
----------

## Help `--help` or flag `-h` ##
ModuleReflectTool Copyright (C) Jeslas Pravin, Since 2022
    Parses the headers in provided module and creates reflection files for them.
    It uses clang libraries and mustache style templates to generate reflection data

> `--generatedList` - List of file path that will be consumed by build as generated reflection translation units

> `--generatedDir` - Directory where the generated files will be dropped.
    Generated header for headers under Public folder, will be placed under public folder of this directory and others will be placed under Private

> `--moduleSrcDir` - Directory to search and parse source headers from for this module.

> `--moduleExportMacro` - Name of API export macro for this module.

> `--intermediateDir` - Directory where intermediate files can be dropped/created.
    This must unique per configuration to track last generated timestamps for files etc,.

> `--includeList`, `--I` - File path that contains list of include directories for this module semicolon(;) separated.

> `--compileDefList`, `--D` - File path that contains list of compile definitions for this module semicolon(;) separated.

> `--sampleCode` - Executes sample code instead of actual application

> `--filterDiagnostics` - Filters the diagnostics results and only display what is absolutely necessary

> `--noDiagnostics` - No diagnostics will be displayed

----------

## How to mark any class to reflect ##

In order to mark a class or struct or field or function as reflected you have to annotate it with `META_ANNOTATE(...)`. You can also use `META_ANNOTATE_API(API_EXPORT_MACRO, ...)` if you wanted to export the symbol as exported API and pass in Export macro as first argument. Each class/struct must also contain `GENERATED_CODES()` macro where the generated code will be injected into.

Anything passed in as `__VA_ARGS__ ` to the macro will be annotated and will be parsed during reflection generation. The information can be meta information for the symbol or build flags(used to customize reflection generated for a symbol). Each of these properties and flags has to be separated by semi-colon(`;`). Each meta properties must be constructor with it's corresponding constructor parameters for meta property classes extended from `PropertyMetaDataBase`. And meta flags can be any valid flags. Rest of the flags will be treated as build flags.

Classes must have atleast one constructor and it must be user provided and not compiler generated default(This is to avoid unnecessary data clear when invoking constructor) This means you need to take extra care to set default values for your reflected classes. Constructors cannot be deleted in reflected type, This is due to feature unsupported in libclang without hack or heavy library import and it is still in decision phase.

Example for 
```cpp    
    struct META_ANNOTATE() BerrySecondData
    {
        GENERATED_CODES()

        META_ANNOTATE()
        uint32 value;
    };

    // BaseType is a build flag
    class META_ANNOTATE_API(COREOBJECTS_EXPORT, BaseType) BerryObject
    {
        GENERATED_CODES()

    public:

        META_ANNOTATE()
        BerrySecondData reflectedStruct;

        META_ANNOTATE()
        TestNS::BerryObject::ETestEnumClassScoped options;
    };

```

Currently there are following build flags
> `BaseType` - To specify a class as base for all reflected classes to be generated. Right now this flag appends necessary virtual/override modifiers to functions generated. This class must not have any reflected parent class. Structs are always marked base type as we do not support hierarychy to them. Always mark the root class of reflected class hierarchy as `BaseType` as it also setup other construction related policies(find more about it in advanced section)

> `NoExport` - If passed inside annotation the generated functions that needs export will not be exported, `META_ANNOTATE_API(API_EXPORT_MACRO, ...)` passes this internally to make sure the generated functions are not mark exported when a class is exported already

## Advanced ##
### Constructor ##
The constructor of any type is invoked via a `Construction policy`. This policy gets defined in the Class marked as `BaseType` to `DefaultConstructionPolicy`. 

If user would like to override this policy to custom policy for a class/hierarchy. typedef it to typename `HeapConstructionPolicy` inside the class in public/protected visibility or simple use `OVERRIDE_CONSTRUCTION_POLICY(PolicyTypeName)` macro after GENERATED_CODES() macro.
