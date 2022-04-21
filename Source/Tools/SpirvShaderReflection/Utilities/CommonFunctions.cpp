/*!
 * \file CommonFunctions.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CommonFunctions.h"

#include <fstream>

bool CommonFunctions::writeToFile(const std::string &writeFile, const std::vector<unsigned char> &dataToWrite)
{
    std::ofstream file = std::ofstream(writeFile, std::ios::binary | std::ios::out);
    if (file.is_open())
    {
        file.write(reinterpret_cast<const char *>(dataToWrite.data()), dataToWrite.size() * sizeof(decltype(dataToWrite[0])));
        file.close();
        return true;
    }
    return false;
}

bool CommonFunctions::writeToFile(const std::string &writeFile, const std::vector<uint32_t> &dataToWrite)
{
    std::ofstream file = std::ofstream(writeFile, std::ios::binary | std::ios::out);
    if (file.is_open())
    {
        file.write(reinterpret_cast<const char *>(dataToWrite.data()), dataToWrite.size() * sizeof(decltype(dataToWrite[0])));
        file.close();
        return true;
    }
    return false;
}

bool CommonFunctions::readFromFile(const std::string &readFile, std::vector<unsigned char> &dataRead)
{
    std::ifstream file = std::ifstream(readFile, std::ios::binary);
    if (file.is_open())
    {
        file.ignore(std::numeric_limits<std::streamsize>::max());
        std::streamsize length = file.gcount();
        file.clear(); //  Since ignore will have set eof.
        file.seekg(0, std::ifstream::beg);

        dataRead.resize(length);
        file.read(reinterpret_cast<char *>(dataRead.data()), dataRead.size());
        file.close();
        return true;
    }
    return false;
}
