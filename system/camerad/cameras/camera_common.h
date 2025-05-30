#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <thread>

#include "cereal/messaging/messaging.h"
#include "cereal/visionipc/visionbuf.h"
#include "cereal/visionipc/visionipc.h"
#include "cereal/visionipc/visionipc_server.h"
#include "system/camerad/transforms/rgb_to_yuv.h"
#include "common/mat.h"
#include "common/queue.h"
#include "common/swaglog.h"
#include "common/visionimg.h"
#include "system/hardware/hw.h"

#define CAMERA_ID_IMX298 0
#define CAMERA_ID_IMX179 1
#define CAMERA_ID_S5K3P8SP 2
#define CAMERA_ID_OV8865 3
#define CAMERA_ID_IMX298_FLIPPED 4
#define CAMERA_ID_OV10640 5
#define CAMERA_ID_LGC920 6
#define CAMERA_ID_LGC615 7
#define CAMERA_ID_AR0231 8
#define CAMERA_ID_IMX390 9
#define CAMERA_ID_MAX 10

const int UI_BUF_COUNT = 4;
const int YUV_BUFFER_COUNT = 100;

enum CameraType {
  RoadCam = 0,
  DriverCam,
  WideRoadCam
};

// TODO: remove these once all the internal tools are moved to vipc
const bool env_send_driver = getenv("SEND_DRIVER") != NULL;
const bool env_send_road = getenv("SEND_ROAD") != NULL;
const bool env_send_wide_road = getenv("SEND_WIDE_ROAD") != NULL;

// for debugging
const bool env_disable_road = getenv("DISABLE_ROAD") != NULL;
const bool env_disable_wide_road = getenv("DISABLE_WIDE_ROAD") != NULL;
const bool env_disable_driver = getenv("DISABLE_DRIVER") != NULL;
const bool env_debug_frames = getenv("DEBUG_FRAMES") != NULL;
const bool env_log_raw_frames = getenv("LOG_RAW_FRAMES") != NULL;

typedef void (*release_cb)(void *cookie, int buf_idx);

typedef struct CameraInfo {
  uint32_t frame_width, frame_height;
  uint32_t frame_stride;
  bool bayer;
  int bayer_flip;
  bool hdr;
  uint32_t frame_offset = 0;
  uint32_t extra_height = 0;
  int registers_offset = -1;
  int stats_offset = -1;
} CameraInfo;

typedef struct FrameMetadata {
  uint32_t frame_id;
  unsigned int frame_length;

  // Timestamps
  uint64_t timestamp_sof; // only set on tici
  uint64_t timestamp_eof;

  // Exposure
  unsigned int integ_lines;
  bool high_conversion_gain;
  float gain;
  float measured_grey_fraction;
  float target_grey_fraction;

  // Focus
  unsigned int lens_pos;
  float lens_err;
  float lens_true_pos;

  float processing_time;
} FrameMetadata;

typedef struct CameraExpInfo {
  int op_id;
  float grey_frac;
} CameraExpInfo;

struct MultiCameraState;
struct CameraState;
class Debayer;

class CameraBuf {
private:
  VisionIpcServer *vipc_server;
  CameraState *camera_state;
  Debayer *debayer = nullptr;
  std::unique_ptr<Rgb2Yuv> rgb2yuv;

  VisionStreamType rgb_type, yuv_type;

  int cur_buf_idx;

  SafeQueue<int> safe_queue;

  int frame_buf_count;
  release_cb release_callback;

public:
  cl_command_queue q;
  FrameMetadata cur_frame_data;
  VisionBuf *cur_rgb_buf;
  VisionBuf *cur_yuv_buf;
  VisionBuf *cur_camera_buf;
  std::unique_ptr<VisionBuf[]> camera_bufs;
  std::unique_ptr<FrameMetadata[]> camera_bufs_metadata;
  int rgb_width, rgb_height, rgb_stride;

  mat3 yuv_transform;

  CameraBuf() = default;
  ~CameraBuf();
  void init(cl_device_id device_id, cl_context context, CameraState *s, VisionIpcServer * v, int frame_cnt, VisionStreamType rgb_type, VisionStreamType yuv_type, release_cb release_callback=nullptr);
  bool acquire();
  void release();
  void queue(size_t buf_idx);
};

typedef void (*process_thread_cb)(MultiCameraState *s, CameraState *c, int cnt);

void fill_frame_data(cereal::FrameData::Builder &framed, const FrameMetadata &frame_data);
kj::Array<uint8_t> get_frame_image(const CameraBuf *b);
kj::Array<uint8_t> get_raw_frame_image(const CameraBuf *b);
float set_exposure_target(const CameraBuf *b, int x_start, int x_end, int x_skip, int y_start, int y_end, int y_skip);
std::thread start_process_thread(MultiCameraState *cameras, CameraState *cs, process_thread_cb callback);
void common_process_driver_camera(MultiCameraState *s, CameraState *c, int cnt);

void cameras_init(VisionIpcServer *v, MultiCameraState *s, cl_device_id device_id, cl_context ctx);
void cameras_open(MultiCameraState *s);
void cameras_run(MultiCameraState *s);
void cameras_close(MultiCameraState *s);
void camera_autoexposure(CameraState *s, float grey_frac);
void camerad_thread();

int open_v4l_by_name_and_index(const char name[], int index = 0, int flags = O_RDWR | O_NONBLOCK);
