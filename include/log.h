/**
 * Copyright(C)2020 NoevilMe. All rights reserved.
 * File              : log.h
 * Author            : NoevilMe <surpass168@live.com>
 * Date              : 2020-04-23 19:04:23
 * Last Modified Date: 2020-07-25 22:03:50
 * Last Modified By  : NoevilMe <surpass168@live.com>
 */
/**
 * log.h
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

#ifndef __LOG_H_
#define __LOG_H_

#define SPDLOG_PREVENT_CHILD_FD

#include "spdlog/spdlog.h"

namespace noevil {
namespace util {

using Logger = std::shared_ptr<spdlog::logger>;

// create logger sink
void Init(const std::string &log_path, int size = 1024 * 1024 * 10,
          int count = 10);

void SetLevel(spdlog::level::level_enum level);

// create or get looger
Logger GetLogger(const std::string &name);

} // namespace util
} // namespace noevil

#endif /* __LOG_H_ */
