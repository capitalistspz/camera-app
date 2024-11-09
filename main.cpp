#include "camera.hpp"
#include "graphics.hpp"
#include <CafeGLSLCompiler.h>
#include <chrono>
#include <format>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/proc.h>

void FinalizeLibs();
[[noreturn]] void ExitToMenu();
uint32_t ReopenCamera(void*);

int main()
{
    namespace chr = std::chrono;
    namespace fs = std::filesystem;

    WHBProcInit();
    WHBLogUdpInit();

    WHBGfxInit();
    GLSL_Init();

    if (!camera::Initialize())
    {
        WHBLogPrint("Failed to initialize camera");
        ExitToMenu();
    }

    WHBLogPrint("Initialized camera");
    if (!camera::Open())
    {
        ExitToMenu();
    }
    WHBLogPrint("Opened camera");

    // Camera is automatically closed when the process is moved to background,
    // so re-open camera when moved back to foreground
    ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, ReopenCamera, nullptr, 10);

    if (!graphics::Initialize())
    {
        WHBLogPrint("Failed to initialize graphics");
        camera::Finalize();
        ExitToMenu();
    }
    WHBLogPrint("Initialized graphics");

    VPADStatus vpadStatus;
    VPADStatus prevVpadStatus{};
    VPADReadError vpadReadError;

    const auto localTimeZone = chr::current_zone();

    // wiiu/cam on SD Card
    const auto imageFolder = fs::path("/vol/external01/wiiu/cam");
    if (!fs::exists(imageFolder))
        fs::create_directories(imageFolder);

    while (WHBProcIsRunning())
    {
        auto* camSurfaceBuf = camera::UpdateSurfaceBuffer();
        if (const auto count = VPADRead(VPAD_CHAN_0, &vpadStatus, 1, &vpadReadError); count > 0)
        {
            const auto justChanged = vpadStatus.hold & ~prevVpadStatus.hold;
            if (justChanged & VPAD_BUTTON_A)
            {
                const auto localNow = localTimeZone->to_local(chr::system_clock::now());
                const auto path = imageFolder / std::format("IMG-{0:%F}-{0:%H}-{0:%M}-{0:%S}.nv12",
                                                            chr::floor<chr::milliseconds>(localNow));
                camera::SaveNV12(path);
            }
            prevVpadStatus = vpadStatus;
        }
        graphics::SetImage(camSurfaceBuf);
        graphics::Draw();
    }

    camera::Finalize();
    graphics::Finalize();
    FinalizeLibs();
    return 0;
}

void FinalizeLibs()
{
    WHBGfxShutdown();
    WHBLogUdpDeinit();
    WHBProcShutdown();
}

[[noreturn]] void ExitToMenu()
{
    SYSLaunchMenu();
    while (WHBProcIsRunning())
    {
    }
    FinalizeLibs();
    std::exit(0);
}

uint32_t ReopenCamera(void*)
{
    return !camera::Open();
}
