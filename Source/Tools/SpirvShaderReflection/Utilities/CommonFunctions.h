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