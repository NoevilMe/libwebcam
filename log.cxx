/**
 * Copyright(C)2020 NoevilMe. All rights reserved.
 * File              : log.cxx
 * Author            : NoevilMe <surpass168@live.com>
 * Date              : 2020-04-23 19:05:39
 * Last Modified Date: 2020-07-25 22:08:37
 * Last Modified By  : NoevilMe <surpass168@live.com>
 */
/**
 * /home/pi/libwebcam/log.cxx
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
#include "log.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <iostream>
#include <string>

namespace noevil {
namespace util {

static std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> log_sink_ =
    nullptr;
static spdlog::level::level_enum level_ = spdlog::level::info;

void Init(const std::string &log_path, int size, int count) {
    if (!log_sink_) {
        log_sink_ = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_path, size, count);
    }
}

void SetLevel(spdlog::level::level_enum level) {
    level_ = level;
    // this only effects existing loggers
    spdlog::set_level(level);
}

Logger GetLogger(const std::string &name) {
    if (!log_sink_)
        return nullptr;

    auto logger = spdlog::get(name);
    if (!logger) {
        logger = std::make_shared<spdlog::logger>(name, log_sink_);
        logger->set_pattern("%Y-%m-%d %T.%e %P.%t [%l] %n - %v");
        logger->set_level(level_);
        logger->flush_on(level_);
        spdlog::register_logger(logger);
    }
    return logger;
}

} // namespace util
} // namespace noevil
