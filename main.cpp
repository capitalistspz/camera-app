#include "camera.hpp"
#include "gfx.hpp"
#include <CafeGLSLCompiler.h>
#include <camera/camera.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <format>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/proc.h>

void DeInit();
void Exit();
void SaveNV12(const uint8_t* buf);

int main()
{
    WHBProcInit();
    WHBLogUdpInit();

    WHBGfxInit();
    GLSL_Init();

    if (!camera::Init())
    {
        WHBLogPrint("Failed to initialize/open camera");
        Exit();
        return 0;
    }

    WHBLogPrint("Initialized and opened camera");

    if (!gfx::Init())
    {
        WHBLogPrint("Failed to initialize graphics");
        Exit();
        return 0;
    }
    WHBLogPrint("Initialized graphics");

    VPADStatus vpadStatus;
    VPADStatus prevVpadStatus{};
    VPADReadError vpadReadError;

    while (WHBProcIsRunning())
    {
        auto* buf = camera::GetSurfaceBuffer();
        if (const auto count = VPADRead(VPAD_CHAN_0, &vpadStatus, 1, &vpadReadError); count > 0)
        {
            const auto justChanged = vpadStatus.hold & ~prevVpadStatus.hold;
            if (justChanged & VPAD_BUTTON_A)
            {
                SaveNV12(static_cast<uint8_t*>(buf));
            }
            prevVpadStatus = vpadStatus;
        }
        gfx::SetImage(buf);
        gfx::Draw();
    }

    camera::Exit();
    DeInit();
    return 0;
}

void DeInit()
{
    WHBGfxShutdown();
    WHBLogCafeDeinit();
    WHBLogUdpDeinit();
    WHBProcShutdown();
}

void Exit()
{
    SYSLaunchMenu();
    while (WHBProcIsRunning())
    {
    }
    DeInit();
}
void SaveNV12(const uint8_t* buf)
{
    WHBLogPrint("Saving a photo");
    namespace chr = std::chrono;
    const auto localNow = chr::current_zone()->to_local(chr::system_clock::now());
    auto path = std::format("/vol/external01/wiiu/cam/IMG-{0:%F}-{0:%H}-{0:%M}-{0:%S}.nv12",
                            chr::floor<chr::milliseconds>(localNow));
    auto* const file = std::fopen(path.c_str(), "wb");
    if (!file)
    {
        WHBLogPrintf("Failed to open file: %s", std::strerror(errno));
        return;
    }
    for (auto line = 0u; line < (CAMERA_HEIGHT + (CAMERA_HEIGHT >> 1)); ++line)
    {
        std::fwrite(buf + CAMERA_PITCH * line, 1, CAMERA_WIDTH, file);
    }

    if (std::fclose(file) != 0)
        WHBLogPrintf("Failed to close file: %s", std::strerror(errno));
    else
        WHBLogPrint("Successfully saved photo");
}
