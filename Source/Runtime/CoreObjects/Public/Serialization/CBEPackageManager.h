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

#include "String/String.h"
#include "String/StringID.h"

#include <set>
#include <unordered_map>

class PackageLoader;

class CBEPackageManager
{
private:
    std::set<String> contentDirs;
    std::unordered_map<StringID, PackageLoader *> packageToLoader;

    std::vector<String> allFoundPackages;
    // Full path of all found objects
    std::vector<String> allFoundObjects;

private:
    void readPackagesIn(const String &contentDir);
    void removePackagesFrom(const String &contentDir);

    void setupPackage(const String &packageFilePath, const String &contentDir);
public:
    CBEPackageManager() = default;
    ~CBEPackageManager();

    void registerContentRoot(const String &contentDir);
    void unregisterContentRoot(const String &contentDir);

    /**
     * CBEPackageManager::findObject - Finds object with path/name if already found
     *
     * Access: public
     *
     * @param const String & objectPath - must be either object's path without package or just object name
     *
     * @return String - Object's Full path if found, Else empty
     */
    String findObject(const String &objectPath) const;

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
};