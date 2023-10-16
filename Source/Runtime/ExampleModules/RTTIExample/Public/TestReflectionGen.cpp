/*!
 * \file TestReflectionGen.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "TestReflectionGen.h"
#include "TestNoGen.h"

std::pair<uint32, TestNS::BerryObject *> BerrySecond::idxToObjext;

uint32 BerrySecond::value;

TestNS::BerryObject *BerrySecond::value1;

const TestNS::BerryObject *BerrySecond::value2;

void BerrySecond::testThisFunction(std::pair<uint32, TestNS::BerryObject *> &, const std::unordered_map<uint64, BerrySecondData *> &, uint32) {}

void BerrySecond::testConstFunction(
    std::vector<std::pair<uint32, TestNS::BerryObject *>> &, const std::unordered_map<uint64, TestNS::BerryObject *> &, uint32
) const
{}

std::set<std::pair<uint32, TestNS::BerryObject *>> BerrySecond::testStaticFunc(
    std::vector<std::pair<uint32, TestNS::BerryObject *>> *&, const std::unordered_map<uint64, TestNS::BerryObject *> &, uint32
)
{
    return {};
}
