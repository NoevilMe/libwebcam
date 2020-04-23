/**
 * Copyright(C)2020 NoevilMe. All rights reserved.
 * File              : webcam_v4l2.h
 * Author            : NoevilMe <surpass168@live.com>
 * Date              : 2020-04-23 19:06:29
 * Last Modified Date: 2020-04-23 19:06:29
 * Last Modified By  : NoevilMe <surpass168@live.com>
 */
/**
 * webcam_v4l2.h
 * Copyright (c) 2020 NoevilMe <surpass168@live.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __WEBCAM_V4L2_H_
#define __WEBCAM_V4L2_H_

#include <memory>
#include <string>

#include <linux/videodev2.h>

#include "log.h"

enum WebcamFormat {
    kFmtNone,
    kFmtMJPG, // Motion-JPEG
    kFmtYUYV  // YUYV422
};

struct V4l2BufUnit {
    int index = 0;
    void* start = nullptr;
    uint32_t length = 0;
    uint32_t offset = 0;
    uint32_t bytes = 0;
};

struct V4l2BufStat {
    // for preparation
    int count = 0;      // mapped count
    uint32_t type = 0; // enum v4l2_buf_type
    struct V4l2BufUnit* buffer = nullptr;

    // for process
    int process_frame_index = 0;
    struct v4l2_buffer buf;
};

class V4l2BufStatDeleter {
public:
    void operator()(V4l2BufStat* stat);
};

class WebcamV4l2 {
public:
    WebcamV4l2();
    WebcamV4l2(const char* name);
    WebcamV4l2(int id);
    ~WebcamV4l2();

    bool Open();
    bool Open(const char* name);
    bool IsOpen();

    bool IsV4l2VideoDevice();

    bool QueryCapability();
    bool SetInput(const char* name = nullptr);
    bool SetPixFormat(WebcamFormat fmt, uint32_t width, uint32_t height);

    bool SetMMap();
    bool FreeMMap();

    bool StreamOn();
    bool StreamOff();

    bool Init();
    bool Grap();

    bool Close();

private:
    void Release();

    bool GrapFrame(void * &img, uint32_t *length);

private:
    std::shared_ptr<spdlog::logger> logger_;
    std::string dev_name_;
    int cam_fd_;
    uint32_t capabilities_;
    uint32_t format_;

    std::unique_ptr<V4l2BufStat, V4l2BufStatDeleter> buf_stat_;
};

#endif /* __WEBCAM_V4L2_H_ */
