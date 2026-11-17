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
#include <gtest/gtest.h>
#include <memory>
#include "decanstructor2/monitor_window.hpp"

namespace
{
tgui::Color background(const tgui::Label::Ptr & byte)
{
  return byte->getRenderer()->getBackgroundColor();
}
tgui::String metadata_text(DeCANstructor2::MonitorWindow & window)
{
  const auto panel = window.gui().get<tgui::Panel>("metadata");
  EXPECT_TRUE(panel);
  if (!panel) {return {};}
  const auto hint = panel->get<tgui::Label>("selection_hint");
  EXPECT_TRUE(hint);
  if (!hint) {return {};}
  if (hint->isVisible()) {return hint->getText();}
  tgui::String text;
  for (const auto name : {"id", "source", "format", "flags", "brs_esi"}) {
    const auto field = panel->get<tgui::Label>(name);
    EXPECT_TRUE(field);
    if (field) {text += field->getText() + " ";}
  }
  return text;
}
}  // namespace

TEST(MonitorWindow, UnselectedFdRowsExposeAllBytesAndExpireHighlights)
{
  using DeCANstructor2::Frame;
  using DeCANstructor2::FrameFormat;
  using DeCANstructor2::FrameModel;
  using DeCANstructor2::MonitorWindow;
  FrameModel model;
  MonitorWindow window;
  Frame frame;
  frame.key.source = "/fd1";
  frame.key.format = FrameFormat::FD;
  frame.length = 64;
  frame.data.resize(64, 0xAA);
  ASSERT_TRUE(model.ingest(frame, 100));
  frame.key.source = "/fd2";
  frame.data[63] = 0xBC;
  ASSERT_TRUE(model.ingest(frame, 100));
  frame.key.source = "/fd3";
  frame.data[63] = 0xDE;
  ASSERT_TRUE(model.ingest(frame, 100));
  window.update(model, 0, 100);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  ASSERT_TRUE(table);
  auto first = table->get<tgui::Panel>("row0");
  auto second = table->get<tgui::Panel>("row1");
  auto third = table->get<tgui::Panel>("row2");
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  ASSERT_TRUE(third);
  auto first_last = first->get<tgui::Label>("byte63");
  auto last = second->get<tgui::Label>("byte63");
  auto third_last = third->get<tgui::Label>("byte63");
  ASSERT_TRUE(first_last);
  ASSERT_TRUE(last);
  ASSERT_TRUE(third_last);
  EXPECT_EQ(last->getText(), tgui::String("BC"));
  EXPECT_EQ(third_last->getText(), tgui::String("DE"));
  EXPECT_EQ(first_last->getText(), tgui::String("AA"));
  window.gui().getContainer()->leftMousePressed(
    third_last->getAbsolutePosition() + tgui::Vector2f{4, 4});
  window.update(model, 0, 100);
  const auto selected_text = metadata_text(window);
  EXPECT_NE(selected_text.find("/fd3"), tgui::String::npos);
  frame.data[63] = 0xEF;
  ASSERT_TRUE(model.ingest(frame, 200));
  frame.key.source = "/fd2";
  frame.data[63] = 0xF0;
  ASSERT_TRUE(model.ingest(frame, 200));
  window.update(model, 0, 200);
  EXPECT_EQ(last->getText(), tgui::String("F0"));
  EXPECT_EQ(third_last->getText(), tgui::String("EF"));
  EXPECT_NE(background(third_last), background(first_last));
  EXPECT_NE(background(last), background(first_last));
  window.update(model, 0, 1000000200);
  EXPECT_EQ(background(last), background(first_last));
  window.draw();
}

