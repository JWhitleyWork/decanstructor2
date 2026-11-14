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
const tgui::Color text(229, 231, 230), muted(145, 154, 161), cell(43, 47, 50);
const tgui::Color absent(101, 108, 112), amber(223, 163, 61);
constexpr float classic_height = 32, fd_height = 190;

sf::VideoMode monitor_video_mode()
{
#if SFML_VERSION_MAJOR >= 3
  return sf::VideoMode(sf::Vector2u{1280, 800});
#else
  return sf::VideoMode(1280, 800);
#endif
}
std::string hex(uint32_t value, int width)
{
  std::ostringstream stream;
  stream << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
  return stream.str();
}
tgui::Label::Ptr label(const std::string & value, unsigned int size = 14)
{
  auto result = tgui::Label::create(value);
  result->setTextSize(size);
  result->getRenderer()->setTextColor(text);
  return result;
}
std::string flags(const FrameKey & key)
{
  return std::string(key.extended ? "EXT" : "STD") + (key.error ? "  ERR" : "") +
         (key.remote ? "  RTR" : "");
}
}  // namespace

MonitorWindow::MonitorWindow()
: window_(monitor_video_mode(), "DeCANstructor2"), gui_(window_)
{
  window_.setFramerateLimit(30);
  auto title = label("DeCANstructor2", 23);
  title->setPosition(18, 12);
  gui_.add(title);
  auto mode = label("LIVE  /  RECEIVE ONLY", 14);
  mode->getRenderer()->setTextColor(muted);
  mode->setPosition("100% - 220", 18);
  gui_.add(mode);

  auto header = label(
    "SOURCE                  ID             FORMAT    LENGTH         PAYLOAD BYTES"
    "                          COUNT        MEAN dt (ms)", 13);
  header->getRenderer()->setTextColor(muted);
  header->setPosition(20, 61);
  gui_.add(header);
  frames_ = tgui::ScrollablePanel::create();
  frames_->setPosition(12, 84);
  frames_->setSize("100% - 282", "100% - 126");
  frames_->getRenderer()->setBackgroundColor(tgui::Color(29, 32, 34));
  frames_->getHorizontalScrollbar()->setPolicy(tgui::Scrollbar::Policy::Never);
  gui_.add(frames_, "frames");

  auto detail_title = label("FRAME METADATA", 15);
  detail_title->setPosition("100% - 252", 64);
  detail_title->getRenderer()->setTextColor(muted);
  gui_.add(detail_title);
  selected_ = label("Select a frame", 14);
  selected_->setPosition("100% - 252", 99);
  selected_->setSize(240, "100% - 150");
  gui_.add(selected_, "selected");
  status_ = label("", 13);
  status_->setPosition(18, "100% - 29");
  status_->getRenderer()->setTextColor(muted);
  gui_.add(status_, "status");
}

bool MonitorWindow::process_events()
{
#if SFML_VERSION_MAJOR >= 3
  while (const auto event = window_.pollEvent()) {
    gui_.handleEvent(*event);
    const auto * key = event->getIf<sf::Event::KeyPressed>();
    if (event->is<sf::Event::Closed>() || (key && key->code == sf::Keyboard::Key::Escape)) {
      window_.close();
    }
  }
#else
  sf::Event event;
  while (window_.pollEvent(event)) {
    gui_.handleEvent(event);
    if (event.type == sf::Event::Closed ||
      (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Key::Escape))
    {
      window_.close();
    }
  }
#endif
  return window_.isOpen();
}

void MonitorWindow::select_row(size_t index)
{
  selected_index_ = index;
}

