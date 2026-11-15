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

#include <algorithm>
#include <set>
#include <vector>

#include "decanstructor2/frame_model.hpp"

namespace DeCANstructor2
{
enum class IdSortDirection { Ascending, Descending };

class StreamView
{
public:
  bool is_visible(const FrameKey & key) const {return hidden_.count(key) == 0;}
  void set_visible(const FrameKey & key, bool visible)
  {
    if (visible) {hidden_.erase(key);} else {hidden_.insert(key);}
  }
  void set_all_visible(const FrameModel & model, bool visible)
  {
    for (const auto & row : model.rows()) {set_visible(row.first, visible);}
  }
  IdSortDirection sort_direction() const {return direction_;}
  void toggle_sort()
  {
    direction_ = direction_ == IdSortDirection::Ascending ?
      IdSortDirection::Descending : IdSortDirection::Ascending;
  }
  std::vector<FrameKey> ordered_keys(const FrameModel & model) const
  {
    std::vector<FrameKey> keys;
    keys.reserve(model.rows().size());
    for (const auto & row : model.rows()) {keys.push_back(row.first);}
    std::sort(keys.begin(), keys.end(), [this](const FrameKey & a, const FrameKey & b) {
        if (a.id == b.id) {return a < b;}
        return direction_ == IdSortDirection::Ascending ? a.id < b.id : a.id > b.id;
      });
    return keys;
  }
  std::vector<FrameKey> visible_keys(const FrameModel & model) const
  {
    auto keys = ordered_keys(model);
    keys.erase(std::remove_if(keys.begin(), keys.end(), [this](const FrameKey & key) {
          return !is_visible(key);
        }), keys.end());
    return keys;
  }

private:
  std::set<FrameKey> hidden_;
  IdSortDirection direction_ = IdSortDirection::Ascending;
};
}  // namespace DeCANstructor2
