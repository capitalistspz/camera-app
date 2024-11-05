#include "camera.hpp"
#include "gfx.hpp"
#include <CafeGLSLCompiler.h>
#include <chrono>
#include <format>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/proc.h>

void DeInit();
void Exit();

int main()
{
    namespace chr = std::chrono;

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

    const auto localTimeZone = chr::current_zone();

    while (WHBProcIsRunning())
    {
        auto* buf = camera::GetSurfaceBuffer();
        if (const auto count = VPADRead(VPAD_CHAN_0, &vpadStatus, 1, &vpadReadError); count > 0)
        {
            const auto justChanged = vpadStatus.hold & ~prevVpadStatus.hold;
            if (justChanged & VPAD_BUTTON_A)
            {
                const auto localNow = localTimeZone->to_local(chr::system_clock::now());
                const auto path = std::format("/vol/external01/wiiu/cam/IMG-{0:%F}-{0:%H}-{0:%M}-{0:%S}.nv12",
                                        chr::floor<chr::milliseconds>(localNow));
                camera::SaveNV12(path);
            }
            prevVpadStatus = vpadStatus;
        }
        gfx::SetImage(buf);
        gfx::Draw();
    }

    camera::Exit();
    gfx::Cleanup();
    DeInit();
    return 0;
}

void DeInit()
{
    WHBGfxShutdown();
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
