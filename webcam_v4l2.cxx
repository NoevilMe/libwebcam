/**
 * Copyright(C)2020 NoevilMe. All rights reserved.
 * File              : webcam_v4l2.cxx
 * Author            : NoevilMe <surpass168@live.com>
 * Date              : 2020-04-23 19:07:00
 * Last Modified Date: 2020-04-23 19:07:00
 * Last Modified By  : NoevilMe <surpass168@live.com>
 */
/**
 * /home/pi/libwebcam/webcam_v4l2.cxx
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
#include "webcam_v4l2.h"

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <utility>


constexpr auto VIDEO_DEV_PREFIX = "/dev/video";

void V4l2BufStatDeleter::operator()(V4l2BufStat* stat)
{
    for (int i = 0; i < stat->count; ++i) {
        munmap(stat->buffer[i].start, stat->buffer[i].length);
    }

    if (stat->buffer) {
        delete[] stat->buffer;
        stat->buffer = nullptr;
    }
}

WebcamV4l2::WebcamV4l2()
    : cam_fd_(-1)
    , capabilities_(0)
    , format_(0)
{
    logger_ = util::GetLogger("webcam-v4l2");
}

WebcamV4l2::WebcamV4l2(int id)
    : cam_fd_(-1)
    , capabilities_(0)
    , format_(0)
    , dev_name_(VIDEO_DEV_PREFIX + std::to_string(id))
{
    logger_ = util::GetLogger("webcam-v4l2");
}

WebcamV4l2::WebcamV4l2(const char* name)
    : cam_fd_(-1)
    , capabilities_(0)
    , format_(0)
    , dev_name_(VIDEO_DEV_PREFIX + std::string(name))
{
    logger_ = util::GetLogger("webcam-v4l2");
}

WebcamV4l2::~WebcamV4l2()
{
    Release();
}

bool WebcamV4l2::Open()
{
    if (IsOpen()) {
        close(cam_fd_);
    }

    cam_fd_ = open(dev_name_.data(), O_RDWR);
    if (cam_fd_ == -1) {
        logger_->error("open {} failure: {}", dev_name_, strerror(errno));
        return false;
    }
    return true;
}

bool WebcamV4l2::Open(const char* name)
{
    if (nullptr == name) {
        return false;
    }
    dev_name_ = VIDEO_DEV_PREFIX + std::string(name);
    return Open();
}

bool WebcamV4l2::IsOpen()
{
    return cam_fd_ != -1;
}

bool WebcamV4l2::Init()
{
    if (!IsOpen()) {
        logger_->error("webcam is not open");
        return false;
    }

    if (!QueryCapability()) {
        return false;
    }

    if (!IsV4l2VideoDevice()) {
        logger_->error("not a video capture device");
        return false;
    }

    // List input
    if (!SetInput()) {
        return false;
    }

    // TODO: set controls

    //if (!SetPixFormat(kFmtNone, 1920, 1180)) {
    if (!SetPixFormat(kFmtNone, 1600, 1180)) {
        return false;
    }

    // TODO: set fps

    if (!SetMMap()) {
        return false;
    }

    return false;
}

bool WebcamV4l2::Grap()
{
    if (!buf_stat_) {
        logger_->error("v4l2 buffers are not ready");
        return false;
    }

    void* img = nullptr;
    uint32_t length = 0;

    int skip = 10;
    int i = 0;

    while(i++ < 100) {
        if (!GrapFrame(img, &length)) {
            logger_->error("grap frame {} failure", i);
            return false;
        }

        int jpg_fd = open((std::string("test") + std::to_string(i) + ".jpeg").data(), O_RDWR | O_CREAT, 00700);
        if (jpg_fd == -1) {
            printf("open ipg Failed!\n ");
            return false;
            ;
        }
        int writesize = write(jpg_fd, img, length);
        printf("Write successfully size : %d\n", writesize);
        close(jpg_fd);

    }



    return true;
}

bool WebcamV4l2::GrapFrame(void*& img, uint32_t* length)
{
    auto buf_ptr = &buf_stat_->buf;
    memset(buf_ptr, 0, sizeof(*buf_ptr));
    buf_ptr->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_ptr->memory = V4L2_MEMORY_MMAP;

    if (ioctl(cam_fd_, VIDIOC_DQBUF, buf_ptr) == -1) {
        logger_->error("VIDIOC_DQBUF failure");
        return false;
    }

    auto index = buf_ptr->index;
    buf_stat_->buffer[index].bytes = buf_ptr->bytesused;

    img = buf_stat_->buffer[index].start;
    *length = buf_stat_->buffer[index].bytes;

    if (ioctl(cam_fd_, VIDIOC_QBUF, buf_ptr) == -1) {
        logger_->error("VIDIOC_QBUF failure");
        return false;
    }

    return true;
}

bool WebcamV4l2::Close()
{
    if (!IsOpen()) {
        close(cam_fd_);
        cam_fd_ = -1;
    }

    return true;
}

bool WebcamV4l2::SetInput(const char* name)
{
    if (!IsV4l2VideoDevice()) {
        logger_->error("{} is not v4L2 device", dev_name_);
        return false;
    }

    struct v4l2_input cam_input;
    cam_input.index = 0;
    uint32_t match_index = cam_input.index;
    while (ioctl(cam_fd_, VIDIOC_ENUMINPUT, &cam_input) == 0) {
        logger_->trace("enumerate input {} name: {}, type: {}", cam_input.index, cam_input.name, cam_input.type);
        if (name != nullptr && strncasecmp((char*)cam_input.name, name, 32) == 0) {
            match_index = cam_input.index;
        }

        ++cam_input.index;
    }

    if (cam_input.index == 0) {
        logger_->error("no input on {}", dev_name_);
        return false;
    }

    cam_input.index = match_index;
    if (ioctl(cam_fd_, VIDIOC_ENUMINPUT, &cam_input) == -1) {
        logger_->error("query input {} failure", match_index);
        return false;
    }

    logger_->debug("try to set input index: {}, name: {}, type: {}", cam_input.index, cam_input.name, cam_input.type);

    if (ioctl(cam_fd_, VIDIOC_S_INPUT, &cam_input) == -1) {
        logger_->error("set input {} failure: {}", cam_input.index, strerror(errno));
        return false;
    }

    logger_->debug("set input success");

    return true;
}

bool WebcamV4l2::QueryCapability()
{
    if (!IsOpen()) {
        logger_->error("{} is not open", dev_name_);
        return false;
    }

    // Judge if the device is a camera device
    struct v4l2_capability cam_cap;
    if (ioctl(cam_fd_, VIDIOC_QUERYCAP, &cam_cap) == -1) {
        logger_->error("query capibility failure: {}", strerror(errno));
        return false;
    }
    logger_->info("card name: {}", cam_cap.card);
    logger_->info("driver name: {}", cam_cap.driver);
    logger_->info("version: {}", cam_cap.version);
    logger_->info("bus info: {}", cam_cap.bus_info);

    capabilities_ = cam_cap.capabilities;
    return true;
}

bool WebcamV4l2::IsV4l2VideoDevice()
{
    return (capabilities_ & V4L2_CAP_VIDEO_CAPTURE) != 0;
}

bool WebcamV4l2::SetPixFormat(WebcamFormat fmt, uint32_t width, uint32_t height)
{
    if (!IsV4l2VideoDevice()) {
        logger_->error("{} is not v4L2 device", dev_name_);
        return false;
    }

    uint32_t pix_format = 0;

    struct v4l2_fmtdesc fmt_desc;
    fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt_desc.index = 0;
    while (ioctl(cam_fd_, VIDIOC_ENUM_FMT, &fmt_desc) == 0) {
        logger_->info("enumerate format: {:X}, {}", fmt_desc.pixelformat,
            fmt_desc.description);

        if (fmt == kFmtMJPG && fmt_desc.pixelformat == V4L2_PIX_FMT_MJPEG) {
            pix_format = V4L2_PIX_FMT_MJPEG;
        }

        if (fmt == kFmtYUYV && fmt_desc.pixelformat == V4L2_PIX_FMT_YUYV) {
            pix_format = V4L2_PIX_FMT_YUYV;
        }

        ++fmt_desc.index;
        fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }

    if (fmt_desc.index == 0) {
        logger_->error("no format is supported");
        return false;
    }

    if (pix_format == 0) {
        fmt_desc.index = 0;
        if (ioctl(cam_fd_, VIDIOC_ENUM_FMT, &fmt_desc) == 0) {
            pix_format = fmt_desc.pixelformat;
        } else {
            logger_->error("get index 0 format failure, {}", strerror(errno));
            return false;
        }
    }

    logger_->debug("try format {:X}, {}x{}", pix_format, width, height);
    struct v4l2_format v4l2_fmt;
    v4l2_fmt.type = V4L2_CAP_VIDEO_CAPTURE;
    v4l2_fmt.fmt.pix.width = width;
    v4l2_fmt.fmt.pix.height = height;
    v4l2_fmt.fmt.pix.pixelformat = pix_format;
    v4l2_fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(cam_fd_, VIDIOC_TRY_FMT, &v4l2_fmt) == -1) {
        logger_->error("try format {:X}, {}x{} error, {}", pix_format, width, height, strerror(errno));
        return false;
    }

    if (v4l2_fmt.fmt.pix.pixelformat != pix_format) {
        logger_->error("format {:X} is not supported, run as {:X}", pix_format, v4l2_fmt.fmt.pix.pixelformat);
        return false;
    }

    logger_->debug("enumerate format {:X} size", pix_format);
    struct v4l2_frmsizeenum frmsize;
    frmsize.pixel_format = pix_format;
    frmsize.index = 0;
    while (ioctl(cam_fd_, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
        logger_->debug("{}x{}", frmsize.discrete.width,
            frmsize.discrete.height);
        ++frmsize.index;
    }

    if (v4l2_fmt.fmt.pix.width != width || v4l2_fmt.fmt.pix.height != height) {
        logger_->info("Adjusting resolution from {}x{} to {}x{}", width, height, v4l2_fmt.fmt.pix.width, v4l2_fmt.fmt.pix.height);
    }

    if (ioctl(cam_fd_, VIDIOC_S_FMT, &v4l2_fmt) == -1) {
        logger_->error("set pixel format failure, {}", strerror(errno));
        return false;
    }

    // TODO:
    //if(format_  == V4L2_PIX_FMT_MJPEG)
    //{
    //    struct v4l2_jpegcompression jpegcomp;

    //    memset(&jpegcomp, 0, sizeof(jpegcomp));
    //    ioctl(cam_fd_, VIDIOC_G_JPEGCOMP, &jpegcomp);
    //    jpegcomp.jpeg_markers |= V4L2_JPEG_MARKER_DHT;
    //    ioctl(cam_fd_, VIDIOC_S_JPEGCOMP, &jpegcomp);
    //}
    format_ = pix_format;

    return true;
}

void WebcamV4l2::Release()
{
    FreeMMap();

    Close();

    capabilities_ = 0;
    format_ = 0;
}

bool WebcamV4l2::SetMMap()
{
    if ((capabilities_ & V4L2_CAP_STREAMING) == 0) {
        logger_->error("set mmap failure, {} is not a streaming device", dev_name_);
        return false;
    }

    std::unique_ptr<V4l2BufStat, V4l2BufStatDeleter> buf_stat(new V4l2BufStat, V4l2BufStatDeleter());

    // request
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(cam_fd_, VIDIOC_REQBUFS, &req) == -1) {
        logger_->error("set mmap failure, {}", strerror(errno));
        return false;
    }

    logger_->debug("mmap information:");
    logger_->debug("buffer for {} frames", req.count);
    if (req.count < 2) {
        logger_->error("Insufficient buffer memory");
        return false;
    }

    buf_stat->count = 0;
    buf_stat->type = req.type;
    buf_stat->buffer = new V4l2BufUnit[req.count];

    // query and map
    for (uint32_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(cam_fd_, VIDIOC_QUERYBUF, &buf) == -1) {
            logger_->error("query buffer {} failure, {}", i, strerror(errno));
            return false;
        }

        auto& unit = buf_stat->buffer[i];
        unit.index = i;
        unit.length = buf.length;
        unit.offset = buf.m.offset;
        unit.start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam_fd_, buf.m.offset);

        if (unit.start == MAP_FAILED) {
            logger_->error("map buffer {} failure, {}", i, strerror(errno));
            return false;
        }
        buf_stat->count = i + 1;
    }

    // put in queue
    for (uint32_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(struct v4l2_buffer));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(cam_fd_, VIDIOC_QBUF, &buf) == -1) {
            logger_->error("unable to queue buffer, {}", strerror(errno));
            return false;
        }
    }

    buf_stat_ = std::move(buf_stat);

    logger_->debug("queue buffer done");
    return true;
}

bool WebcamV4l2::FreeMMap()
{
    if (buf_stat_) {
        buf_stat_.reset();
        return true;
    }
    return false;
}

bool WebcamV4l2::StreamOn()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam_fd_, VIDIOC_STREAMON, &type) == -1) {
        logger_->error("streamon failure, {}", strerror(errno));
        return false;
    }

    return true;
}

bool WebcamV4l2::StreamOff()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam_fd_, VIDIOC_STREAMOFF, &type) == -1) {
        logger_->error("streamoff failure, {}", strerror(errno));
        return false;
    }

    return true;
}
