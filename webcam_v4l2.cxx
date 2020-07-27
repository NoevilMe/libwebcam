/**
 * Copyright(C)2020 NoevilMe. All rights reserved.
 * File              : webcam_v4l2.cxx
 * Author            : NoevilMe <surpass168@live.com>
 * Date              : 2020-05-21 23:02:05
 * Last Modified Date: 2020-07-26 09:33:07
 * Last Modified By  : NoevilMe <surpass168@live.com>
 */
#include "webcam_v4l2.h"

#include "spdlog/fmt/bundled/core.h"
#include "spdlog/fmt/bundled/format.h"
#include "string_util.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <turbojpeg.h>

// The SCALE macro converts a value (sv) from one range (sf -> sr)
#define SCALE(df, dr, sf, sr, sv) (((sv - sf) * (dr - df) / (sr - sf)) + df)

namespace noevil {
namespace webcam {

static constexpr auto QBUF_SIZE = 5;
static constexpr auto VIDEO_DEV_PREFIX = "/dev/video";
static constexpr auto LOGGER_NAME = "webcam-v4l2";

void V4l2BufStatDeleter::operator()(V4l2BufStat *stat) {
    if (!stat->buffer)
        return;

    for (int i = 0; i < stat->count; ++i) {
        munmap(stat->buffer[i].start, stat->buffer[i].length);
    }

    delete[] stat->buffer;
    stat->buffer = nullptr;
}

WebcamV4l2::WebcamV4l2()
    : working_(false),
      cam_fd_(-1),
      capabilities_(0),
      format_(0),
      logger_(util::GetLogger(LOGGER_NAME)) {}

WebcamV4l2::WebcamV4l2(int id)
    : working_(false),
      cam_fd_(-1),
      capabilities_(0),
      format_(0),
      dev_name_(VIDEO_DEV_PREFIX + std::to_string(id)),
      logger_(util::GetLogger(LOGGER_NAME)) {}

WebcamV4l2::WebcamV4l2(const char *name)
    : working_(false),
      cam_fd_(-1),
      capabilities_(0),
      format_(0),
      dev_name_(name),
      logger_(util::GetLogger(LOGGER_NAME)) {}

WebcamV4l2::~WebcamV4l2() { Release(); }

std::string WebcamV4l2::GetError() const { return error_; }

std::string WebcamV4l2::FormatErrno() {
    return fmt::format("{} - {}", errno, strerror(errno));
}

bool WebcamV4l2::Open(bool force) {
    logger_->debug("check {} open", dev_name_);
    if (IsOpen()) {
        if (force) {
            close(cam_fd_);
            cam_fd_ = -1;

        } else {
            logger_->debug("{} is open", dev_name_);
            return true;
        }
    }

    logger_->debug("check {} stat", dev_name_);
    struct stat st;
    if (-1 == stat(dev_name_.data(), &st)) {
        error_ = FormatErrno();
        logger_->error("can not identify {}, {}", dev_name_, error_);
        return false;
    }

    logger_->debug("check {} type", dev_name_);
    // check if it's device
    if (!S_ISCHR(st.st_mode)) {
        error_ = dev_name_ + " is not a device";
        logger_->error(error_);
        return false;
    }

    logger_->debug("openning {} ", dev_name_);
    cam_fd_ = open(dev_name_.data(), O_RDWR | O_NONBLOCK);
    if (cam_fd_ == -1) {
        error_ = FormatErrno();
        logger_->error("open {} failure: {}", dev_name_, error_);
        return false;
    }
    logger_->debug("open {} success", dev_name_);
    return true;
}

bool WebcamV4l2::Open(const char *name) {
    if (!name) {
        return false;
    }
    dev_name_ = name;
    return Open();
}

bool WebcamV4l2::IsOpen() { return cam_fd_ != -1; }

bool WebcamV4l2::Init() {
    logger_->debug("initializing {}", dev_name_);
    if (!IsOpen()) {
        error_ = "webcam is not open";
        logger_->error(error_);
        return false;
    }

    if (!QueryCapability()) {
        return false;
    }

    if (!IsV4l2VideoDevice()) {
        error_ = fmt::format("{} is not a video device", dev_name_);
        logger_->error(error_);
        return false;
    }

    if (!SetInput()) {
        return false;
    }

    return true;
}

bool WebcamV4l2::GrabFrame(void *&img, uint32_t *length, uint32_t timeout) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout * 1000;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(cam_fd_, &fds);

