/**
 * Copyright(C)2020 NoevilMe. All rights reserved.
 * File              : webcam_v4l2.h
 * Author            : NoevilMe <surpass168@live.com>
 * Date              : 2020-05-21 23:00:54
 * Last Modified Date: 2020-07-25 21:56:28
 * Last Modified By  : NoevilMe <surpass168@live.com>
 */
#ifndef __WEBCAM_V4L2_H_
#define __WEBCAM_V4L2_H_

#include "log.h"

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <linux/videodev2.h>

namespace noevil {
namespace webcam {

enum class WebcamFormat {
    kFmtNone,
    kFmtMJPG, // Motion-JPEG
    kFmtYUYV  // YUYV422
};

struct V4l2BufUnit {
    int index = 0;
    uint32_t length = 0;
    uint32_t offset = 0;

    void *start = nullptr;
    uint32_t bytes = 0;
};

struct V4l2BufStat {
    // for preparation
    int count = 0;     // mapped count
    uint32_t type = 0; // enum v4l2_buf_type
    struct V4l2BufUnit *buffer = nullptr;

    // tmp
    struct v4l2_buffer buf;
};

class V4l2BufStatDeleter {
public:
    void operator()(V4l2BufStat *stat);
};

struct V4l2Ctrl {
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
};

class WebcamV4l2 {
public:
    WebcamV4l2();
    WebcamV4l2(const char *name);
    WebcamV4l2(int id);
    ~WebcamV4l2();

    bool IsOpen();

    // get error message if any interface returns false
    std::string GetError() const;

    bool Open(bool force = false);
    bool Open(const char *name);
    bool Close();

    // settings
    bool Init();
    bool SetPixFormat(WebcamFormat fmt, uint32_t width, uint32_t height);
    bool SetFps(uint8_t fps);

    // sync mode
    // @timeout milliseconds
    bool Grab(std::string &out, uint32_t timeout = 100);
    bool Start();
    bool Stop();

    // retrieve
    bool Retrieve(std::string &img);
    bool Retrieve(std::string *img);

    // query util
    bool GetControl();
    bool SetExposure();

private:
    bool IsV4l2VideoDevice();

    bool QueryCapability();
    bool SetInput(const char *name = nullptr);

    bool SetMMap();
    bool FreeMMap();

    bool StreamOn();
    bool StreamOff();

    std::string EnumerateMenu(uint32_t id, int32_t index_min,
                              int32_t index_max);

    static std::string FormatErrno();
    static std::string PixFormatName(uint32_t format);
    // TODO:
    bool SetExtControl();

    bool ShowCtrlMenu(struct v4l2_queryctrl *queryctrl);
    bool ShowCtrlInt(struct v4l2_queryctrl *queryctrl);
    bool ShowControl(struct v4l2_queryctrl *queryctrl);

    void Release();

    // select and grab
    bool GrabFrame(void *&img, uint32_t *length, uint32_t timeout = 100);
    bool GrabFrame(std::string &img, uint32_t timeout = 100);


private:
    bool working_;
    int cam_fd_;
    uint32_t capabilities_;
    uint32_t format_;

    std::string error_;
    std::string dev_name_;
    std::shared_ptr<spdlog::logger> logger_;

    std::map<decltype(V4l2Ctrl::queryctrl.id), V4l2Ctrl> ctrl_;
    std::unique_ptr<V4l2BufStat, V4l2BufStatDeleter> buf_stat_;
};

} // namespace webcam
} // namespace noevil

#endif /* __WEBCAM_V4L2_H_ */