TEST(MonitorWindow, ShortFdAndClassicRemoteHaveNoInventedPayload)
{
  using DeCANstructor2::Frame;
  using DeCANstructor2::FrameFormat;
  using DeCANstructor2::FrameModel;
  using DeCANstructor2::MonitorWindow;
  FrameModel model;
  MonitorWindow window;
  Frame fd;
  fd.key.source = "/fd";
  fd.key.format = FrameFormat::FD;
  fd.length = 1;
  fd.data = {0xAB};
  ASSERT_TRUE(model.ingest(fd, 100));
  Frame remote;
  remote.key.source = "/remote";
  remote.key.remote = true;
  remote.length = 8;
  ASSERT_TRUE(model.ingest(remote, 100));
  window.update(model, 1, 100);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  ASSERT_TRUE(table);
  auto fdrow = table->get<tgui::Panel>("row0");
  auto remote_row = table->get<tgui::Panel>("row1");
  ASSERT_TRUE(fdrow);
  ASSERT_TRUE(remote_row);
  EXPECT_EQ(fdrow->get<tgui::Label>("byte0")->getText(), tgui::String("AB"));
  EXPECT_EQ(fdrow->get<tgui::Label>("byte63")->getText(), tgui::String("--"));
  EXPECT_LT(remote_row->getSize().y, fdrow->getSize().y);
  EXPECT_EQ(remote_row->get<tgui::Label>("byte0")->getText(), tgui::String("--"));
  const auto metadata = metadata_text(window);
  EXPECT_EQ(metadata.find("AB"), tgui::String::npos);
  const auto status = window.gui().get<tgui::Label>("status")->getText();
  EXPECT_NE(status.find("Rejected: 1"), tgui::String::npos);
  window.gui().getContainer()->leftMousePressed(
    remote_row->getAbsolutePosition() + tgui::Vector2f{5, 5});
  window.update(model, 1, 100);
  const auto remote_metadata = metadata_text(window);
  EXPECT_NE(remote_metadata.find("/remote"), tgui::String::npos);
  EXPECT_NE(remote_metadata.find("RTR"), tgui::String::npos);
}

TEST(MonitorWindow, ClassicChangesAndEmptyFdAreRepresentedInRow)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame classic;
  classic.key.source = "/classic";
  classic.length = 2;
  classic.data = {0x12, 0x34};
  ASSERT_TRUE(model.ingest(classic, 10));
  DeCANstructor2::Frame empty;
  empty.key.source = "/empty";
  empty.key.format = DeCANstructor2::FrameFormat::FD;
  ASSERT_TRUE(model.ingest(empty, 10));
  classic.data[1] = 0x56;
  ASSERT_TRUE(model.ingest(classic, 20));
  window.update(model, 0, 20);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  auto row = table->get<tgui::Panel>("row0");
  auto fd = table->get<tgui::Panel>("row1");
  ASSERT_TRUE(row);
  ASSERT_TRUE(fd);
  EXPECT_EQ(row->get<tgui::Label>("byte1")->getText(), tgui::String("56"));
  const auto changed = row->get<tgui::Label>("byte1");
  const auto unchanged = row->get<tgui::Label>("byte0");
  EXPECT_NE(background(changed), background(unchanged));
  EXPECT_EQ(fd->get<tgui::Label>("byte0")->getText(), tgui::String("--"));
  EXPECT_EQ(fd->get<tgui::Label>("byte63")->getText(), tgui::String("--"));
  window.update(model, 0, 1000000020);
  EXPECT_EQ(background(changed), background(unchanged));
}

