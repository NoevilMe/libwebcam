#include "jpeg_transform.h"

#include <cstring>
#include <stdexcept>

namespace webcam {

JpegTransform::JpegTransform(JpegTransformOp op)
    : handle_(tjInitTransform())
{
    if (!handle_) {
        throw std::runtime_error(tjGetErrorStr());
    }

    memset(&xtrans_, 0, sizeof(xtrans_));

    switch (op) {
    case kTransNone:
        xtrans_.op = TJXOP_NONE;
        break;
    case kTransRot90:
        xtrans_.op = TJXOP_ROT90;
        break;
    case kTransRot180:
        xtrans_.op = TJXOP_ROT180;
        break;
    case kTransRot270:
        xtrans_.op = TJXOP_ROT270;
        break;
    default:
        xtrans_.op = TJXOP_NONE;
    }

    xtrans_.options |= TJXOPT_TRIM;
}

JpegTransform::~JpegTransform()
{
    if (handle_) {
        tjDestroy(handle_);
    }
}

bool JpegTransform::Transform(unsigned char* jpeg_buf,
    unsigned long jpeg_size, std::string& out)
{
    if (xtrans_.op == TJXOP_NONE) {
        return false;
    }

    unsigned long dst_size = 0;
    unsigned char* dst_buf = nullptr;

    int rel = tjTransform(handle_, jpeg_buf, jpeg_size, 1, &dst_buf, &dst_size,
        &xtrans_, TJFLAG_ACCURATEDCT);
    if (rel) {
        throw std::runtime_error(tjGetErrorStr());
    }

    out.assign((char*)dst_buf, dst_size);

    if (dst_size) {
        tjFree(dst_buf);
    }

    return true;
}

bool JpegTransform::Transform(const std::string& jpeg, std::string& out)
{
    return Transform((unsigned char*)jpeg.data(), jpeg.size(), out);
}

}
