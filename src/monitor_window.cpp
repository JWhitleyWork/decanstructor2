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
const char * field_names[] = {"source", "id", "format", "length", "count", "interval"};
const char * field_positions[] = {"1%", "17%", "31%", "39%", "82%", "90%"};
const char * field_widths[] = {"15%", "14%", "8%", "5%", "8%", "10%"};

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
  auto streams = tgui::Button::create("Streams...");
  streams->setPosition(220, 12);
  streams->setSize(105, 30);
  streams->onPress([this] {selector_->setVisible(!selector_->isVisible());});
  gui_.add(streams, "streams_toggle");

  selector_ = tgui::ChildWindow::create("Streams");
  selector_->setPosition(220, 48);
  selector_->setSize(500, 450);
  selector_->setVisible(false);
  selector_->getRenderer()->setBackgroundColor(tgui::Color(33, 36, 38));
  selector_->getRenderer()->setTitleBarColor(tgui::Color(43, 47, 50));
  selector_->getRenderer()->setTitleColor(text);
  selector_->getRenderer()->setBorderColor(tgui::Color(73, 82, 89));
  selector_->onClose([this] {selector_->setVisible(false);});
  auto show = tgui::Button::create("Show all");
  show->setPosition(8, 8);
  show->setSize(100, 30);
  show->onPress([this] {set_known_visible(true);});
  selector_->add(show, "show_all");
  auto hide = tgui::Button::create("Hide all");
  hide->setPosition(116, 8);
  hide->setSize(100, 30);
  hide->onPress([this] {set_known_visible(false);});
  selector_->add(hide, "hide_all");
  choices_ = tgui::ScrollablePanel::create();
  choices_->setPosition(8, 46);
  choices_->setSize("100% - 16", "100% - 54");
  choices_->getRenderer()->setBackgroundColor(tgui::Color(29, 32, 34));
  selector_->add(choices_, "stream_choices");

  auto columns = tgui::Panel::create();
  columns->setPosition(12, 55);
  columns->setSize("100% - 282", 29);
  columns->getRenderer()->setBackgroundColor(tgui::Color(25, 27, 29));
  auto header = tgui::Panel::create();
  header->getRenderer()->setBackgroundColor(tgui::Color(25, 27, 29));
  columns->add(header, "header");
  const char * headings[] = {"SOURCE", "ID", "FORMAT", "LEN", "COUNT", "MEAN ms"};
  for (size_t n = 0; n < 6; ++n) {
    if (n == 1) {
      sort_ = tgui::Button::create("ID ^");
      sort_->setPosition(field_positions[n], 0);
      sort_->setSize(field_widths[n], 29);
      sort_->setTextSize(12);
      sort_->onPress(
        [this] {
          view_.toggle_sort();
          projection_dirty_ = true;
          frames_->getVerticalScrollbar()->setValue(0);
        });
      header->add(sort_, "id");
      continue;
    }
    auto field = label(headings[n], 12);
    field->setPosition(field_positions[n], 6);
    field->setSize(field_widths[n], 22);
    field->getRenderer()->setTextColor(muted);
    header->add(field, field_names[n]);
  }
  auto payload_heading = label("PAYLOAD BYTES", 12);
  payload_heading->setPosition("45%", 6);
  payload_heading->getRenderer()->setTextColor(muted);
  header->add(payload_heading);
  gui_.add(columns, "columns");
  frames_ = tgui::ScrollablePanel::create();
  frames_->setPosition(12, 84);
  frames_->setSize("100% - 282", "100% - 126");
  frames_->getRenderer()->setBackgroundColor(tgui::Color(29, 32, 34));
  frames_->getHorizontalScrollbar()->setPolicy(tgui::Scrollbar::Policy::Automatic);
  auto scrollbar = tgui::Scrollbar::create();
  scrollbar->getRenderer()->setTrackColor(tgui::Color(29, 32, 34));
  scrollbar->getRenderer()->setThumbColor(tgui::Color(73, 82, 89));
  scrollbar->getRenderer()->setThumbColorHover(tgui::Color(99, 112, 122));
  scrollbar->getRenderer()->setArrowBackgroundColor(tgui::Color(29, 32, 34));
  scrollbar->getRenderer()->setArrowColor(muted);
  frames_->getRenderer()->setScrollbar(scrollbar->getRenderer()->getData());
  gui_.add(frames_, "frames");
  empty_ = label("No visible streams. Open Streams... to show them; reception continues.");
  empty_->setPosition(25, 100);
  empty_->setVisible(false);
  gui_.add(empty_, "empty_hint");

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
  gui_.add(selector_, "stream_selector");
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