TEST(MonitorWindow, HeadersTrackColumnsAndNarrowWindowsPreserveCellWidths)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.key.format = DeCANstructor2::FrameFormat::FD;
  frame.length = 64;
  frame.data.resize(64, 0xAB);
  ASSERT_TRUE(model.ingest(frame, 100));
  frame.key.source = "/second";
  ASSERT_TRUE(model.ingest(frame, 100));
  window.update(model, 0, 100);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  auto columns = window.gui().get<tgui::Panel>("columns");
  ASSERT_TRUE(columns);
  auto header = columns->get<tgui::Panel>("header");
  ASSERT_TRUE(header);
  ASSERT_TRUE(header->get<tgui::Button>("id"));
  auto row = table->get<tgui::Panel>("row0");
  ASSERT_TRUE(row);
  for (const auto width : {1280.f, 1000.f}) {
    const float height = width == 1280.f ? 800.f : 600.f;
    window.gui().setAbsoluteView({0, 0, width, height});
    window.update(model, 0, 100);
    EXPECT_FLOAT_EQ(
      header->get<tgui::Button>("id")->getPosition().x,
      row->get<tgui::Label>("id")->getPosition().x);
    EXPECT_EQ(table->getHorizontalScrollbar()->getValue(), 0U);
    EXPECT_LE(row->getSize().x, table->getSize().x - 20);
    const float viewport_right = table->getPosition().x + table->getSize().x - 20;
    const float viewport_bottom = table->getPosition().y + table->getSize().y;
    for (size_t index = 0; index < 2; ++index) {
      auto fd_row = table->get<tgui::Panel>("row" + std::to_string(index));
      ASSERT_TRUE(fd_row);
      for (size_t i = 0; i < 64; ++i) {
        auto byte = fd_row->get<tgui::Label>("byte" + std::to_string(i));
        ASSERT_TRUE(byte);
        EXPECT_EQ(byte->getText(), tgui::String("AB"));
        EXPECT_GE(byte->getSize().x, 27.f);
        EXPECT_LE(byte->getAbsolutePosition().x + byte->getSize().x, viewport_right);
        EXPECT_GE(byte->getAbsolutePosition().y, table->getPosition().y);
        EXPECT_LE(byte->getAbsolutePosition().y + byte->getSize().y, viewport_bottom);
        EXPECT_LE(byte->getPosition().y + byte->getSize().y, fd_row->getSize().y);
      }
    }
  }
  window.gui().setAbsoluteView({0, 0, 800, 600});
  window.update(model, 0, 100);
  EXPECT_EQ(table->getHorizontalScrollbar()->getPolicy(), tgui::Scrollbar::Policy::Automatic);
  table->getHorizontalScrollbar()->setValue(100);
  window.update(model, 0, 100);
  EXPECT_FLOAT_EQ(header->getPosition().x, -100.f);
}

TEST(MonitorWindow, ScrollsLargeModelWithBoundedVisibleWidgets)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.key.format = DeCANstructor2::FrameFormat::FD;
  frame.length = 64;
  frame.data.resize(64, 0xCD);
  for (unsigned int i = 0; i < 4096; ++i) {
    frame.key.id = i;
    frame.key.extended = true;
    ASSERT_TRUE(model.ingest(frame, 100));
  }
  window.update(model, 0, 100);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  ASSERT_TRUE(table);
  EXPECT_LT(table->getWidgets().size(), 10U);
  table->getVerticalScrollbar()->setValue(table->getVerticalScrollbar()->getMaximum());
  window.update(model, 0, 100);
  auto last = table->get<tgui::Panel>("row4095");
  ASSERT_TRUE(last);
  EXPECT_EQ(last->get<tgui::Label>("byte63")->getText(), tgui::String("CD"));
  EXPECT_LT(table->getWidgets().size(), 10U);
}

TEST(MonitorWindow, LateEarlierKeyReordersRowsAndKeepsSelection)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.key.source = "/z";
  frame.length = 1;
  frame.data = {0xAB};
  ASSERT_TRUE(model.ingest(frame, 10));
  window.update(model, 0, 10);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  auto row = table->get<tgui::Panel>("row0");
  ASSERT_TRUE(row);
  window.gui().getContainer()->leftMousePressed(
    row->getAbsolutePosition() + tgui::Vector2f{5, 5});
  window.update(model, 0, 10);
  EXPECT_NE(metadata_text(window).find("/z"), tgui::String::npos);
  frame.key.source = "/a";
  frame.data[0] = 0xCD;
  ASSERT_TRUE(model.ingest(frame, 20));
  window.update(model, 0, 20);
  row = table->get<tgui::Panel>("row0");
  auto late = table->get<tgui::Panel>("row1");
  ASSERT_TRUE(row);
  ASSERT_TRUE(late);
  EXPECT_EQ(row->get<tgui::Label>("byte0")->getText(), tgui::String("CD"));
  EXPECT_EQ(late->get<tgui::Label>("byte0")->getText(), tgui::String("AB"));
  EXPECT_NE(metadata_text(window).find("/z"), tgui::String::npos);
}

