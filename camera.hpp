#pragma once
#include <filesystem>

namespace camera
{
bool Init();
void* GetSurfaceBuffer();
void Exit();
void SaveNV12(const std::filesystem::path& path);
} // namespace camera
