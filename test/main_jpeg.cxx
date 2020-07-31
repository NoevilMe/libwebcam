#include "jpeg_transform.h"
#include "webcam_v4l2.h"
#include <iostream>
#include <string>

using namespace noevil::webcam;

bool WriteFile(const std::string &path, const std::string &content) {
    int fd = open(path.data(), O_RDWR | O_CREAT, 00664);
    if (fd == -1) {
        throw std::runtime_error(
            fmt::format("Failed to open {}, {}", path, strerror(errno)));
    }

    int writesize = write(fd, content.data(), content.length());
    close(fd);
    return true;
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");

    noevil::util::Init("cam.log");
    noevil::util::SetLevel(spdlog::level::trace);

    noevil::webcam::WebcamV4l2 cam(argv[1]);
    if (!cam.Open()) {
        return 1;
    }
    if (!cam.Init()) {
        std::cout << "init failure, " << cam.GetError() << std::endl;
        return 1;
    }

    if (!cam.SetPixFormat(noevil::webcam::WebcamFormat::kFmtMJPG, 1920, 1080)) {
        std::cout << "set format failure, " << cam.GetError() << std::endl;
        return 1;
    }

    if (!cam.Start()) {
        std::cout << "start failure, " << cam.GetError() << std::endl;
        return 1;
    }

    std::unique_ptr<JpegTransform> transform(
        new JpegTransform(JpegTransform::JpegTransformOp::kTransRot90));

    for (int i = 0; i < 100; ++i) {
        std::string frm;
        if (cam.Grab(frm)) {
            std::string name = std::to_string(i) + ".jpg";
            WriteFile(name, frm);

            std::string rotate;
            transform->Transform(frm, rotate);
            WriteFile(std::to_string(i) + "_90.jpg", rotate);
        }
    }

    cam.Stop();

    return 0;
}
