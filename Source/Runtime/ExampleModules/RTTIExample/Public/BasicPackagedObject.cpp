/*!
 * \file BasicPackagedObject.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "BasicPackagedObject.h"

void BasicPackagedObject::onPostLoad() { LOG("BasicPackageObject", "Loaded BasicPackageObject {}", getObjectData().path); }
void BasicPackagedObject::onConstructed() { LOG("BasicPackageObject", "Constructed BasicPackageObject {}", getObjectData().path); }
void BasicPackagedObject::exampleFunc() const { LOG("BasicPackageObject", "Example interface function"); }

void BasicFieldSerializedObject::onPostLoad()
{
    LOG("BasicFieldSerializedObject", "Loaded BasicFieldSerializedObject {}", getObjectData().path);
}
void BasicFieldSerializedObject::onConstructed()
{
    LOG("BasicPackageObject", "Constructed BasicFieldSerializedObject {}", getObjectData().path);
}
void BasicFieldSerializedObject::exampleFunc() const { LOG("BasicFieldSerializedObject", "Example interface function"); }
