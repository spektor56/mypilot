#pragma once

#include <unistd.h>
#include "cereal/visionipc/visionipc_server.h"
#include "common/queue.h"
#include "selfdrive/ui/replay/framereader.h"
#include "selfdrive/ui/replay/logreader.h"

class CameraServer {
public:
  CameraServer(std::pair<int, int> camera_size[MAX_CAMERAS] = nullptr, bool send_yuv = false);
  ~CameraServer();
  void pushFrame(CameraType type, FrameReader* fr, const cereal::EncodeIndex::Reader& eidx);
  void waitForSent();

protected:
  struct Camera {
    CameraType type;
    VisionStreamType rgb_type;
    VisionStreamType yuv_type;
    int width;
    int height;
    std::thread thread;
    SafeQueue<std::pair<FrameReader*, const cereal::EncodeIndex::Reader>> queue;
    int cached_id = -1;
    int cached_seg = -1;
    std::pair<VisionBuf *, VisionBuf*> cached_buf;
  };
  void startVipcServer();
  void cameraThread(Camera &cam);

  Camera cameras_[MAX_CAMERAS] = {
      {.type = RoadCam, .rgb_type = VISION_STREAM_RGB_ROAD, .yuv_type = VISION_STREAM_ROAD},
      {.type = DriverCam, .rgb_type = VISION_STREAM_RGB_DRIVER, .yuv_type = VISION_STREAM_DRIVER},
      {.type = WideRoadCam, .rgb_type = VISION_STREAM_RGB_WIDE_ROAD, .yuv_type = VISION_STREAM_WIDE_ROAD},
  };
  std::atomic<int> publishing_ = 0;
  std::unique_ptr<VisionIpcServer> vipc_server_;
  bool send_yuv;
};
