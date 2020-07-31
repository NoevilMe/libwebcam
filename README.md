# libwebcam
Linux V4L2 C++ library for UVC camera

This library includes third-party spdlog 1.7.0 for logging. An example is provided, which outputs 100 images. libturbojpeg is used to rotate the frame.

Dependencies:
- sudo apt install libturbojpeg0-dev

Features:
- output jpeg/yuv directly
- adjust resolution automatically
- set fps (but it usually fails because of the factory driver)
- list controls
- rotate jpeg

TODO:
- set controls
- ...
