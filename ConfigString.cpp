#include <Windows.h>  // GetPrivateProfileString
#include <array>     // array
#include <string>        // string

std::string GetConfigString(const std::string& filePath, const char* pSectionName, const char* pKeyName)
{
	if (filePath.empty()) {
		return "";
	}
	std::array<char, MAX_PATH> buf = {};
	GetPrivateProfileStringA(pSectionName, pKeyName, "", &buf.front(), static_cast<DWORD>(buf.size()), filePath.c_str());
	return &buf.front();
}
