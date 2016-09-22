/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/cli/benchmark.h"
#include "eventql/util/wallclock.h"

/**
 * TODO:
 *   - print benchmark stats
 *   - Benchmark::connect()
 *   - benchmark stats: moving average
 *   -----
 *   - Benchmark::runRequest()
 *   - improve thread load balancing/scheudling
 */

namespace eventql {
namespace cli {

// FIXME pass proper arguments
Benchmark::Benchmark(
    size_t num_threads,
    size_t rate,
    size_t remaining_requests /* = size_t(-1) */) :
    num_threads_(num_threads),
    rate_limit_interval_(0),
    remaining_requests_(remaining_requests),
    status_(ReturnCode::success()),
    threads_running_(0),
    last_request_time_(0) {
  if (rate > 0 && rate < kMicrosPerSecond) {
    rate_limit_interval_ = kMicrosPerSecond / rate;
  }

  threads_.resize(num_threads_);
}

void Benchmark::setRequestHandler(std::function<ReturnCode ()> handler) {
  request_handler_ = handler;
}

void Benchmark::setProgressCallback(std::function<void ()> cb) {
  on_progress_ = cb;
}

ReturnCode Benchmark::run() {
  if (!request_handler_) {
    return ReturnCode::error("ERUNTIME", "no request handler set");
  }

  for (size_t i = 0; i < num_threads_; ++i) {
    threads_[i] = std::thread(std::bind(&Benchmark::runThread, this, i));
    ++threads_running_;
  }

  {
    std::unique_lock<std::mutex> lk(mutex_);
    while (threads_running_ > 0) {
      cv_.wait_for(lk, std::chrono::microseconds(kMicrosPerSecond / 10)); // FIXME
      if (on_progress_) {
        on_progress_(); //FIXME pass BenchmarkStats
      }
      // FIXME rate limit progress callback?
    }
  }

  for (auto& t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }

  return status_;
}

void Benchmark::kill() {
  std::unique_lock<std::mutex> lk(mutex_);
  status_ = ReturnCode::error("ERUNTIME", "Benchmark aborted...");
  cv_.notify_all();
}

const BenchmarkStats* Benchmark::getStats() const {
  return &stats_;
}

void Benchmark::runThread(size_t idx) {
  while (getRequestSlot(idx)) {
    stats_.addRequestStart();

    auto rc = ReturnCode::success();
    auto t0 = MonotonicClock::now();
    try {
      rc = request_handler_();
    } catch (const std::exception& e) {
      rc = ReturnCode::error("ERUNTIME", e.what());
    }
    auto t1 = MonotonicClock::now();

    std::unique_lock<std::mutex> lk(mutex_);
    stats_.addRequestComplete(rc.isSuccess(), t1 - t0);

    if (!rc.isSuccess()) {
      status_ = rc;
      cv_.notify_all();
      break;
    }
  }

  std::unique_lock<std::mutex> lk(mutex_);
  --threads_running_;
  cv_.notify_all();
}

// FIXME: this will "randomly" (not really) select one of our threads to run
// the next request. we should improve this to do some kind of best-effort
// round robin/load balancing
bool Benchmark::getRequestSlot(size_t idx) {
  std::unique_lock<std::mutex> lk(mutex_);
  for (;;) {
    if (!status_.isSuccess()) {
      return false;
    }

    if (remaining_requests_ == 0) {
      return false;
    }

    if (rate_limit_interval_ == 0) {
      return true;
    }

    auto now = MonotonicClock::now();
    if (last_request_time_ + rate_limit_interval_ <= now) {
      last_request_time_ = now;
      if (remaining_requests_ != size_t(-1)) {
        --remaining_requests_;
      }
      break;
    }

    auto wait = (last_request_time_ + rate_limit_interval_) - now;
    cv_.wait_for(lk, std::chrono::microseconds(wait));
  }

  return true;
}

} //cli
} //eventql
