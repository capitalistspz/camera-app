#include "camera.hpp"
#include <camera/camera.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <whb/log.h>

namespace camera
{
void* Allocate(uint32_t alignment, uint32_t size)
{
    void* out = std::aligned_alloc(alignment, size);
    while (CAMCheckMemSegmentation(out, size) != CAMERA_ERROR_OK)
    {
        std::free(out);
        out = std::aligned_alloc(alignment, size);
    }
    return out;
}

CAMHandle s_handle = -1;
CAMSurface s_surfaces[2];
unsigned s_decodeSurfaceIndex = 0;
bool s_submitNewSurface = true;
void* s_workMemBuf = nullptr;

void EventHandler(CAMEventData* evtData)
{
    if (evtData->eventType == CAMERA_DECODE_DONE)
    {
        s_submitNewSurface = true;
        s_decodeSurfaceIndex = !s_decodeSurfaceIndex;
    }
}

bool Initialize()
{
    CAMStreamInfo streamInfo{.type = CAMERA_STREAM_TYPE_1, .height = CAMERA_HEIGHT, .width = CAMERA_WIDTH};
    const auto memReq = CAMGetMemReq(&streamInfo);
    if (memReq <= 0)
    {
        WHBLogPrintf("Failed to get memory requirement: %x", memReq);
        return false;
    }
    s_workMemBuf = Allocate(0x100, memReq);

    CAMWorkMem workMem{.size = static_cast<uint32_t>(memReq), .pMem = s_workMemBuf};
    CAMMode mode{.forceDrc = false, .fps = CAMERA_FPS_30};
    CAMSetupInfo setupInfo{.streamInfo = streamInfo,
                           .workMem = workMem,
                           .eventHandler = EventHandler,
                           .mode = mode,
                           .threadAffinity = OS_THREAD_ATTRIB_AFFINITY_CPU1};
    CAMError camError;
    s_handle = CAMInit(0, &setupInfo, &camError);
    if (s_handle < 0)
    {
        WHBLogPrintf("Failed to initialized camera: %d", camError);
        return false;
    }
    if (const auto error = CAMOpen(s_handle))
    {
        WHBLogPrintf("Failed to open camera: %d", error);
        return false;
    }
    s_surfaces[0].surfaceSize = CAMERA_YUV_BUFFER_SIZE;
    s_surfaces[0].alignment = CAMERA_YUV_BUFFER_ALIGNMENT;
    s_surfaces[0].surfaceBuffer = Allocate(CAMERA_YUV_BUFFER_ALIGNMENT, CAMERA_YUV_BUFFER_SIZE);

    s_surfaces[1].surfaceSize = CAMERA_YUV_BUFFER_SIZE;
    s_surfaces[1].alignment = CAMERA_YUV_BUFFER_ALIGNMENT;
    s_surfaces[1].surfaceBuffer = Allocate(CAMERA_YUV_BUFFER_ALIGNMENT, CAMERA_YUV_BUFFER_SIZE);
    return true;
}

void* UpdateSurfaceBuffer()
{
    if (s_submitNewSurface)
    {
        if (CAMSubmitTargetSurface(s_handle, &s_surfaces[s_decodeSurfaceIndex]) == CAMERA_ERROR_OK)
        {
            s_submitNewSurface = false;
        }
    }
    return s_surfaces[!s_decodeSurfaceIndex].surfaceBuffer;
}
void Finalize()
{
    CAMExit(s_handle);
    std::free(s_workMemBuf);
    std::free(s_surfaces[0].surfaceBuffer);
    std::free(s_surfaces[1].surfaceBuffer);
}

void SaveNV12(const std::filesystem::path& path)
{
    WHBLogPrintf("Saving a photo to %s", path.c_str());

    auto* const file = std::fopen(path.c_str(), "wb");
    if (!file)
    {
        WHBLogPrintf("Failed to open file: %s", std::strerror(errno));
        return;
    }

    auto* const buf = static_cast<const uint8_t*>(s_surfaces[!s_decodeSurfaceIndex].surfaceBuffer);
    for (auto line = 0u; line < (CAMERA_HEIGHT + (CAMERA_HEIGHT >> 1)); ++line)
    {
        std::fwrite(buf + CAMERA_PITCH * line, 1, CAMERA_WIDTH, file);
    }

    if (std::fclose(file) != 0)
        WHBLogPrintf("Failed to close file: %s", std::strerror(errno));
    else
        WHBLogPrint("Successfully saved photo");
}

} // namespace camera