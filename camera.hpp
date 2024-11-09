#pragma once
#include <filesystem>

namespace camera
{
bool Initialize();
void Finalize();

bool Open();
void Close();

void* UpdateSurfaceBuffer();
void SaveNV12(const std::filesystem::path& path);
} // namespace camera
