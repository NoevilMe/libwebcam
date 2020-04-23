#include  "webcam_v4l2.h"

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    util::Init("cam.log");
    util::SetLevel(spdlog::level::trace);

    WebcamV4l2 cam(0);
    cam.Open();
    cam.Init();
    cam.StreamOn();
    cam.Grap();
    cam.StreamOff();

    return 0;
}
