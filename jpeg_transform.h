#ifndef __JPEG_TRANSFORM_H_
#define __JPEG_TRANSFORM_H_

#include <string>
#include <turbojpeg.h>

namespace webcam {

class JpegTransform {
public:
    enum JpegTransformOp {
        kTransNone,
        kTransRot90,
        kTransRot180,
        kTransRot270
    };

public:
    JpegTransform(JpegTransformOp op);
    ~JpegTransform();

    bool Transform(unsigned char* jpeg_buf, unsigned long jpeg_size, std::string& out);
    bool Transform(const std::string& jpeg, std::string& out);

private:
    tjhandle handle_;
    tjtransform xtrans_;
};

}
#endif /* __JPEG_TRANSFORM_H_ */
