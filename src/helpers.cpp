#include "precompiled_header.h"

#include "helpers.h"

int intPow(int value, int power)
{
    int res = 1;
    for (int i = 0; i < power; ++i)
    {
        res *= value;
    }
    return res;
}

std::string wStringToString(const std::wstring& in)
{
    const int requiredSize =
        WideCharToMultiByte(CP_UTF8, 0, in.c_str(), in.size(), NULL, 0, NULL, NULL);
    if (requiredSize == 0)
        return {};

    std::vector<char> buffer(requiredSize);
    const int result = WideCharToMultiByte(CP_UTF8, 0, in.c_str(), in.size(), buffer.data(),
                                           requiredSize, NULL, NULL);
    if (result == 0)
        return {};

    return std::string{buffer.data(), buffer.size()};
}

std::wstring stringToWString(const std::string& in)
{
    const int requiredSize = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), in.size(), NULL, 0);
    if (requiredSize == 0)
        return {};

    std::vector<wchar_t> buffer((size_t)requiredSize);
    const int result =
        MultiByteToWideChar(CP_UTF8, 0, in.c_str(), in.size(), buffer.data(), requiredSize);
    if (result == 0)
        return {};

    return std::wstring{buffer.data(), buffer.size()};
}
