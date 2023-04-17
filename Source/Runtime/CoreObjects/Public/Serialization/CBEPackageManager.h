/*!
 * \file CBEPackageManager.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObjectTypes.h"

#include <set>
#include <unordered_map>

class PackageLoader;

class CBEPackageManager
{
private:
    std::set<String> contentDirs;
    std::unordered_map<StringID, PackageLoader *> packageToLoader;

    std::vector<String> allFoundPackages;

    struct FoundObjectsInfo
    {
        String fullPath;
        StringID packageName;
        CBEClass objClass;
    };
    // Full path of all found objects
    std::vector<FoundObjectsInfo> allFoundObjects;

public:
    CBEPackageManager() = default;
    ~CBEPackageManager();

    void registerContentRoot(StringView contentDir);
    void unregisterContentRoot(StringView contentDir);
    void onObjectDeleted(cbe::Object *obj);

    /**
     * CBEPackageManager::findObject - Finds object with path/name if already found
     *
     * Access: public
     *
     * @param StringView objectPath - must be either object's path without package or just object name
     * @param CBEClass clazz - Class this object must be. if null will ignore class check and returns first found
     *
     * @return String - Object's Full path if found, Else empty
     */
    String findObject(StringView objectPath, CBEClass clazz) const;

    // Scans all content directory and finds new package if present and loads its meta and package tables
    void refreshPackages();

    FORCE_INLINE PackageLoader *getPackageLoader(StringID packageId) const
    {
        auto itr = packageToLoader.find(packageId);
        if (itr != packageToLoader.cend())
        {
            return itr->second;
        }
        return nullptr;
    }

private:
    void readPackagesIn(StringView contentDir);
    void removePackagesFrom(StringView contentDir);

    void setupPackage(StringView packageFilePath, StringView contentDir);
    // Clears everything related to a package stored in CBEPackageManager and deletes the loader
    void clearPackage(PackageLoader *loader);
};