void MonitorWindow::update(const FrameModel & model, uint64_t rejected, int64_t now_ns)
{
  window_.setTitle("DeCANstructor2 - " + std::to_string(model.rows().size()) + " streams");
  status_->setText("Streams: " + std::to_string(model.rows().size()) +
    "    |    Rejected: " + std::to_string(rejected) +
    "    |    Receipt interval (steady clock)    |    Amber: changed within 1s");
  for (const auto & entry : model.rows()) {
    if (row_indices_.find(entry.first) == row_indices_.end()) {
      row_indices_.emplace(entry.first, row_keys_.size());
      row_keys_.push_back(entry.first);
      row_offsets_.push_back(row_offsets_.empty() ? 0 :
        row_offsets_.back() + (row_keys_[row_keys_.size() - 2].format == FrameFormat::FD ?
        fd_height : classic_height));
    }
  }
  const float total = row_keys_.empty() ? 0 : row_offsets_.back() +
    (row_keys_.back().format == FrameFormat::FD ? fd_height : classic_height);
  frames_->setContentSize({std::max(1.f, frames_->getSize().x - 20), std::max(1.f, total)});
  const float scroll = static_cast<float>(frames_->getVerticalScrollbar()->getValue());
  auto start = std::upper_bound(row_offsets_.begin(), row_offsets_.end(), scroll);
  const size_t begin = start == row_offsets_.begin() ? 0 :
    static_cast<size_t>(start - row_offsets_.begin() - 1);
  auto finish = std::lower_bound(
    row_offsets_.begin(), row_offsets_.end(), scroll + frames_->getSize().y);
  size_t end = std::min(row_keys_.size(), static_cast<size_t>(finish - row_offsets_.begin()) + 1);
  if (begin != visible_begin_ || end != visible_end_) {
    frames_->removeAllWidgets();
    visible_begin_ = begin;
    visible_end_ = end;
    for (size_t index = begin; index < end; ++index) {
      const bool fd = row_keys_[index].format == FrameFormat::FD;
      auto row = tgui::Panel::create();
      row->setPosition(0, row_offsets_[index]);
      row->setSize("100%", fd ? fd_height : classic_height);
      row->onMousePress([this, index] {select_row(index);});
      frames_->add(row, "row" + std::to_string(index));
      const char * names[] = {"source", "id", "format", "length", "count", "interval"};
      const char * positions[] = {"1%", "21%", "32%", "39%", "82%", "90%"};
      const char * widths[] = {"19%", "10%", "7%", "5%", "8%", "10%"};
      for (size_t n = 0; n < 6; ++n) {
        auto field = label("");
        field->setPosition(positions[n], 7);
        field->setSize(widths[n], 22);
        row->add(field, names[n]);
      }
      const size_t bytes = fd ? 64 : 8;
      for (size_t i = 0; i < bytes; ++i) {
        auto byte = label("", 14);
        byte->setPosition(tgui::Layout{tgui::String{std::to_string(45 + (i % 8) * 4.4) + "%"}},
          static_cast<float>(fd ? 4 + (i / 8) * 23 : 5));
        byte->setSize("4%", 20);
        byte->setHorizontalAlignment(tgui::HorizontalAlignment::Center);
        row->add(byte, "byte" + std::to_string(i));
      }
    }
  }
  for (size_t index = begin; index < end; ++index) {
    const auto & state = model.rows().at(row_keys_[index]);
    const auto & frame = state.frame;
    auto row = frames_->get<tgui::Panel>("row" + std::to_string(index));
    row->getRenderer()->setBackgroundColor(index == selected_index_ ?
      tgui::Color(41, 56, 67) : tgui::Color(33, 36, 38));
    row->get<tgui::Label>("source")->setText(frame.key.source);
    row->get<tgui::Label>("id")->setText(
      "0x" + hex(frame.key.id, frame.key.extended || frame.key.error ? 8 : 3));
    row->get<tgui::Label>("format")->setText(frame.key.format == FrameFormat::FD ? "FD" : "CAN");
    row->get<tgui::Label>("length")->setText(std::to_string(frame.length));
    row->get<tgui::Label>("count")->setText(std::to_string(state.count));
    std::ostringstream interval;
    if (state.average_interval_ms) {
      interval << std::fixed << std::setprecision(3) << *state.average_interval_ms;
    } else {
      interval << "--";
    }
    row->get<tgui::Label>("interval")->setText(interval.str());
    const size_t bytes = frame.key.format == FrameFormat::FD ? 64 : 8;
    for (size_t i = 0; i < bytes; ++i) {
      auto byte = row->get<tgui::Label>("byte" + std::to_string(i));
      const bool present = i < frame.data.size();
      const auto changed_at = state.changed_at_ns[i];
      const bool changed = present && changed_at && now_ns >= *changed_at &&
        static_cast<uint64_t>(now_ns) - static_cast<uint64_t>(*changed_at) < 1000000000ULL;
      byte->setText(present ? hex(frame.data[i], 2) : "--");
      const auto foreground = changed ? tgui::Color(25, 25, 25) : (present ? text : absent);
      byte->getRenderer()->setTextColor(foreground);
      byte->getRenderer()->setBackgroundColor(changed ? amber : cell);
    }
  }
  if (!row_keys_.empty()) {
    const auto & state = model.rows().at(row_keys_[selected_index_]);
    const auto & frame = state.frame;
    selected_->setText("ID\n0x" + hex(frame.key.id, frame.key.extended || frame.key.error ? 8 : 3) +
      "\n\nTOPIC\n" + frame.key.source + "\n\nFORMAT\n" +
      (frame.key.format == FrameFormat::FD ? "CAN FD" : "Classic CAN") + "\n\nFLAGS\n" +
      flags(frame.key) + "\n\nBRS / ESI\nUnavailable");
  }
}

void MonitorWindow::draw()
{
  window_.clear(sf::Color(25, 27, 29));
  gui_.draw();
  window_.display();
}
}  // namespace DeCANstructor2
