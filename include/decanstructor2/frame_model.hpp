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

#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace DeCANstructor2
{
enum class FrameFormat { Classic, FD };
struct FrameKey
{
  std::string source;
  uint32_t id = 0;
  bool extended = false;
  bool error = false;
  bool remote = false;
  FrameFormat format = FrameFormat::Classic;
  bool operator<(const FrameKey & other) const;
  bool operator==(const FrameKey & other) const;
};
struct Frame
{
  FrameKey key;
  uint8_t length = 0;
  std::vector<uint8_t> data;
};
struct MessageState
{
  Frame frame;
  uint64_t count = 0;
  std::optional<double> average_interval_ms;
  std::array<std::optional<int64_t>, 64> changed_at_ns{};
};
class FrameModel {
public:
  explicit FrameModel(std::size_t capacity = 4096)
  : capacity_(capacity) {}
  bool ingest(const Frame & frame, int64_t received_ns);
  const std::map<FrameKey, MessageState> & rows() const {return rows_;}

private:
  struct Timing
  {
    int64_t last_ns = 0;
    uint64_t intervals = 0;
    long double mean_ms = 0;
  };
  std::size_t capacity_;
  std::map<FrameKey, MessageState> rows_;
  std::map<FrameKey, Timing> timing_;
};
}  // namespace DeCANstructor2