TEST(MonitorWindow, NumericSortButtonReordersRowsAndPreservesSelection)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.length = 1;
  for (const auto & entry : {std::pair<const char *, unsigned>{"/a", 256},
      {"/b", 16}, {"/c", 2}, {"/d", 16}})
  {
    frame.key.source = entry.first;
    frame.key.id = entry.second;
    frame.data = {static_cast<uint8_t>(entry.second)};
    ASSERT_TRUE(model.ingest(frame, 10));
  }
  window.update(model, 0, 10);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  auto source = [&](size_t index) {
      return table->get<tgui::Panel>("row" + std::to_string(index))->
             get<tgui::Label>("source")->getText();
    };
  EXPECT_EQ(source(0), "/c");
  EXPECT_EQ(source(1), "/b");
  EXPECT_EQ(source(2), "/d");
  EXPECT_EQ(source(3), "/a");
  auto sidebar = window.gui().get<tgui::Panel>("message_ids");
  ASSERT_TRUE(sidebar);
  auto choices = sidebar->get<tgui::ScrollablePanel>("id_choices");
  ASSERT_TRUE(choices);
  const char * ordered_ids[] = {"002", "010", "010", "100"};
  for (size_t i = 0; i < 4; ++i) {
    auto choice = choices->get<tgui::CheckBox>("choice" + std::to_string(i));
    ASSERT_TRUE(choice);
    EXPECT_NE(choice->getText().find(ordered_ids[i]), tgui::String::npos);
  }
  auto row = table->get<tgui::Panel>("row1");
  auto point = row->getAbsolutePosition() + tgui::Vector2f{5, 5};
  window.gui().getContainer()->leftMousePressed(point);
  window.gui().getContainer()->leftMouseReleased(point);
  window.update(model, 0, 10);
  auto header = window.gui().get<tgui::Panel>("columns")->get<tgui::Panel>("header");
  auto button = header->get<tgui::Button>("id");
  ASSERT_TRUE(button);
  point = button->getAbsolutePosition() + tgui::Vector2f{5, 5};
  window.gui().getContainer()->leftMousePressed(point);
  window.gui().getContainer()->leftMouseReleased(point);
  window.update(model, 0, 10);
  EXPECT_EQ(source(0), "/a");
  EXPECT_EQ(source(1), "/b");
  EXPECT_EQ(source(2), "/d");
  EXPECT_EQ(source(3), "/c");
  EXPECT_NE(metadata_text(window).find("/b"), tgui::String::npos);
  row = table->get<tgui::Panel>("row0");
  point = row->getAbsolutePosition() + tgui::Vector2f{5, 5};
  window.gui().getContainer()->leftMousePressed(point);
  window.gui().getContainer()->leftMouseReleased(point);
  window.update(model, 0, 10);
  EXPECT_NE(metadata_text(window).find("/a"), tgui::String::npos);
}

TEST(MonitorWindow, StreamControlsHideAndRestoreWhileReceptionContinues)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.key.source = "/first";
  frame.length = 1;
  frame.data = {0x12};
  ASSERT_TRUE(model.ingest(frame, 10));
  window.update(model, 0, 10);
  auto click = [&](const tgui::Widget::Ptr & widget) {
      ASSERT_TRUE(widget);
      const auto point = widget->getAbsolutePosition() + tgui::Vector2f{5, 5};
      window.gui().getContainer()->leftMousePressed(point);
      window.gui().getContainer()->leftMouseReleased(point);
      window.update(model, 0, 10);
    };
  auto selector = window.gui().get<tgui::Panel>("message_ids");
  ASSERT_TRUE(selector);
  click(selector->get<tgui::Button>("hide_all"));
  EXPECT_EQ(window.gui().get<tgui::ScrollablePanel>("frames")->getWidgets().size(), 0U);
  EXPECT_EQ(
    metadata_text(window).find("/first"),
    tgui::String::npos);
  EXPECT_TRUE(window.gui().get<tgui::Label>("empty_hint")->isVisible());
  EXPECT_NE(
    window.gui().get<tgui::Label>("empty_hint")->getText().find("reception continues"),
    tgui::String::npos);
  EXPECT_NE(window.gui().get<tgui::Label>("status")->getText().find("0/1"), tgui::String::npos);
  frame.data[0] = 0x34;
  ASSERT_TRUE(model.ingest(frame, 20));
  frame.key.source = "/late";
  ASSERT_TRUE(model.ingest(frame, 30));
  window.update(model, 0, 30);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  ASSERT_TRUE(table->get<tgui::Panel>("row0"));
  EXPECT_EQ(table->get<tgui::Panel>("row0")->get<tgui::Label>("source")->getText(), "/late");
  click(selector->get<tgui::Button>("show_all"));
  auto first = table->get<tgui::Panel>("row0");
  ASSERT_TRUE(first);
  EXPECT_EQ(first->get<tgui::Label>("byte0")->getText(), "34");
  EXPECT_EQ(first->get<tgui::Label>("count")->getText(), "2");
  EXPECT_EQ(metadata_text(window), "Select a frame");
}

