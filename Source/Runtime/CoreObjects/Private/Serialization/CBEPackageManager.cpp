/*!
 * \file CBEPackageManager.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/CBEPackageManager.h"
#include "String/StringRegex.h"
#include "CBEPackage.h"
#include "CoreObjectsModule.h"
#include "CBEObjectHelpers.h"
#include "ObjectPathHelpers.h"
#include "Serialization/PackageLoader.h"
#include "Serialization/PackageSaver.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/PathFunctions.h"

bool ObjectPathHelper::isValidPackageName(const String &packageName)
{
    // Must start with non / valid symbol and followed by any valid symbols
    static const StringRegex matchPattern(TCHAR("^[a-zA-Z0-9_]{1}[a-zA-Z0-9_/]*"), std::regex_constants::ECMAScript);
    return std::regex_match(packageName, matchPattern);
}

String ObjectPathHelper::getValidPackageName(const String &packageName)
{
    String output;
    output.resize(packageName.length());

    // Remove all invalid characters, or all / at the start
    static const StringRegex matchPattern(TCHAR("^[/]*|[^a-zA-Z0-9_/]{1}"), std::regex_constants::ECMAScript);
    auto newEndItr = std::regex_replace(output.begin(), packageName.cbegin(), packageName.cend(), matchPattern, TCHAR(""));
    output.erase(newEndItr, output.end());

    // There is possibility that all invalid has been removed but slashes only exists
    while (output.startsWith(ObjectObjectSeparator))
    {
        output.eraseL(1);
    }

    if (output.empty())
    {
        output = TCHAR("InvalidName");
    }
    return output;
}

FORCE_INLINE String ObjectPathHelper::packagePathFromFilePath(const String &filePath, const String &contentDir)
{
    String relPath = PathFunctions::toRelativePath(filePath, contentDir);
    // Right now we use relative path as Package path. But in future once plug ins were added
    // Have to allow package path uniqueness per plugin like prefixing plugin name to package path
    relPath = PathFunctions::asGenericPath(PathFunctions::stripExtension(relPath));
    while (relPath.startsWith(ObjectObjectSeparator))
    {
        relPath.eraseL(1);
    }
    return relPath;
}

namespace cbe
{
cbe::Package *Package::createPackage(const String &relativePath, const String &contentDir, bool bForLoading)
{
    String packagePath = ObjectPathHelper::packagePathFromFilePath(relativePath, contentDir);
    Package *package = createOrGet<Package>(packagePath, nullptr, bForLoading ? EObjectFlagBits::ObjFlag_PackageLoadPending : 0);
    package->setPackageRoot(contentDir);
    return package;
}

Object *load(String objectPath)
{
    CBEPackageManager &packageManager = CoreObjectsModule::packageManager();

    String packagePath = ObjectPathHelper::getPackagePath(objectPath.getChar());
    // If no package path, find a package that has this object name or path
    if (packagePath.empty())
    {
        String objPath = packageManager.findObject(objectPath);
        if (objPath.empty())
        {
            packageManager.refreshPackages();
            objPath = packageManager.findObject(objectPath);
        }
        if (objPath.empty())
        {
            LOG_ERROR("ObjectHelper", "Object %s is not found in any packages!", objectPath);
            return nullptr;
        }

        packagePath = ObjectPathHelper::getPackagePath(objPath.getChar());
        objectPath = objPath;
    }

    PackageLoader *objectPackageLoader = packageManager.getPackageLoader(packagePath.getChar());
    if (!objectPackageLoader)
    {
        LOG_WARN("ObjectHelper", "ObjectLoader for object %s is not found", objectPath);
        packageManager.refreshPackages();
        objectPackageLoader = packageManager.getPackageLoader(packagePath.getChar());
        if (!objectPackageLoader)
        {
            LOG_ERROR("ObjectHelper", "Object %s is not found in any packages!", objectPath);
            return nullptr;
        }
    }

    cbe::Package *package = objectPackageLoader->getPackage();
    debugAssert(package);

    if (BIT_SET(package->getFlags(), EObjectFlagBits::ObjFlag_PackageLoadPending))
    {
        EPackageLoadSaveResult loadResult = objectPackageLoader->load();
        if (CBEPACKAGE_SAVELOAD_ERROR(loadResult))
        {
            fatalAssertf(CBEPACKAGE_SAVELOAD_SUCCESS(loadResult), "Loading package %s failed", package->getName());
            return nullptr;
        }
        else if (!CBEPACKAGE_SAVELOAD_SUCCESS(loadResult))
        {
            LOG_WARN("ObjectHelper", "Loaded package %s(For object %s) with few minor errors", packagePath, objectPath);
        }
    }

    StringID objId{ objectPath };
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    Object *obj = nullptr;
    if (objectsDb.hasObject(objId))
    {
        obj = objectsDb.getObject(objId);
    }
    debugAssert(obj && BIT_NOT_SET(obj->getFlags(), EObjectFlagBits::ObjFlag_PackageLoadPending));
    return obj;
}

Object *getOrLoad(String objectPath)
{
    String packagePath = ObjectPathHelper::getPackagePath(objectPath.getChar());
    // If no package path, find a package that has this object name or path
    if (packagePath.empty())
    {
        CBEPackageManager &packageManager = CoreObjectsModule::packageManager();
        String objPath = packageManager.findObject(objectPath);
        if (objPath.empty())
        {
            packageManager.refreshPackages();
            objPath = packageManager.findObject(objectPath);
        }
        if (objPath.empty())
        {
            LOG_ERROR("ObjectHelper", "Object %s is not found in any packages!", objectPath);
            return nullptr;
        }
        objectPath = objPath;
    }

    StringID objId{ objectPath };
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    Object *obj = nullptr;
    if (objectsDb.hasObject(objId))
    {
        obj = objectsDb.getObject(objId);
    }

    if (!obj || BIT_SET(obj->getFlags(), EObjectFlagBits::ObjFlag_PackageLoadPending))
    {
        return load(objectPath);
    }
    return obj;
}

void markDirty(Object *obj)
{
    cbe::Package *package = cast<cbe::Package>(obj->getOuterMost());
    if (package)
    {
        SET_BITS(INTERNAL_ObjectCoreAccessors::getFlags(obj), EObjectFlagBits::ObjFlag_PackageDirty);
    }
}

bool save(Object *obj)
{
    cbe::Package *package = cast<cbe::Package>(obj);
    if (!package)
    {
        package = cast<cbe::Package>(obj->getOuterMost());
    }
    if (!package)
    {
        LOG_WARN("ObjectHelper", "Object %s cannot be saved due to invalid package", obj->getFullPath());
        return false;
    }

    PackageSaver saver(package);
    EPackageLoadSaveResult saveResult = saver.savePackage();
    if (CBEPACKAGE_SAVELOAD_ERROR(saveResult))
    {
        LOG_ERROR("ObjectHelper", "Failed to save package %s", package->getName());
        return false;
    }
    else if (!CBEPACKAGE_SAVELOAD_SUCCESS(saveResult))
    {
        LOG_WARN("ObjectHelper", "Saved package %s with minor warnings", package->getName());
    }
    CLEAR_BITS(INTERNAL_ObjectCoreAccessors::getFlags(obj), EObjectFlagBits::ObjFlag_PackageDirty);
    return true;
}
} // namespace cbe

void CBEPackageManager::readPackagesIn(const String &contentDir)
{
    std::vector<String> packageFiles = FileSystemFunctions::listFiles(contentDir, true, String("*.") + cbe::PACKAGE_EXT);
    for (const String &packageFile : packageFiles)
    {
        setupPackage(packageFile, contentDir);
    }
}

void CBEPackageManager::removePackagesFrom(const String &contentDir)
{
    for (auto itr = packageToLoader.begin(); itr != packageToLoader.end();)
    {
        if (itr->second->getPackage()->getPackageRoot() == contentDir)
        {
            std::erase(allFoundPackages, itr->second->getPackage()->getName());
            for (const PackageContainedData &containedData : itr->second->getContainedObjects())
            {
                if (containedData.object)
                {
                    std::erase(allFoundObjects, containedData.object->getFullPath());
                }
                else
                {
                    std::erase(
                        allFoundObjects,
                        itr->second->getPackage()->getFullPath() + ObjectPathHelper::RootObjectSeparator + containedData.objectPath
                    );
                }
            }
            itr->second->getPackage()->beginDestroy();
            delete itr->second;
            itr = packageToLoader.erase(itr);
        }
        else
        {
            ++itr;
        }
    }
}

void CBEPackageManager::refreshPackages()
{
    for (const String &contentDir : contentDirs)
    {
        std::vector<String> packageFiles = FileSystemFunctions::listFiles(contentDir, true, String("*.") + cbe::PACKAGE_EXT);
        for (const String &packageFilePath : packageFiles)
        {
            String packagePath = ObjectPathHelper::packagePathFromFilePath(packageFilePath, contentDir);
            if (!packageToLoader.contains(packagePath.getChar()))
            {
                setupPackage(packageFilePath, contentDir);
            }
        }
    }
}

void CBEPackageManager::setupPackage(const String &packageFilePath, const String &contentDir)
{
    String packagePath = ObjectPathHelper::packagePathFromFilePath(packageFilePath, contentDir);
    cbe::Package *package = cbe::Package::createPackage(PathFunctions::toRelativePath(packageFilePath, contentDir), contentDir, true);

    PackageLoader *loader = new PackageLoader(package, packageFilePath);
    loader->prepareLoader();

    packageToLoader[packagePath.getChar()] = loader;
    allFoundPackages.emplace_back(packagePath);
    // Add all objects
    for (const PackageContainedData &containedData : loader->getContainedObjects())
    {
        allFoundObjects.emplace_back(packagePath + ObjectPathHelper::RootObjectSeparator + containedData.objectPath);
    }
}

CBEPackageManager::~CBEPackageManager()
{
    for (const String &contentDir : contentDirs)
    {
        removePackagesFrom(contentDir);
    }
}

void CBEPackageManager::registerContentRoot(const String &contentDir)
{
    contentDirs.insert(contentDir);
    readPackagesIn(contentDir);
}

void CBEPackageManager::unregisterContentRoot(const String &contentDir)
{
    contentDirs.erase(contentDir);
    removePackagesFrom(contentDir);
}

String CBEPackageManager::findObject(const String &objectPath) const
{
    auto itr = std::find_if(
        allFoundObjects.cbegin(), allFoundObjects.cend(),
        [&objectPath](const String &otherObjectPath)
        {
            return otherObjectPath.find(objectPath) != String::npos;
        }
    );
    return (itr != allFoundObjects.cend()) ? *itr : TCHAR("");
}