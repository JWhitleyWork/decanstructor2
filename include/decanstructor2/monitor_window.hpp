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
#include <vector>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "decanstructor2/frame_model.hpp"

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
  tgui::ListView::Ptr frames_;
  tgui::Label::Ptr status_;
  tgui::Label::Ptr selected_;
  std::array<tgui::Label::Ptr, 64> bytes_;
  std::map<FrameKey, size_t> row_indices_;
  std::vector<FrameKey> row_keys_;
};
}  // namespace DeCANstructor2
#endif  // DECANSTRUCTOR2__MONITOR_WINDOW_HPP_
