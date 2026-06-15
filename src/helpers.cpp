/*
    HDR Display Read GUI
    Copyright (C) 2026  Dmytro Bulatov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    My contacts are on https://boolka.dev/
*/

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
