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
#include "CBEPackage.h"
#include "CoreObjectsModule.h"
#include "CBEObjectHelpers.h"
#include "ObjectPathHelpers.h"
#include "Serialization/PackageLoader.h"
#include "Serialization/PackageSaver.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/PathFunctions.h"

FORCE_INLINE String ObjectPathHelper::packagePathFromFilePath(const String &filePath, const String &contentDir)
{
    String relPath = PathFunctions::toRelativePath(filePath, contentDir);
    // Right now we use relative path as Package path. But in future once plug ins were added
    // Have to allow package path uniqueness per plugin like prefixing plugin name to package path
    return PathFunctions::stripExtension(relPath);
}

namespace CBE
{
Package *Package::createPackage(const String &relativePath, const String &contentDir)
{
    String packagePath = ObjectPathHelper::packagePathFromFilePath(relativePath, contentDir);
    CBE::Package *package = CBE::createOrGet<CBE::Package>(packagePath, nullptr, CBE::EObjectFlagBits::PackageLoadPending);
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

    CBE::Package *package = objectPackageLoader->getPackage();
    debugAssert(package);

    if (BIT_SET(package->getFlags(), EObjectFlagBits::PackageLoadPending))
    {
        bool bPackageLoaded = objectPackageLoader->load();
        if (!bPackageLoaded)
        {
            fatalAssertf(bPackageLoaded, "Loading package %s failed", package->getName());
            return nullptr;
        }
    }

    StringID objId{ objectPath };
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    Object *obj = nullptr;
    if (objectsDb.hasObject(objId))
    {
        obj = objectsDb.getObject(objId);
    }
    debugAssert(obj && BIT_NOT_SET(obj->getFlags(), EObjectFlagBits::PackageLoadPending));
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

    if (!obj || BIT_SET(obj->getFlags(), EObjectFlagBits::PackageLoadPending))
    {
        return load(objectPath);
    }
    return obj;
}

void markDirty(Object *obj)
{
    CBE::Package *package = cast<CBE::Package>(obj->getOuterMost());
    if (package)
    {
        SET_BITS(INTERNAL_ObjectCoreAccessors::getFlags(obj), EObjectFlagBits::PackageDirty);
    }
}

bool save(Object *obj)
{
    CBE::Package *package = cast<CBE::Package>(obj->getOuterMost());
    if (!package)
    {
        LOG_WARN("ObjectHelper", "Object %s cannot be saved due to invalid package", obj->getFullPath());
        return false;
    }

    PackageSaver saver(package);
    if (!saver.savePackage())
    {
        LOG_ERROR("ObjectHelper", "Failed to save package %s", package->getName());
        return false;
    }
    CLEAR_BITS(INTERNAL_ObjectCoreAccessors::getFlags(obj), EObjectFlagBits::PackageDirty);
    return true;
}
} // namespace CBE

void CBEPackageManager::readPackagesIn(const String &contentDir)
{
    std::vector<String> packageFiles = FileSystemFunctions::listFiles(contentDir, true, String("*.") + CBE::PACKAGE_EXT);
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
        std::vector<String> packageFiles = FileSystemFunctions::listFiles(contentDir, true, String("*.") + CBE::PACKAGE_EXT);
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
    CBE::Package *package = CBE::Package::createPackage(PathFunctions::toRelativePath(packageFilePath, contentDir), contentDir);

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
        [&objectPath](const String &otherObjectPath) { return otherObjectPath.find(objectPath) != String::npos; }
    );
    return (itr != allFoundObjects.cend()) ? *itr : TCHAR("");
}