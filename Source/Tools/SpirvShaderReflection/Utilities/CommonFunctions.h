/*!
 * \file CommonFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <string>
#include <vector>

class CommonFunctions
{
private:
    CommonFunctions() = default;
public:
    static bool writeToFile(const std::string& writeFile, const std::vector<unsigned char>& dataToWrite);
    static bool writeToFile(const std::string& writeFile, const std::vector<uint32_t>& dataToWrite);
    static bool readFromFile(const std::string& readFile, std::vector<unsigned char>& dataRead);
};