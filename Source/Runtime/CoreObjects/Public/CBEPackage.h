/*!
 * \file CBEPackage.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObject.h"

#include "CBEPackage.gen.h"

namespace CBE
{
inline constexpr static const TChar *PACKAGE_EXT = TCHAR("berry");

class META_ANNOTATE_API(COREOBJECTS_EXPORT) Package final : public Object
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
    FORCE_INLINE void setPackageRoot(const String &root) { packageRoot = root; }

    String getPackageFilePath() const;

    /* CBE::Object overrides */
    void destroy() override;
    /* Overrides ends */
};
} // namespace CBE
