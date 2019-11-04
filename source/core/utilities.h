#pragma once
#include <string>
#include <filesystem>

std::string TimeString(double time);
std::string FpsString(double deltaTime);
bool LoadText(std::filesystem::path filePath, std::string& output);