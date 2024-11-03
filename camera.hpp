#pragma once
#include <camera/camera.h>

namespace camera
{
bool Init();
void* GetSurfaceBuffer();
void Exit();
} // namespace camera
