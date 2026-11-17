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

#ifndef DECANSTRUCTOR2__MONITOR_WINDOW_HPP_
#define DECANSTRUCTOR2__MONITOR_WINDOW_HPP_

#include <array>
#include <map>
#include <optional>
#include <vector>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "decanstructor2/frame_model.hpp"
#include "decanstructor2/stream_view.hpp"

namespace DeCANstructor2
{
class MonitorWindow
{
public:
  MonitorWindow();
  bool process_events();
  void update(const FrameModel & model, uint64_t rejected, int64_t now_ns);
  void draw();
  tgui::Gui & gui() {return gui_;}

private:
  sf::RenderWindow window_;
  tgui::Gui gui_;
  tgui::ScrollablePanel::Ptr frames_;
  tgui::Label::Ptr status_;
  tgui::Panel::Ptr metadata_;
  tgui::Label::Ptr selection_hint_;
  std::array<tgui::Label::Ptr, 5> metadata_values_;
  tgui::Label::Ptr empty_;
  tgui::ScrollablePanel::Ptr choices_;
  tgui::Button::Ptr sort_;
  StreamView view_;
  std::vector<FrameKey> row_keys_;
  std::vector<FrameKey> choice_keys_;
  std::vector<float> row_offsets_;
  std::optional<FrameKey> selected_key_;
  bool initial_selection_done_ = false;
  bool projection_dirty_ = true;
  size_t known_count_ = 0;

  size_t choice_begin_ = 0;
  size_t choice_end_ = 0;
  size_t visible_begin_ = 0;
  size_t visible_end_ = 0;
  void select_row(const FrameKey & key);
  void set_known_visible(bool visible);
};
}  // namespace DeCANstructor2
#endif  // DECANSTRUCTOR2__MONITOR_WINDOW_HPP_