TEST(MonitorWindow, CheckboxUsesFullKeyAndDoesNotReselectAfterRestore)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.key.source = "/same";
  frame.key.id = 7;
  frame.length = 1;
  frame.data = {0x12};
  ASSERT_TRUE(model.ingest(frame, 10));
  frame.key.format = DeCANstructor2::FrameFormat::FD;
  frame.data[0] = 0x34;
  ASSERT_TRUE(model.ingest(frame, 10));
  window.update(model, 0, 10);
  auto click = [&](const tgui::Widget::Ptr & widget) {
      ASSERT_TRUE(widget);
      const auto point = widget->getAbsolutePosition() + tgui::Vector2f{5, 5};
      window.gui().getContainer()->leftMousePressed(point);
      window.gui().getContainer()->leftMouseReleased(point);
      window.update(model, 0, 10);
    };
  auto selector = window.gui().get<tgui::Panel>("message_ids");
  ASSERT_TRUE(selector);
  auto choices = selector->get<tgui::ScrollablePanel>("id_choices");
  auto checkbox = choices->get<tgui::CheckBox>("choice0");
  ASSERT_TRUE(checkbox);
  click(checkbox);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  auto row = table->get<tgui::Panel>("row0");
  ASSERT_TRUE(row);
  EXPECT_EQ(row->get<tgui::Label>("format")->getText(), "FD");
  EXPECT_EQ(row->get<tgui::Label>("byte0")->getText(), "34");
  EXPECT_EQ(metadata_text(window), "Select a frame");
  checkbox = choices->get<tgui::CheckBox>("choice0");
  click(checkbox);
  EXPECT_EQ(metadata_text(window), "Select a frame");
  ASSERT_TRUE(table->get<tgui::Panel>("row1"));
}

TEST(MonitorWindow, BulkActionsAffectOnlyKeysKnownAtClick)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame known;
  known.key.source = "/known";
  ASSERT_TRUE(model.ingest(known, 10));
  window.update(model, 0, 10);
  auto click = [&](const tgui::Widget::Ptr & widget) {
      ASSERT_TRUE(widget);
      const auto point = widget->getAbsolutePosition() + tgui::Vector2f{5, 5};
      window.gui().getContainer()->leftMousePressed(point);
      window.gui().getContainer()->leftMouseReleased(point);
    };
  auto selector = window.gui().get<tgui::Panel>("message_ids");
  ASSERT_TRUE(selector);
  click(selector->get<tgui::Button>("hide_all"));
  // A callback is followed by ROS executor work before the next GUI update.
  DeCANstructor2::Frame late;
  late.key.source = "/late";
  ASSERT_TRUE(model.ingest(late, 20));
  window.update(model, 0, 20);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  auto row = table->get<tgui::Panel>("row0");
  ASSERT_TRUE(row);
  EXPECT_EQ(row->get<tgui::Label>("source")->getText(), "/late");
  EXPECT_EQ(table->getWidgets().size(), 1U);
  EXPECT_NE(window.gui().get<tgui::Label>("status")->getText().find("1/2"), tgui::String::npos);
  EXPECT_EQ(metadata_text(window), "Select a frame");
}

