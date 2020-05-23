#include "String.h"

String::String(const String& otherString):std::string(otherString)
{
    
}

String::String(const String& otherString, size_type pos, size_type len):std::string(otherString,pos,len)
{

}

String::String(const AChar* s,size_type n):std::string(s,n)
{

}

String::String(size_type n, AChar c):std::string(n,c)
{

}

String::String(const AChar* s):std::string(s)
{

}

String::String(const std::string& otherString):std::string(otherString)
{

}

String::String(std::string&& otherString): std::string(otherString)
{

}

const AChar* String::getChar() const
{
    return c_str();
}

bool String::findAny(size_t& outIndex, String& outFoundString, const std::vector<String>& findStrgs,
    size_t offset /*= 0*/, bool fromEnd /*= false*/) const
{
    size_t foundAt = npos;
    for (const String& strg : findStrgs) {
         size_t foundAtNew = fromEnd ? rfind(strg, length() - offset) : find(strg, offset);
        if (foundAtNew != npos) {
            outIndex = (foundAt == npos || (fromEnd ? foundAt<foundAtNew : foundAt>foundAtNew)) ? foundAtNew : foundAt;
            outFoundString = (foundAt == npos || (fromEnd ? foundAt<foundAtNew : foundAt>foundAtNew))?strg:outFoundString;
            foundAt = outIndex;
        }
    }
    if (foundAt != npos)
    {
        return true;
    }
    outIndex = npos;
    outFoundString = "";
    return false;
}

String String::replaceAll(const String& from, const String& to) const
{
    String newStr = this->getChar();
    size_t replaceAtPos = 0;
    while ((replaceAtPos = newStr.find(from, replaceAtPos)) != npos)
    {
        newStr.replace(replaceAtPos, from.size(), to);
        replaceAtPos += to.size();
    }
    return newStr;
}

void String::replaceAllInline(const String& from, const String& to)
{
    size_t replaceAtPos = 0;
    while ((replaceAtPos = find(from, replaceAtPos)) != npos)
    {
        replace(replaceAtPos, from.size(), to);
        replaceAtPos += to.size();
    }
}

bool String::operator==(const String& rhs) const
{
    return compare(rhs) == 0;
}