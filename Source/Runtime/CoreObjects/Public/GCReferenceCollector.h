/*!
 * \file GCReferenceCollector.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <vector>

namespace cbe
{
class Object;
}

class IReferenceCollector
{
public:
    /**
     * IReferenceCollector::clearReferences - Must clear holding references to passed in objects as they
     * will be deleted
     *
     * Access: virtual public
     *
     * @param const std::vector<cbe::Object * > & deletedObjects
     *
     * @return void
     */
    virtual void clearReferences(const std::vector<cbe::Object *> &deletedObjects) = 0;
    virtual void collectReferences(std::vector<cbe::Object *> &outObjects) const = 0;
};