void MonitorWindow::select_row(const FrameKey & key)
{
  selected_key_ = key;
}

void MonitorWindow::set_known_visible(bool visible)
{
  for (const auto & key : choice_keys_) {
    view_.set_visible(key, visible);
  }
  projection_dirty_ = true;
  frames_->getVerticalScrollbar()->setValue(0);
}

void MonitorWindow::update(const FrameModel & model, uint64_t rejected, int64_t now_ns)
{
  window_.setTitle("DeCANstructor2 - " + std::to_string(model.rows().size()) + " streams");
  const bool rebuild = projection_dirty_ || known_count_ != model.rows().size();
  if (rebuild) {
    row_keys_ = view_.visible_keys(model);
    choice_keys_ = view_.ordered_keys(model);
    row_offsets_.clear();
    float offset = 0;
    for (const auto & key : row_keys_) {
      row_offsets_.push_back(offset);
      offset += key.format == FrameFormat::FD ? fd_height : classic_height;
    }
    if (selected_key_ && !view_.is_visible(*selected_key_)) {
      selected_key_.reset();
    }
    if (!initial_selection_done_ && !row_keys_.empty()) {
      selected_key_ = row_keys_.front();
      initial_selection_done_ = true;
    }
    known_count_ = model.rows().size();
    projection_dirty_ = false;
    sort_->setText(view_.sort_direction() == IdSortDirection::Ascending ? "ID ^" : "ID v");
  }
  const auto status_text = "Streams: " + std::to_string(row_keys_.size()) + "/" +
    std::to_string(model.rows().size()) +
    "    |    Rejected: " + std::to_string(rejected) +
    "    |    Receipt interval (steady clock)    |    Amber: changed within 1s";
  status_->setText(status_text);
  empty_->setVisible(row_keys_.empty() && !model.rows().empty());
  constexpr float choice_height = 32;
  choices_->setContentSize(
    {std::max(1.f, choices_->getSize().x - 20),
      std::max(1.f, choice_height * choice_keys_.size())});
  const auto choice_scroll = choices_->getVerticalScrollbar()->getValue();
  const size_t cb = std::min(
    choice_keys_.size(),
    static_cast<size_t>(choice_scroll / choice_height));
  const size_t ce = std::min(
    choice_keys_.size(), cb +
    static_cast<size_t>(choices_->getSize().y / choice_height) + 2);
  if (rebuild || cb != choice_begin_ || ce != choice_end_) {
    choices_->removeAllWidgets();
    choice_begin_ = cb;
    choice_end_ = ce;
    for (size_t index = cb; index < ce; ++index) {
      const auto key = choice_keys_[index];
      const auto caption = "0x" + hex(key.id, key.extended || key.error ? 8 : 3) +
        "  " + (key.format == FrameFormat::FD ? "FD" : "CAN") + "  " + flags(key) +
        "  " + key.source;
      auto choice = tgui::CheckBox::create(caption);
      choice->setPosition(5, index * choice_height + 4);
      choice->setSize(22, 22);
      choice->setTextSize(14);
      choice->setChecked(view_.is_visible(key));
      choice->getRenderer()->setTextColor(text);
      choice->getRenderer()->setTextColorHover(text);
      auto tooltip = label(caption);
      tooltip->getRenderer()->setBackgroundColor(cell);
      choice->setToolTip(tooltip);
      choice->onChange(
        [this, key](bool checked) {
          view_.set_visible(key, checked);
          projection_dirty_ = true;
          frames_->getVerticalScrollbar()->setValue(0);
        });
      choices_->add(choice, "choice" + std::to_string(index));
    }
  }
  const float total = row_keys_.empty() ? 0 : row_offsets_.back() +
    (row_keys_.back().format == FrameFormat::FD ? fd_height : classic_height);
  const float content_width = std::max(680.f, frames_->getSize().x - 20);
  frames_->setContentSize({content_width, std::max(1.f, total)});
  auto header = gui_.get<tgui::Panel>("columns")->get<tgui::Panel>("header");
  header->setSize(content_width, 29);
  header->setPosition(-static_cast<float>(frames_->getHorizontalScrollbar()->getValue()), 0);
  const float scroll = static_cast<float>(frames_->getVerticalScrollbar()->getValue());
  auto start = std::upper_bound(row_offsets_.begin(), row_offsets_.end(), scroll);
  const size_t begin = start == row_offsets_.begin() ? 0 :
    static_cast<size_t>(start - row_offsets_.begin() - 1);
  auto finish = std::lower_bound(
    row_offsets_.begin(), row_offsets_.end(), scroll + frames_->getSize().y);
  size_t end = std::min(row_keys_.size(), static_cast<size_t>(finish - row_offsets_.begin()) + 1);
  if (rebuild || begin != visible_begin_ || end != visible_end_) {
    frames_->removeAllWidgets();
    visible_begin_ = begin;
    visible_end_ = end;
    for (size_t index = begin; index < end; ++index) {
      const bool fd = row_keys_[index].format == FrameFormat::FD;
      auto row = tgui::Panel::create();
      row->setPosition(0, row_offsets_[index]);
      row->setSize(content_width, fd ? fd_height : classic_height);
      const auto key = row_keys_[index];
      row->onMousePress([this, key] {select_row(key);});
      frames_->add(row, "row" + std::to_string(index));
      for (size_t n = 0; n < 6; ++n) {
        auto field = label("");
        field->setPosition(field_positions[n], 7);
        field->setSize(field_widths[n], 22);
        row->add(field, field_names[n]);
      }
      if (fd) {
        for (size_t i = 0; i < 8; ++i) {
          auto offset = label(hex(i * 8, 2), 11);
          offset->setPosition("43%", static_cast<float>(7 + i * 23));
          offset->getRenderer()->setTextColor(muted);
          row->add(offset);
        }
      }
      const size_t bytes = fd ? 64 : 8;
      for (size_t i = 0; i < bytes; ++i) {
        auto byte = label("", 14);
        const auto x = tgui::Layout{tgui::String{std::to_string(45 + (i % 8) * 4.4) + "%"}};
        const auto y = static_cast<float>(fd ? 4 + (i / 8) * 23 : 5);
        byte->setPosition(x, y);
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
    row->setSize(content_width, frame.key.format == FrameFormat::FD ? fd_height : classic_height);
    const auto row_background = selected_key_ && row_keys_[index] == *selected_key_ ?
      tgui::Color(41, 56, 67) : tgui::Color(33, 36, 38);
    row->getRenderer()->setBackgroundColor(row_background);
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
  if (selected_key_) {
    const auto & state = model.rows().at(*selected_key_);
    const auto & frame = state.frame;
    const auto id_text = hex(frame.key.id, frame.key.extended || frame.key.error ? 8 : 3);
    const auto metadata = "ID\n0x" + id_text +
      "\n\nTOPIC\n" + frame.key.source + "\n\nFORMAT\n" +
      (frame.key.format == FrameFormat::FD ? "CAN FD" : "Classic CAN") + "\n\nFLAGS\n" +
      flags(frame.key) + "\n\nBRS / ESI\nUnavailable";
    selected_->setText(metadata);
  } else {
    selected_->setText("Select a frame");
  }
}

void MonitorWindow::draw()
{
  window_.clear(sf::Color(25, 27, 29));
  gui_.draw();
  window_.display();
}
}  // namespace DeCANstructor2
