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

#include "decanstructor2/stream_view.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using DeCANstructor2::Frame;
using DeCANstructor2::FrameFormat;
using DeCANstructor2::FrameKey;
using DeCANstructor2::FrameModel;
using DeCANstructor2::IdSortDirection;
using DeCANstructor2::StreamView;

Frame frame(const char * source, uint32_t id)
{
  Frame f;
  f.key.source = source;
  f.key.id = id;
  f.length = 1;
  f.data = {1};
  return f;
}

void ordering()
{
  FrameModel model;
  StreamView view;
  auto low = frame("z", 0x2);
  auto mid = frame("b", 0x10);
  auto tie = frame("a", 0x10);
  auto high = frame("a", 0x100);
  assert(view.sort_direction() == IdSortDirection::Ascending);
  assert(view.is_visible(low.key));
  assert(view.ordered_keys(model).empty());
  assert(view.visible_keys(model).empty());
  for (const auto & f : {low, mid, tie, high}) {
    assert(model.ingest(f, 1));
  }
  assert(view.ordered_keys(model) == (std::vector<FrameKey>{low.key, tie.key, mid.key, high.key}));
  view.toggle_sort();
  assert(view.sort_direction() == IdSortDirection::Descending);
  assert(view.ordered_keys(model) == (std::vector<FrameKey>{high.key, tie.key, mid.key, low.key}));
  view.set_visible(tie.key, false);
  assert(view.visible_keys(model) == (std::vector<FrameKey>{high.key, mid.key, low.key}));
  assert(view.ordered_keys(model).size() == 4);
  view.toggle_sort();
  assert(view.sort_direction() == IdSortDirection::Ascending);
  assert(view.visible_keys(model) == (std::vector<FrameKey>{low.key, mid.key, high.key}));
}

void visibility_identity_and_bulk()
{
  FrameModel model;
  StreamView view;
  auto base = frame("can0", 42);
  auto source = base; source.key.source = "can1";
  auto extended = base; extended.key.extended = true;
  auto error = base; error.key.error = true;
  auto remote = base; remote.key.remote = true; remote.data.clear();
  auto fd = base; fd.key.format = FrameFormat::FD;
  std::vector<Frame> frames{base, source, extended, error, remote, fd};
  for (const auto & f : frames) {
    assert(model.ingest(f, 1));
  }
  assert(model.rows().size() == frames.size());
  for (const auto & hidden : frames) {
    view.set_visible(hidden.key, false);
    view.set_visible(hidden.key, false);
    for (const auto & f : frames) {
      assert(view.is_visible(f.key) == !(f.key == hidden.key));
    }
    view.set_visible(hidden.key, true);
    view.set_visible(hidden.key, true);
    assert(view.is_visible(hidden.key));
  }
  auto unknown = frame("future", 55);
  view.set_visible(unknown.key, false);
  view.set_all_visible(model, false);
  view.set_all_visible(model, false);
  assert(view.visible_keys(model).empty());
  assert(model.ingest(unknown, 2));
  assert(!view.is_visible(unknown.key));
  auto late = frame("late", 56);
  assert(model.ingest(late, 2));
  assert(view.is_visible(late.key));
  assert(view.visible_keys(model) == (std::vector<FrameKey>{late.key}));
  view.set_all_visible(model, true);
  view.set_all_visible(model, true);
  assert(view.visible_keys(model).size() == model.rows().size());
}

void hidden_reception_preserves_state()
{
  FrameModel model;
  StreamView view;
  auto f = frame("can0", 12);
  assert(model.ingest(f, 1000000));
  view.set_visible(f.key, false);
  f.data[0] = 9;
  assert(model.ingest(f, 3000000));
  assert(view.visible_keys(model).empty());
  const auto & hidden = model.rows().at(f.key);
  assert(hidden.frame.data == (std::vector<uint8_t>{9}));
  assert(hidden.count == 2);
  assert(hidden.average_interval_ms == 2.0);
  assert(hidden.changed_at_ns[0] == 3000000);
  view.set_visible(f.key, true);
  const auto & shown = model.rows().at(f.key);
  assert(shown.frame.data == (std::vector<uint8_t>{9}));
  assert(shown.count == 2);
  assert(shown.average_interval_ms == 2.0);
  assert(shown.changed_at_ns[0] == 3000000);
  assert(view.visible_keys(model) == (std::vector<FrameKey>{f.key}));
}

int main()
{
  ordering();
  visibility_identity_and_bulk();
  hidden_reception_preserves_state();
  std::cout << "stream view tests passed\n";
}
