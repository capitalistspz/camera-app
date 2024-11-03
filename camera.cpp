//
// Created by pz on 02/11/24.
//

#include "camera.hpp"
#include <cstdlib>
#include <whb/log.h>

namespace camera
{
void* Alloc(uint32_t alignment, uint32_t size)
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
unsigned s_surfaceIndex = 0;
bool s_submitNewSurface = true;
void* s_workMemBuf = nullptr;

void EventHandler(CAMEventData* evtData)
{
    if (evtData->eventType == CAMERA_DECODE_DONE)
    {
        s_submitNewSurface = true;
        s_surfaceIndex = !s_surfaceIndex;
    }
}

bool Init()
{
    CAMStreamInfo streamInfo{.type = CAMERA_STREAM_TYPE_1, .height = CAMERA_HEIGHT, .width = CAMERA_WIDTH};
    const auto memReq = CAMGetMemReq(&streamInfo);
    if (memReq <= 0)
    {
        WHBLogPrintf("Failed to get memory requirement: %x", memReq);
        return false;
    }
    s_workMemBuf = Alloc(0x100, memReq);

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
    s_surfaces[0].surfaceBuffer = Alloc(CAMERA_YUV_BUFFER_ALIGNMENT, CAMERA_YUV_BUFFER_SIZE);

    s_surfaces[1].surfaceSize = CAMERA_YUV_BUFFER_SIZE;
    s_surfaces[1].alignment = CAMERA_YUV_BUFFER_ALIGNMENT;
    s_surfaces[1].surfaceBuffer = Alloc(CAMERA_YUV_BUFFER_ALIGNMENT, CAMERA_YUV_BUFFER_SIZE);
    return true;
}

void* GetSurfaceBuffer()
{
    if (s_submitNewSurface)
    {
        if (CAMSubmitTargetSurface(s_handle, &s_surfaces[s_surfaceIndex]) == CAMERA_ERROR_OK)
        {
            s_submitNewSurface = false;
        }
    }
    return s_surfaces[!s_surfaceIndex].surfaceBuffer;
}
void Exit()
{
    CAMExit(s_handle);
    std::free(s_workMemBuf);
}
} // namespace camera