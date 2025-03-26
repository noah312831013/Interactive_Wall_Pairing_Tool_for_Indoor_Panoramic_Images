#pragma once
#include <string>

class FileDialogs
{
public:
	std::wstring previousPath = std::filesystem::current_path() / L"assets\\test_data\\100_testing_set";
	// These return empty strings if cancelled
	static std::string OpenFile(const char* filter);
	static std::string SaveFile(const char* filter);
	static std::wstring OpenFolder(FileDialogs& panopath);
};