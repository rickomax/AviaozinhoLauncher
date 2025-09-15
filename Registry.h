#pragma once
#include <windows.h>
#include <string>
#include <iostream>

constexpr LPCWSTR kSubKey = REGISTRYKEY;

bool WriteDwordToRegistry(DWORD data, const char* valueName)
{
    HKEY hKey = nullptr;
    LSTATUS status = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        (LPWSTR)kSubKey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &hKey,
        nullptr);

    if (status != ERROR_SUCCESS)
        return false;

    status = RegSetValueExW(
        hKey,
        (LPWSTR)valueName,
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&data),
        sizeof(DWORD));

    RegCloseKey(hKey);
    return status == ERROR_SUCCESS;
}

bool ReadDwordFromRegistry(DWORD& outData, const char* valueName)
{
    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        (LPWSTR)kSubKey,
        0,
        KEY_QUERY_VALUE,
        &hKey);

    if (status != ERROR_SUCCESS)
        return false;

    DWORD type = 0;
    DWORD data = 0;
    DWORD dataSz = sizeof(DWORD);

    status = RegQueryValueExW(
        hKey,
        (LPWSTR)valueName,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(&data),
        &dataSz);

    RegCloseKey(hKey);

    if (status == ERROR_SUCCESS && type == REG_DWORD && dataSz == sizeof(DWORD))
    {
        outData = data;
        return true;
    }
    return false;
}