/*!
 * \file BasicPackagedObject.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "BasicPackagedObject.h"

void BasicPackagedObject::onPostLoad() { LOG("BasicPackageObject", "Loaded BasicPackageObject %s", getFullPath()); }
void BasicPackagedObject::onConstructed() { LOG("BasicPackageObject", "Constructed BasicPackageObject %s", getFullPath()); }
void BasicPackagedObject::exampleFunc() const { LOG("BasicPackageObject", "Example interface function"); }

void BasicFieldSerializedObject::onPostLoad() { LOG("BasicFieldSerializedObject", "Loaded BasicFieldSerializedObject %s", getFullPath()); }
void BasicFieldSerializedObject::onConstructed() { LOG("BasicPackageObject", "Constructed BasicFieldSerializedObject %s", getFullPath()); }
void BasicFieldSerializedObject::exampleFunc() const { LOG("BasicFieldSerializedObject", "Example interface function"); }
