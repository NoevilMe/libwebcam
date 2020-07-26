#include "webcam_v4l2.h"
#include <iostream>
//#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <vector>

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");

    noevil::util::Init("cam.log");
    noevil::util::SetLevel(spdlog::level::trace);

    noevil::webcam::WebcamV4l2 cam(0);
    cam.Open();
    cam.Init();
    cam.GetControl();
    cam.SetPixFormat(noevil::webcam::kFmtMJPG, 1280, 720);

    cam.Start();

    int count = 100;
    while (count-- > 0) {
        std::cout << count << std::endl;
        std::string img;
        if (cam.Grab(img)) {
            std::vector<unsigned char> bts(img.begin(), img.end());
            cv::Mat data_byte(bts, true);
            cv::Mat image(cv::imdecode(data_byte, 1));
            cv::imshow("Display window", image);
        }
    }
    cv::waitKey(0);

    cam.Stop();

    return 0;
}
