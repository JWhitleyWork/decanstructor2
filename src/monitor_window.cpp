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

#include "decanstructor2/monitor_window.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace DeCANstructor2
{
namespace
{
std::string hex(uint32_t value, int width)
{
  std::ostringstream stream;
  stream << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
  return stream.str();
}

std::vector<tgui::String> row_text(const MessageState & state)
{
  const auto & frame = state.frame;
  std::string flags = frame.key.extended ? "EXT" : "STD";
  if (frame.key.error) {flags += " ERR";}
  if (frame.key.remote) {flags += " RTR";}
  std::ostringstream interval;
  if (state.average_interval_ms) {
    interval << std::fixed << std::setprecision(3) << *state.average_interval_ms;
  } else {
    interval << "-";
  }
  std::string preview;
  for (size_t i = 0; i < std::min<size_t>(8, frame.data.size()); ++i) {
    if (i) {preview += " ";}
    preview += hex(frame.data[i], 2);
  }
  if (frame.data.size() > 8) {preview += " ... (+" + std::to_string(frame.data.size() - 8) + ")";}
  if (frame.key.remote) {preview = "RTR (no payload)";}
  return {frame.key.source, "0x" + hex(frame.key.id, frame.key.extended || frame.key.error ? 8 : 3),
    frame.key.format == FrameFormat::FD ? "FD" : "CAN", flags,
    std::to_string(frame.length), std::to_string(state.count), interval.str(), preview};
}
}  // namespace

MonitorWindow::MonitorWindow()
: window_(sf::VideoMode(1280, 800), "DeCANstructor2"), gui_(window_)
{
  window_.setFramerateLimit(30);
  auto title = tgui::Label::create("DeCANstructor2  |  CAN + CAN FD  |  Receive only");
  title->setPosition(10, 10);
  title->setTextSize(22);
  gui_.add(title);
  status_ = tgui::Label::create();
  status_->setPosition(10, 40);
  status_->setTextSize(15);
  gui_.add(status_, "status");

  frames_ = tgui::ListView::create();
  frames_->setPosition(10, 70);
  frames_->setSize("100% - 20", "45%");
  frames_->setItemHeight(26);
  frames_->setTextSize(16);
  frames_->addColumn("Source", 230);
  frames_->addColumn("ID", 110);
  frames_->addColumn("Format", 65);
  frames_->addColumn("Flags", 110);
  frames_->addColumn("Bytes", 55);
  frames_->addColumn("Count", 85);
  frames_->addColumn("Mean dt (ms)", 120);
  frames_->addColumn("Payload preview", 340);
  gui_.add(frames_, "frames");

  selected_ =
    tgui::Label::create("Select a frame to inspect its payload (byte offsets are hexadecimal)");
  selected_->setPosition(10, "45% + 90");
  selected_->setTextSize(17);
  gui_.add(selected_, "selected");
  for (size_t i = 0; i < bytes_.size(); ++i) {
    auto label = tgui::Label::create(hex(i, 2) + ": --");
    label->setPosition(
      tgui::Layout{tgui::String{std::to_string(1 + (i % 16) * 6) + "%"}},
      tgui::Layout{tgui::String{"45% + " + std::to_string(140 + (i / 16) * 42)}});
    label->setSize("5.6%", 34);
    label->setTextSize(16);
    label->getRenderer()->setBackgroundColor(tgui::Color::White);
    bytes_[i] = label;
    gui_.add(label, "byte" + std::to_string(i));
  }
  auto help = tgui::Label::create(
    "Yellow = changed in last second. Timing uses steady-clock receipt intervals, "
    "not bus timestamps.\n"
    "FD message exposes no BRS/ESI flags. Physical CAN access is provided by your ROS CAN driver.");
  help->setPosition(10, "100% - 58");
  help->setTextSize(14);
  gui_.add(help);
}

bool MonitorWindow::process_events()
{
  sf::Event event;
  while (window_.pollEvent(event)) {
    gui_.handleEvent(event);
    if (event.type == sf::Event::Closed ||
      (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
    {
      window_.close();
    }
  }
  return window_.isOpen();
}

void MonitorWindow::update(const FrameModel & model, uint64_t rejected, int64_t now_ns)
{
  window_.setTitle("DeCANstructor2 - " + std::to_string(model.rows().size()) + " streams");
  status_->setText("Streams: " + std::to_string(model.rows().size()) +
    "   |   Rejected: " + std::to_string(rejected) +
      "   |   Waiting for / receiving ROS CAN topics");
  for (const auto & entry : model.rows()) {
    auto index = row_indices_.find(entry.first);
    if (index == row_indices_.end()) {
      const auto new_index = frames_->addItem(row_text(entry.second));
      row_indices_.emplace(entry.first, new_index);
      row_keys_.push_back(entry.first);
    } else {
      frames_->changeItem(index->second, row_text(entry.second));
    }
  }
  if (frames_->getSelectedItemIndex() < 0 && !row_keys_.empty()) {frames_->setSelectedItem(0);}
  const auto index = frames_->getSelectedItemIndex();
  if (index < 0 || static_cast<size_t>(index) >= row_keys_.size()) {return;}
  const auto & state = model.rows().at(row_keys_[index]);
  const auto & frame = state.frame;
  selected_->setText(frame.key.source + "  |  ID 0x" + hex(frame.key.id,
      frame.key.extended ? 8 : 3) +
    "  |  " + std::to_string(frame.length) + " bytes" +
    (frame.key.remote ? " requested (RTR, no payload)" : ""));
  for (size_t i = 0; i < bytes_.size(); ++i) {
    const bool present = i < frame.data.size();
    const auto changed_at = state.changed_at_ns[i];
    const bool changed = present && changed_at && now_ns >= *changed_at &&
      now_ns - *changed_at < 1000000000;
    bytes_[i]->setText(hex(i, 2) + ": " + (present ? hex(frame.data[i], 2) : "--"));
    bytes_[i]->getRenderer()->setTextColor(present ? tgui::Color::Black : tgui::Color(125, 125,
        125));
    bytes_[i]->getRenderer()->setBackgroundColor(changed ? tgui::Color(255, 220,
        100) : tgui::Color::White);
  }
}

void MonitorWindow::draw()
{
  window_.clear(sf::Color(245, 245, 245));
  gui_.draw();
  window_.display();
}
}  // namespace DeCANstructor2
