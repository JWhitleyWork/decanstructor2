// Copyright 2026 Joshua Whitley
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "decanstructor2/frame_model.hpp"
#include <tuple>

namespace DeCANstructor2
{
bool FrameKey::operator<(const FrameKey & other) const
{
  return std::tie(source, id, extended, error, remote, format) <
         std::tie(other.source, other.id, other.extended, other.error, other.remote, other.format);
}
bool FrameKey::operator==(const FrameKey & other) const
{
  return std::tie(source, id, extended, error, remote, format) ==
         std::tie(other.source, other.id, other.extended, other.error, other.remote, other.format);
}
bool FrameModel::ingest(const Frame & frame, int64_t received_ns)
{
  const auto & key = frame.key;
  const bool fd = key.format == FrameFormat::FD;
  if ((key.format != FrameFormat::Classic && !fd) ||
    frame.length > (fd ? 64 : 8) || (fd && key.remote) ||
    key.id > ((key.extended || key.error) ? 0x1fffffffU : 0x7ffU) ||
    (key.remote ? !frame.data.empty() : frame.data.size() != frame.length))
  {
    return false;
  }

  if (rows_.find(key) == rows_.end() && rows_.size() >= capacity_) {
    return false;
  }
  auto [it, inserted] = rows_.try_emplace(key);
  auto & state = it->second;
  auto & timing = timing_[key];
  if (!inserted) {
    for (std::size_t i = 0; i < frame.data.size(); ++i) {
      if (i >= state.frame.data.size() || frame.data[i] != state.frame.data[i]) {
        state.changed_at_ns[i] = received_ns;
      }
    }
    for (std::size_t i = frame.data.size(); i < state.changed_at_ns.size(); ++i) {
      state.changed_at_ns[i].reset();
    }
    if (received_ns <= timing.last_ns) {
      timing.intervals = 0;
      timing.mean_ms = 0;
      state.average_interval_ms.reset();
    } else {
      const long double interval = (static_cast<long double>(received_ns) -
        static_cast<long double>(timing.last_ns)) / 1000000.0L;
      ++timing.intervals;
      timing.mean_ms += (interval - timing.mean_ms) / timing.intervals;
      state.average_interval_ms = static_cast<double>(timing.mean_ms);
    }
  }
  timing.last_ns = received_ns;
  state.frame = frame;
  ++state.count;
  return true;
}
}  // namespace DeCANstructor2