TEST(MonitorWindow, PermanentMessageIdsAndHorizontalMetadataFitBothSizes)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.key.source = "/a/long/source/name/that/must/remain/available/in/the/tooltip";
  frame.key.format = DeCANstructor2::FrameFormat::FD;
  frame.length = 64;
  frame.data.resize(64, 0xAB);
  ASSERT_TRUE(model.ingest(frame, 10));
  window.update(model, 0, 10);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  auto sidebar = window.gui().get<tgui::Panel>("message_ids");
  auto metadata = window.gui().get<tgui::Panel>("metadata");
  ASSERT_TRUE(table);
  ASSERT_TRUE(sidebar);
  ASSERT_TRUE(metadata);
  ASSERT_TRUE(sidebar->get<tgui::Label>("title"));
  EXPECT_EQ(sidebar->get<tgui::Label>("title")->getText(), "MESSAGE IDS");
  ASSERT_TRUE(metadata->get<tgui::Label>("title"));
  EXPECT_EQ(metadata->get<tgui::Label>("title")->getText(), "FRAME METADATA");
  ASSERT_FALSE(window.gui().get<tgui::Button>("streams_toggle"));
  ASSERT_FALSE(window.gui().get<tgui::ChildWindow>("stream_selector"));
  for (const auto & widget : window.gui().getContainer()->getWidgets()) {
    EXPECT_FALSE(std::dynamic_pointer_cast<tgui::ChildWindow>(widget));
  }
  auto hint = metadata->get<tgui::Label>("selection_hint");
  auto choices = sidebar->get<tgui::ScrollablePanel>("id_choices");
  auto hide = sidebar->get<tgui::Button>("hide_all");
  auto show = sidebar->get<tgui::Button>("show_all");
  ASSERT_TRUE(hint);
  ASSERT_TRUE(choices);
  ASSERT_TRUE(hide);
  ASSERT_TRUE(show);
  auto click = [&](const tgui::Widget::Ptr & widget) {
      ASSERT_TRUE(widget);
      const auto point = widget->getAbsolutePosition() + tgui::Vector2f{5, 5};
      window.gui().getContainer()->leftMousePressed(point);
      window.gui().getContainer()->leftMouseReleased(point);
      window.update(model, 0, 10);
    };
  for (const auto width : {1280.f, 1000.f}) {
    const float height = width == 1280.f ? 800.f : 600.f;
    window.gui().setAbsoluteView({0, 0, width, height});
    window.update(model, 0, 10);
    EXPECT_TRUE(sidebar->isVisible());
    EXPECT_GE(
      sidebar->getAbsolutePosition().x,
      table->getPosition().x + table->getSize().x);
    EXPECT_LE(sidebar->getAbsolutePosition().x + sidebar->getSize().x, width);
    EXPECT_GE(
      metadata->getAbsolutePosition().y,
      table->getPosition().y + table->getSize().y);
    EXPECT_LE(
      metadata->getAbsolutePosition().x + metadata->getSize().x,
      sidebar->getAbsolutePosition().x);
    EXPECT_LE(metadata->getAbsolutePosition().y + metadata->getSize().y, height);
    auto row = table->get<tgui::Panel>("row0");
    ASSERT_TRUE(row);
    click(row);
    EXPECT_FALSE(hint->isVisible());
    std::vector<tgui::Label::Ptr> fields;
    for (const auto name : {"id", "source", "format", "flags", "brs_esi"}) {
      auto field = metadata->get<tgui::Label>(name);
      ASSERT_TRUE(field);
      EXPECT_TRUE(field->isVisible());
      EXPECT_GE(field->getAbsolutePosition().x, metadata->getAbsolutePosition().x);
      EXPECT_GE(field->getAbsolutePosition().y, metadata->getAbsolutePosition().y);
      EXPECT_LE(
        field->getAbsolutePosition().y + field->getSize().y,
        metadata->getAbsolutePosition().y + metadata->getSize().y);
      EXPECT_LE(
        field->getAbsolutePosition().x + field->getSize().x,
        metadata->getAbsolutePosition().x + metadata->getSize().x);
      for (const auto & other : fields) {
        EXPECT_TRUE(
          field->getAbsolutePosition().x + field->getSize().x <= other->getAbsolutePosition().x ||
          other->getAbsolutePosition().x + other->getSize().x <= field->getAbsolutePosition().x);
      }
      fields.push_back(field);
    }
    auto source = metadata->get<tgui::Label>("source");
    auto tooltip = std::dynamic_pointer_cast<tgui::Label>(source->getToolTip());
    ASSERT_TRUE(tooltip);
    EXPECT_NE(tooltip->getText().find(frame.key.source), tgui::String::npos);
    click(hide);
    EXPECT_TRUE(sidebar->isVisible());
    EXPECT_TRUE(hint->isVisible());
    EXPECT_EQ(hint->getText(), "Select a frame");
    EXPECT_EQ(table->getWidgets().size(), 0U);
    click(show);
    ASSERT_TRUE(table->get<tgui::Panel>("row0"));
    auto choice = choices->get<tgui::CheckBox>("choice0");
    ASSERT_TRUE(choice);
    click(choice);
    EXPECT_EQ(table->getWidgets().size(), 0U);
    choice = choices->get<tgui::CheckBox>("choice0");
    ASSERT_TRUE(choice);
    click(choice);
    ASSERT_TRUE(table->get<tgui::Panel>("row0"));
    EXPECT_NE(
      window.gui().get<tgui::Label>("status")->getText().find("Message IDs:"),
      tgui::String::npos);
  }
}

