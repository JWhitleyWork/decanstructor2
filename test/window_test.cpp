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

TEST(MonitorWindow, DisplaysAll64BytesAndHighlightsOnlyChangedBytes)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  window.update(model, 0, 100);
  auto list = window.gui().get<tgui::ListView>("frames");
  ASSERT_EQ(list->getItemCount(), 0U);
  DeCANstructor2::Frame frame;
  frame.key.source = "/can_fd_in";
  frame.key.id = 0x123;
  frame.key.format = DeCANstructor2::FrameFormat::FD;
  frame.length = 64;
  frame.data.resize(64, 0xAA);
  frame.data[63] = 0xBC;
  ASSERT_TRUE(model.ingest(frame, 100));
  window.update(model, 0, 100);
  ASSERT_EQ(list->getItemCount(), 1U);
  EXPECT_EQ(list->getItemRow(0)[2], tgui::String("FD"));
  EXPECT_EQ(list->getItemRow(0)[4], tgui::String("64"));
  auto last = window.gui().get<tgui::Label>("byte63");
  auto first = window.gui().get<tgui::Label>("byte0");
  EXPECT_EQ(last->getText(), tgui::String("3F: BC"));
  frame.data[63] = 0xEF;
  ASSERT_TRUE(model.ingest(frame, 200));
  window.update(model, 0, 200);
  EXPECT_EQ(last->getText(), tgui::String("3F: EF"));
  EXPECT_NE(last->getRenderer()->getBackgroundColor(), first->getRenderer()->getBackgroundColor());
  window.update(model, 0, 2000000200);
  EXPECT_EQ(last->getRenderer()->getBackgroundColor(), first->getRenderer()->getBackgroundColor());
  EXPECT_EQ(list->getItemCount(), 1U);
  window.draw();
}

TEST(MonitorWindow, SelectionShowsClassicRemoteFrameWithoutInventedPayload)
{
  DeCANstructor2::FrameModel model;
  DeCANstructor2::MonitorWindow window;
  DeCANstructor2::Frame fd;
  fd.key.source = "/fd";
  fd.key.format = DeCANstructor2::FrameFormat::FD;
  fd.length = 1;
  fd.data = {0xAA};
  ASSERT_TRUE(model.ingest(fd, 100));
  window.update(model, 0, 100);
  DeCANstructor2::Frame remote;
  remote.key.source = "/classic";
  remote.key.remote = true;
  remote.length = 8;
  ASSERT_TRUE(model.ingest(remote, 200));
  window.update(model, 1, 200);
  auto list = window.gui().get<tgui::ListView>("frames");
  ASSERT_EQ(list->getItemCount(), 2U);
  EXPECT_EQ(list->getSelectedItemIndex(), 0);
  list->setSelectedItem(1);
  window.update(model, 1, 200);
  EXPECT_EQ(window.gui().get<tgui::Label>("byte0")->getText(), tgui::String("00: --"));
  EXPECT_EQ(list->getItemRow(1)[2], tgui::String("CAN"));
  EXPECT_EQ(list->getItemRow(1)[7], tgui::String("RTR (no payload)"));
  const auto status_text = window.gui().get<tgui::Label>("status")->getText();
  EXPECT_NE(status_text.find("Rejected: 1"), tgui::String::npos);
}
