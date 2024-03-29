/*!
 * \file CBEPackage.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObject.h"

#include "CBEPackage.gen.h"

namespace cbe
{
constexpr inline const TChar *PACKAGE_EXT = TCHAR("bry");

class COREOBJECTS_EXPORT Package final : public Object
{
    GENERATED_CODES()
private:
    String packageName;
    String packagePath;
    // Base content path under which package is supposed to exist
    String packageRoot;

public:
    Package();

    FORCE_INLINE const String &getPackageName() const { return packageName; }
    FORCE_INLINE const String &getPackagePath() const { return packagePath; }
    FORCE_INLINE const String &getPackageRoot() const { return packageRoot; }
    void setPackageRoot(const String &root);

    String getPackageFilePath() const;

    /* cbe::Object overrides */
    void destroy() override;
    /* Overrides ends */

    /**
     * cbe::Package::createPackage
     *
     * Access: public static
     *
     * @param const String & relativePath - Package path relative to contentDir
     * @param const String & contentDir - Root directory under which the package must be saved or loaded from
     * @param bool bForLoading - If package is create to load a package from directory? If dynamically created package then it should be false
     *
     * @return cbe::Package *
     */
    static Package *createPackage(const String &relativePath, const String &contentDir, bool bForLoading);

} META_ANNOTATE(NoExport);
} // namespace cbe
