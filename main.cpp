#include "camera.hpp"
#include "gfx.hpp"
#include <CafeGLSLCompiler.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/proc.h>

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
    // GLSL_Shutdown();
}

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
        auto buf = camera::GetSurfaceBuffer();
        if (const auto count = VPADRead(VPAD_CHAN_0, &vpadStatus, 1, &vpadReadError); count > 0)
        {
            const auto justChanged = vpadStatus.hold & ~prevVpadStatus.hold;
            if (justChanged & VPAD_BUTTON_A)
            {
                WHBLogPrint("Starting image capture");
                auto* const file = std::fopen("/vol/external01/wiiu/cam/img4.nv12", "wb");
                if (!file)
                {
                    WHBLogPrintf("Failed to open file: %s", std::strerror(errno));
                    continue;
                }
                const auto bytesWritten = std::fwrite(buf, 1, CAMERA_YUV_BUFFER_SIZE, file);
                WHBLogPrintf("Write %d bytes", bytesWritten);
                if (std::fclose(file) != 0)
                {
                    WHBLogPrintf("Failed to close file: %s", std::strerror(errno));
                    continue;
                }

                WHBLogPrint("Successfully saved image");
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
