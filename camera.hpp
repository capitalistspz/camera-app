#pragma once
#include <filesystem>

namespace camera
{
bool Initialize();
void* UpdateSurfaceBuffer();
void Finalize();
void SaveNV12(const std::filesystem::path& path);
} // namespace camera