TEST(MonitorWindow, VirtualizedChoicesReachLastStreamAndMixedRowsScroll)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.key.extended = true;
  frame.length = 1;
  frame.data = {0xAB};
  for (unsigned int i = 0; i < 4096; ++i) {
    frame.key.id = i;
    frame.key.format = i % 2 ? DeCANstructor2::FrameFormat::FD :
      DeCANstructor2::FrameFormat::Classic;
    ASSERT_TRUE(model.ingest(frame, 10));
  }
  window.update(model, 0, 10);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  table->getVerticalScrollbar()->setValue(table->getVerticalScrollbar()->getMaximum());
  window.update(model, 0, 10);
  ASSERT_TRUE(table->get<tgui::Panel>("row4095"));
  EXPECT_TRUE(table->get<tgui::Panel>("row4095")->get<tgui::Label>("byte63"));
  EXPECT_LT(table->getWidgets().size(), 30U);
  auto selector = window.gui().get<tgui::Panel>("message_ids");
  ASSERT_TRUE(selector);
  auto choices = selector->get<tgui::ScrollablePanel>("id_choices");
  ASSERT_TRUE(choices);
  const auto choice_bound = 2U * (static_cast<size_t>(choices->getSize().y / 48.f) + 3U);
  EXPECT_LE(choices->getWidgets().size(), choice_bound);
  choices->getVerticalScrollbar()->setValue(choices->getVerticalScrollbar()->getMaximum());
  window.update(model, 0, 10);
  auto last = choices->get<tgui::CheckBox>("choice4095");
  ASSERT_TRUE(last);
  EXPECT_NE(last->getText().find("00000FFF"), tgui::String::npos);
  EXPECT_LE(choices->getWidgets().size(), choice_bound);
  auto point = last->getAbsolutePosition() + tgui::Vector2f{5, 5};
  window.gui().getContainer()->leftMousePressed(point);
  window.gui().getContainer()->leftMouseReleased(point);
  window.update(model, 0, 10);
  last = choices->get<tgui::CheckBox>("choice4095");
  ASSERT_TRUE(last);
  EXPECT_FALSE(last->isChecked());
  EXPECT_NE(
    window.gui().get<tgui::Label>("status")->getText().find("4095/4096"),
    tgui::String::npos);
  EXPECT_EQ(table->getVerticalScrollbar()->getValue(), 0U);
}

TEST(MonitorWindow, ShrinkingFdClearsUnusedCellsAndHighlights)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame frame;
  frame.key.format = DeCANstructor2::FrameFormat::FD;
  frame.length = 64;
  frame.data.resize(64, 0xAA);
  ASSERT_TRUE(model.ingest(frame, 10));
  frame.data[63] = 0xBB;
  ASSERT_TRUE(model.ingest(frame, 20));
  window.update(model, 0, 20);
  auto table = window.gui().get<tgui::ScrollablePanel>("frames");
  auto row = table->get<tgui::Panel>("row0");
  ASSERT_TRUE(row);
  auto last = row->get<tgui::Label>("byte63");
  auto first = row->get<tgui::Label>("byte0");
  ASSERT_TRUE(last);
  ASSERT_TRUE(first);
  EXPECT_NE(background(last), background(first));
  frame.length = 1;
  frame.data.resize(1);
  ASSERT_TRUE(model.ingest(frame, 30));
  window.update(model, 0, 30);
  EXPECT_EQ(last->getText(), tgui::String("--"));
  EXPECT_EQ(background(last), background(first));
  frame.length = 64;
  frame.data.resize(64, 0xCC);
  ASSERT_TRUE(model.ingest(frame, 40));
  window.update(model, 0, 40);
  EXPECT_EQ(last->getText(), tgui::String("CC"));
  EXPECT_NE(background(last), background(first));
}
