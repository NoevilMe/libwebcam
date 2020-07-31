#include "webcam_v4l2.h"
#include <iostream>
#include <string>
#include <cstdio>


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

    if (!cam.SetPixFormat(noevil::webcam::WebcamFormat::kFmtYUYV, 1280, 720)) {
        std::cout << "set format failure, " << cam.GetError() << std::endl;
        return 1;
    }

    if (!cam.Start()) {
        std::cout << "start failure, " << cam.GetError() << std::endl;
        return 1;
    }

    FILE *fp = fopen("video_yuv422.yuv", "wb+");

    auto cb=[&](const char* const data, uint32_t size)
    {
        fwrite(data, 1, size, fp);
    };

    cam.SetFrameCallback(cb);

    for (int i = 0; i < 100; ++i) {
        cam.Grab(200);
    }

    cam.Stop();

    fclose(fp);

    return 0;
}