    int r = select(cam_fd_ + 1, &fds, nullptr, nullptr, &tv);

    if (-1 == r) {
        error_ = fmt::format("select failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    if (!r) {
        error_ = fmt::format("select {} ms timeout", timeout);
        logger_->error(error_);
        return false;
    }

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

bool WebcamV4l2::GrabFrame(std::string &img, uint32_t timeout) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout * 1000;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(cam_fd_, &fds);

    int r = select(cam_fd_ + 1, &fds, nullptr, nullptr, &tv);

    if (-1 == r) {
        error_ = fmt::format("select failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    if (!r) {
        error_ = fmt::format("select {} ms timeout", timeout);
        logger_->error(error_);
        return false;
    }

    auto buf_ptr = &buf_stat_->buf;
    memset(buf_ptr, 0, sizeof(*buf_ptr));
    buf_ptr->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_ptr->memory = V4L2_MEMORY_MMAP;

    if (ioctl(cam_fd_, VIDIOC_DQBUF, buf_ptr) == -1) {
        error_ = fmt::format("VIDIOC_DQBUF failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    auto index = buf_ptr->index;
    buf_stat_->buffer[index].bytes = buf_ptr->bytesused;

    img.assign((char *)buf_stat_->buffer[index].start,
               buf_stat_->buffer[index].bytes);

    if (ioctl(cam_fd_, VIDIOC_QBUF, buf_ptr) == -1) {
        error_ = fmt::format("VIDIOC_QBUF failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    return true;
}

bool WebcamV4l2::Close() {
    if (IsOpen()) {
        close(cam_fd_);
        cam_fd_ = -1;
    }

    return true;
}

bool WebcamV4l2::SetInput(const char *name) {
    uint32_t match_index = 0;

    struct v4l2_input cam_input;
    cam_input.index = 0;
    while (ioctl(cam_fd_, VIDIOC_ENUMINPUT, &cam_input) == 0) {
        logger_->debug("enumerate input {} name: {}, type: {}", cam_input.index,
                       cam_input.name, cam_input.type);
        if (name && strncasecmp((char *)cam_input.name, name, 32) == 0) {
            match_index = cam_input.index;
        }

        ++cam_input.index;
    }

    if (cam_input.index == 0) {
        error_ = fmt::format("no input on {}", dev_name_);
        logger_->error(error_);
        return false;
    }

    cam_input.index = match_index;
    if (ioctl(cam_fd_, VIDIOC_ENUMINPUT, &cam_input) == -1) {
        error_ = fmt::format("query input {} failure", match_index);
        logger_->error(error_);
        return false;
    }

    logger_->debug("try to set input index: {}, name: {}, type: {}",
                   cam_input.index, cam_input.name, cam_input.type);

    if (ioctl(cam_fd_, VIDIOC_S_INPUT, &cam_input) == -1) {
        error_ = fmt::format("set input {} failure: {}", cam_input.index,
                             strerror(errno));
        logger_->error(error_);
        return false;
    }

    logger_->debug("set input success");

    return true;
}

bool WebcamV4l2::QueryCapability() {
    if (!IsOpen()) {
        error_ = fmt::format("{} is not open", dev_name_);
        logger_->error(error_);
        return false;
    }

    struct v4l2_capability cam_cap;
    if (ioctl(cam_fd_, VIDIOC_QUERYCAP, &cam_cap) == -1) {
        error_ = FormatErrno();
        logger_->error("query capibility failure: {}", error_);
        return false;
    }
    logger_->info("capabilities: 0x{:X}", cam_cap.capabilities);
    logger_->info("card name   : {}", cam_cap.card);
    logger_->info("driver name : {}", cam_cap.driver);
    logger_->info("version     : {}", cam_cap.version);
    logger_->info("bus info    : {}", cam_cap.bus_info);

    capabilities_ = cam_cap.capabilities;
    return true;
}

bool WebcamV4l2::IsV4l2VideoDevice() {
    // Judge if the device is a camera device
    return (capabilities_ & V4L2_CAP_VIDEO_CAPTURE) != 0;
}

std::string WebcamV4l2::PixFormatName(uint32_t format) {
    char buf[20] = {0};
    sprintf(buf, "[0x%08X] '%c%c%c%c'", format, format >> 0, format >> 8,
            format >> 16, format >> 24);
    return buf;
}

bool WebcamV4l2::SetPixFormat(WebcamFormat fmt, uint32_t width,
                              uint32_t height) {
    if (!IsV4l2VideoDevice()) {
        error_ = fmt::format("{} is not a video device", dev_name_);
        logger_->error(error_);
        return false;
    }

    uint32_t pix_format = 0;

    struct v4l2_fmtdesc fmt_desc;
    fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt_desc.index = 0;
    while (ioctl(cam_fd_, VIDIOC_ENUM_FMT, &fmt_desc) == 0) {
        logger_->info("enumerate format: {}, {}",
                      PixFormatName(fmt_desc.pixelformat),
                      fmt_desc.description);

        if (fmt == WebcamFormat::kFmtMJPG && fmt_desc.pixelformat == V4L2_PIX_FMT_MJPEG) {
            pix_format = V4L2_PIX_FMT_MJPEG;
        }

        if (fmt == WebcamFormat::kFmtYUYV && fmt_desc.pixelformat == V4L2_PIX_FMT_YUYV) {
            pix_format = V4L2_PIX_FMT_YUYV;
        }

        ++fmt_desc.index;
    }

    if (fmt_desc.index == 0) {
        error_ = "no format is supported";
        logger_->error(error_);
        return false;
    }

    // no match, select the 1st format
    if (pix_format == 0) {
        fmt_desc.index = 0;
        if (ioctl(cam_fd_, VIDIOC_ENUM_FMT, &fmt_desc) == 0) {
            pix_format = fmt_desc.pixelformat;
        } else {
            error_ = fmt::format("get index 0 format failure, {}", FormatErrno());
            logger_->error(error_);
            return false;
        }
    }

    logger_->debug("try format {}, {}x{}", PixFormatName(pix_format), width,
                   height);
    struct v4l2_format v4l2_fmt;
    v4l2_fmt.type = V4L2_CAP_VIDEO_CAPTURE;
    v4l2_fmt.fmt.pix.width = width;
    v4l2_fmt.fmt.pix.height = height;
    v4l2_fmt.fmt.pix.pixelformat = pix_format;
    v4l2_fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(cam_fd_, VIDIOC_TRY_FMT, &v4l2_fmt) == -1) {
        error_ = fmt::format("try format {}, {}x{} error, {}",
                             PixFormatName(pix_format), width, height,
                             FormatErrno());
        logger_->error(error_);
        return false;
    }

    if (v4l2_fmt.fmt.pix.pixelformat != pix_format) {
        error_ = fmt::format("format {} is not supported, run as {}",
                             PixFormatName(pix_format),
                             PixFormatName(v4l2_fmt.fmt.pix.pixelformat));
        logger_->error(error_);
        return false;
    }

    logger_->debug("enumerate format {} support frame size",
                   PixFormatName(pix_format));

    struct v4l2_frmsizeenum frmsize;
    frmsize.pixel_format = pix_format;
    frmsize.index = 0;
    while (ioctl(cam_fd_, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
        logger_->debug("size: {}x{}", frmsize.discrete.width,
                       frmsize.discrete.height);

        struct v4l2_frmivalenum frmival;
        memset(&frmival, 0, sizeof(frmival));
        frmival.pixel_format = frmsize.pixel_format;
        frmival.width = frmsize.discrete.width;
        frmival.height = frmsize.discrete.height;
        frmival.type = V4L2_FRMIVAL_TYPE_DISCRETE;
        frmival.index = 0;

        while (ioctl(cam_fd_, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
            logger_->debug("discrete frame interval: {:0.3f}s ({} fps)",
                           (double)frmival.discrete.numerator /
                               frmival.discrete.denominator,
                           frmival.discrete.denominator);
            frmival.index++;
        }
        ++frmsize.index;
    }

    if (v4l2_fmt.fmt.pix.width != width || v4l2_fmt.fmt.pix.height != height) {
        logger_->info("adjust resolution from {}x{} to {}x{}", width, height,
                      v4l2_fmt.fmt.pix.width, v4l2_fmt.fmt.pix.height);
    }

    if (ioctl(cam_fd_, VIDIOC_S_FMT, &v4l2_fmt) == -1) {
        error_ = fmt::format("set pixel format failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    format_ = pix_format;
    logger_->info("enable pixel format {} {}x{}", PixFormatName(format_),
                  v4l2_fmt.fmt.pix.width, v4l2_fmt.fmt.pix.height);

    return true;
}

void WebcamV4l2::Release() {
    Stop();

    Close();

    capabilities_ = 0;
    format_ = 0;
}

bool WebcamV4l2::SetMMap() {
    if (buf_stat_) {
        return true;
    }

    if ((capabilities_ & V4L2_CAP_STREAMING) == 0) {
        error_ = fmt::format("set mmap failure, {} is not a streaming device",
                             dev_name_);
        logger_->error(error_);
        return false;
    }

    std::unique_ptr<V4l2BufStat, V4l2BufStatDeleter> buf_stat(
        new V4l2BufStat, V4l2BufStatDeleter());

    // request
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = QBUF_SIZE;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(cam_fd_, VIDIOC_REQBUFS, &req) == -1) {
        error_ = fmt::format("VIDIOC_REQBUFS failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    logger_->debug("mmap information:");
    logger_->debug("buffer for {} frames", req.count);
    if (req.count < 2) {
        error_ = "Insufficient buffer memory";
        logger_->error(error_);
        return false;
    }

    buf_stat->type = req.type;
    buf_stat->buffer = new V4l2BufUnit[req.count];
    buf_stat->count = 0;

    // query and map
    for (uint32_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer &buf = buf_stat->buf;
        memset(&buf, 0, sizeof(struct v4l2_buffer));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(cam_fd_, VIDIOC_QUERYBUF, &buf) == -1) {
            error_ =
                fmt::format("query buffer {} failure, {}", i, strerror(errno));
            logger_->error(error_);
            return false;
        }

        auto &unit = buf_stat->buffer[i];
        unit.index = i;
        unit.length = buf.length;
        unit.offset = buf.m.offset;
        unit.start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                          MAP_SHARED, cam_fd_, buf.m.offset);

        if (unit.start == MAP_FAILED) {
            error_ = fmt::format("map buffer {} failure, {}", i, FormatErrno());
            logger_->error(error_);
            return false;
        }
        buf_stat->count = i + 1;
    }

    // put in queue
    for (uint32_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer &buf = buf_stat->buf;
        memset(&buf, 0, sizeof(struct v4l2_buffer));

        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(cam_fd_, VIDIOC_QBUF, &buf) == -1) {
            error_ = fmt::format("unable to queue buffer, {}", FormatErrno());
            logger_->error(error_);
            return false;
        }
    }

    buf_stat_ = std::move(buf_stat);
    return true;
}

bool WebcamV4l2::FreeMMap() {
    if (buf_stat_) {
        buf_stat_.reset();
    }
    return true;
}

bool WebcamV4l2::StreamOn() {
    if (!buf_stat_) {
        error_ = "mmap is not ready";
        logger_->error(error_);
        return false;
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam_fd_, VIDIOC_STREAMON, &type) == -1) {
        error_ = fmt::format("streamon failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    return true;
}

bool WebcamV4l2::StreamOff() {
    if (!buf_stat_) {
        error_ = "mmap is not ready";
        logger_->error(error_);
        return false;
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam_fd_, VIDIOC_STREAMOFF, &type) == -1) {
        error_ = fmt::format("streamoff failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    return true;
}

bool WebcamV4l2::SetFps(uint8_t fps) {
    if (!fps) {
        return false;
    }

    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(struct v4l2_streamparm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam_fd_, VIDIOC_G_PARM, &parm) == -1) {
        error_ = fmt::format("VIDIOC_G_PARM failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    logger_->info("current fps {}", parm.parm.capture.timeperframe.denominator);
    if (parm.parm.capture.timeperframe.denominator == fps) {
        return true;
    }

    logger_->info("try to set fps {}", fps);
    struct v4l2_streamparm setfps;
    memset(&setfps, 0, sizeof(setfps));
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = fps;
    if (ioctl(cam_fd_, VIDIOC_S_PARM, &setfps) == -1) {
        /* Not fatal - just warn about it */
        error_ = fmt::format("set fps failure, {}", FormatErrno());
        logger_->warn(error_);
        return false;
    }

    if (ioctl(cam_fd_, VIDIOC_G_PARM, &parm) == -1) {
        error_ = fmt::format("VIDIOC_G_PARM failure, {}", FormatErrno());
        logger_->error(error_);
        return false;
    }

    logger_->info("current fps {}", parm.parm.capture.timeperframe.denominator);
    return true;
}

bool WebcamV4l2::GetControl() {
    struct v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while (0 == ioctl(cam_fd_, VIDIOC_QUERYCTRL, &queryctrl)) {
        ShowControl(&queryctrl);
        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    return true;
}

bool WebcamV4l2::SetExtControl() {
    // might be same as SetControl
    struct v4l2_query_ext_ctrl query_ext_ctrl;
    memset(&query_ext_ctrl, 0, sizeof(query_ext_ctrl));
    query_ext_ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;

    while (0 == ioctl(cam_fd_, VIDIOC_QUERY_EXT_CTRL, &query_ext_ctrl)) {
        if (!(query_ext_ctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
            logger_->info("Ext control {}", query_ext_ctrl.name);

            if (query_ext_ctrl.type == V4L2_CTRL_TYPE_MENU) {
                logger_->info("{}", EnumerateMenu(query_ext_ctrl.id,
                                                  query_ext_ctrl.minimum,
                                                  query_ext_ctrl.maximum));
            }
        }

        query_ext_ctrl.id |=
            V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    }
    return true;
}

std::string WebcamV4l2::EnumerateMenu(uint32_t id, int32_t index_min,
                                      int32_t index_max) {
    struct v4l2_querymenu querymenu;
    memset(&querymenu, 0, sizeof(querymenu));
    querymenu.id = id;

    std::vector<std::string> menu_names;
    for (int32_t m = index_min; m <= index_max; ++m) {
        querymenu.index = m;
        if (0 == ioctl(cam_fd_, VIDIOC_QUERYMENU, &querymenu)) {
            menu_names.push_back((char *)querymenu.name);
        }
    }

    if (menu_names.empty()) {
        return std::string();
    } else {
        return string_util::join(menu_names, " | ");
    }
}

bool WebcamV4l2::ShowCtrlMenu(struct v4l2_queryctrl *queryctrl) {

    std::string options =
        EnumerateMenu(queryctrl->id, queryctrl->minimum, queryctrl->maximum);

    struct v4l2_control control;
    memset(&control, 0, sizeof(control));
    control.id = queryctrl->id;
    if (ioctl(cam_fd_, VIDIOC_G_CTRL, &control) == -1) {
        error_ = fmt::format("read value of control {} failure, {}",
                             queryctrl->name, strerror(errno));
        logger_->error(error_);
        return false;
    }

    struct v4l2_querymenu querymenu;
    memset(&querymenu, 0, sizeof(querymenu));
    querymenu.id = queryctrl->id;
    querymenu.index = control.value;

    if (-1 == ioctl(cam_fd_, VIDIOC_QUERYMENU, &querymenu)) {
        error_ =
            fmt::format("read menu item {} value of control {} failure, {}",
                        control.value, queryctrl->name, strerror(errno));
        logger_->error(error_);
        return false;
    }

    logger_->info("queryctrl menu id: 0x{:X}, name: {}, menu: {}, options: {}",
                  queryctrl->id, queryctrl->name, querymenu.name, options);

    return true;
}

bool WebcamV4l2::ShowCtrlInt(struct v4l2_queryctrl *queryctrl) {
    struct v4l2_control control;
    memset(&control, 0, sizeof(control));
    control.id = queryctrl->id;

    if (ioctl(cam_fd_, VIDIOC_G_CTRL, &control) == -1) {
        logger_->error("read value of control {} failure, {}", queryctrl->name,
                       strerror(errno));
        return false;
    }

    if (queryctrl->maximum - queryctrl->minimum <= 10) {
        logger_->info("queryctrl int  id: 0x{:X}, name: {:<32}, value:{:<12}, "
                      "flags: {:<2} "
                      "(default: {:<4}, [{:<6}:{:<6}:{:<2}])",
                      queryctrl->id, queryctrl->name, control.value,
                      queryctrl->flags, queryctrl->default_value,
                      queryctrl->minimum, queryctrl->maximum, queryctrl->step);
    } else {
        logger_->info("queryctrl int  id: 0x{:X}, name: {:<32}, value:{:<5} - "
                      "{:>3}%, flags: "
                      "{:<2} (default: {:<4}, [{:<6}:{:<6}:{:<2}])",
                      queryctrl->id, queryctrl->name, control.value,
                      SCALE(0, 100, queryctrl->minimum, queryctrl->maximum,
                            control.value),
                      queryctrl->flags, queryctrl->default_value,
                      queryctrl->minimum, queryctrl->maximum, queryctrl->step);
    }

    V4l2Ctrl ctrl{.queryctrl = *queryctrl, .control = control};
    ctrl_[control.id] = ctrl;

    return true;
}

bool WebcamV4l2::ShowControl(struct v4l2_queryctrl *queryctrl) {
    if (!queryctrl) {
        return false;
    }

    if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED) {
        logger_->info(
            "queryctrl id: 0x{:X}, name: {:<32}, DISABLED, flags: {:<2} ",
            queryctrl->id, queryctrl->name, queryctrl->flags);
        return false;
    }

    switch (queryctrl->type) {
    case V4L2_CTRL_TYPE_INTEGER:
        if (!ShowCtrlInt(queryctrl)) {
            return false;
        }
        break;

    case V4L2_CTRL_TYPE_BOOLEAN: {
        struct v4l2_control control;
        memset(&control, 0, sizeof(control));

        control.id = queryctrl->id;
        if (ioctl(cam_fd_, VIDIOC_G_CTRL, &control) == -1) {
            error_ = fmt::format("read value of control {} failure, {}",
                                 queryctrl->name, strerror(errno));
            logger_->error(error_);
            return false;
        }
        logger_->info("queryctrl bool id: 0x{:X}, name: {:<32}, value:{:<12}, "
                      "flags: {:<2} "
                      "(default: {})",
                      queryctrl->id, queryctrl->name,
                      control.value ? "True" : "False", queryctrl->flags,
                      queryctrl->default_value ? "True" : "False");

        V4l2Ctrl ctrl{.queryctrl = *queryctrl, .control = control};
        ctrl_[control.id] = ctrl;
    } break;

    case V4L2_CTRL_TYPE_MENU:
        if (!ShowCtrlMenu(queryctrl)) {
            return false;
        }
        break;

    case V4L2_CTRL_TYPE_BUTTON:
        logger_->info("queryctrl btn id: 0x{:X}, name: {:<32} - [Button]",
                      queryctrl->id, queryctrl->name);
        break;

    default:
        logger_->info("queryctrl deft id: 0x{:X}, name: {:<32} N/A [Unknown "
                      "Control Type]",
                      queryctrl->id, queryctrl->name);
        break;
    }

    return true;
}

bool WebcamV4l2::SetExposure() {
    int ret;
    struct v4l2_control ctrl;
    //得到曝光模式
    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    if (ioctl(cam_fd_, VIDIOC_G_CTRL, &ctrl) == -1) {
        printf("Get exposure auto Type failed\n");
        return false;
    }
    printf("\nGet Exposure Auto Type:[%d]\n", ctrl.value);

    // ctrl.id = V4L2_CID_ROTATE;
    // ctrl.value = 90;
    // if (ioctl(cam_fd_, VIDIOC_S_CTRL, &ctrl) == -1) {
    // printf("Set rotate failed\n");
    // return false;
    //}
    // printf("\nSet rotate:[%d]\n", ctrl.value);
    // struct v4l2_control ctrl;
    // ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    // if (ioctl(cam_fd_, VIDIOC_G_CTRL, &ctrl) == -1) {
    // logger_->warn("get exposure failure, {}", strerror(errno));
    //}
    // logger_->info("exposure {}", ctrl.value);
    return false;
}

// sync mode
bool WebcamV4l2::Grab(std::string &out, uint32_t timeout) {
    if (!working_) {
        error_ = "not started";
        logger_->error(error_);
        return false;
    }

    if (!buf_stat_) {
        error_ = "v4l2 buffers are not ready";
        logger_->error(error_);
        return false;
    }

    std::string img_bytes;

    if (!GrabFrame(img_bytes, timeout)) {
        error_ = fmt::format("grab frame failure, {}", error_);
        logger_->error(error_);
        return false;
    }

    if (transform_) {
        return transform_->Transform(img_bytes, out);
    } else {
        out.swap(img_bytes);
        return true;
    }
}

bool WebcamV4l2::Retrieve(std::string &img) {
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(cam_fd_, VIDIOC_DQBUF, &buf) == -1) {
        logger_->error("retrieve VIDIOC_DQBUF failure, {}", FormatErrno());
        return false;
    }

    img.assign((char *)buf_stat_->buffer[buf.index].start, buf.bytesused);

    if (ioctl(cam_fd_, VIDIOC_QBUF, &buf) == -1) {
        logger_->error("retrieve VIDIOC_QBUF failure, {}", FormatErrno());
        return false;
    }

    return true;
}

bool WebcamV4l2::Retrieve(std::string *img) {
    if (img) {
        return Retrieve(*img);
    } else {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(cam_fd_, VIDIOC_DQBUF, &buf) == -1) {
            logger_->error("retrieve VIDIOC_DQBUF failure, {}", FormatErrno());
            return false;
        }

        if (ioctl(cam_fd_, VIDIOC_QBUF, &buf) == -1) {
            logger_->error("retrieve VIDIOC_QBUF failure, {}", FormatErrno());
            return false;
        }

        return true;
    }
}

bool WebcamV4l2::Start() {
    if (working_) {
        return true;
    }

    if (!SetMMap()) {
        return false;
    }

    working_ = StreamOn();
    logger_->info("start working {}", working_);
    return working_;
}

bool WebcamV4l2::Stop() {
    if (!working_) {
        return false;
    }

    StreamOff();

    FreeMMap();

    working_ = false;
    logger_->info("stop working");
    return true;
}

bool WebcamV4l2::SetTransform(JpegTransform::JpegTransformOp op) {
    if (op != JpegTransform::JpegTransformOp::kTransNone) {
        try {
            transform_.reset(new JpegTransform(op));
            logger_->info("JpegTransform is created");
        } catch (std::runtime_error &ex) {
            error_ =
                fmt::format("failed to reset JpegTransform, {}", ex.what());
            logger_->error(error_);
            return false;
        }
    }

    return true;
}

} // namespace webcam
} // namespace noevil
