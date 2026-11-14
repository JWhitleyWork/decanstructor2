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
#include "decanstructor2/monitor_window.hpp"

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
  frame.data[63] = 0xEF;
  ASSERT_TRUE(model.ingest(frame, 200));
  frame.key.source = "/fd2";
  frame.data[63] = 0xF0;
  ASSERT_TRUE(model.ingest(frame, 200));
  window.update(model, 0, 200);
  EXPECT_EQ(last->getText(), tgui::String("F0"));
  EXPECT_EQ(third_last->getText(), tgui::String("EF"));
  EXPECT_NE(third_last->getRenderer()->getBackgroundColor(),
    first_last->getRenderer()->getBackgroundColor());
  EXPECT_NE(last->getRenderer()->getBackgroundColor(),
    first_last->getRenderer()->getBackgroundColor());
  window.update(model, 0, 1000000200);
  EXPECT_EQ(last->getRenderer()->getBackgroundColor(),
    first_last->getRenderer()->getBackgroundColor());
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
  const auto metadata = window.gui().get<tgui::Label>("selected")->getText();
  EXPECT_EQ(metadata.find("AB"), tgui::String::npos);
  EXPECT_NE(window.gui().get<tgui::Label>("status")->getText().find("Rejected: 1"),
    tgui::String::npos);
  remote_row->leftMousePressed(remote_row->getPosition() + tgui::Vector2f{5, 5});
  window.update(model, 1, 100);
  const auto remote_metadata = window.gui().get<tgui::Label>("selected")->getText();
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
  EXPECT_NE(row->get<tgui::Label>("byte1")->getRenderer()->getBackgroundColor(),
    row->get<tgui::Label>("byte0")->getRenderer()->getBackgroundColor());
  EXPECT_EQ(fd->get<tgui::Label>("byte0")->getText(), tgui::String("--"));
  EXPECT_EQ(fd->get<tgui::Label>("byte63")->getText(), tgui::String("--"));
  window.update(model, 0, 1000000020);
  EXPECT_EQ(row->get<tgui::Label>("byte1")->getRenderer()->getBackgroundColor(),
    row->get<tgui::Label>("byte0")->getRenderer()->getBackgroundColor());
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
