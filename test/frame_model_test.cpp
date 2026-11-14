#include "decanstructor2/frame_model.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using namespace DeCANstructor2;

Frame make_frame(uint8_t length = 2) {
    Frame f;
    f.key.source = "can0";
    f.key.id = 42;
    f.length = length;
    f.data.resize(length, 1);
    return f;
}

void basic_and_timing() {
    FrameModel m;
    auto f = make_frame();
    assert(m.ingest(f, 1000000));
    assert(m.rows().size() == 1);
    assert(m.rows().at(f.key).count == 1);
    assert(m.rows().at(f.key).frame.data == f.data);
    assert(!m.rows().at(f.key).average_interval_ms);
    assert(!m.rows().at(f.key).changed_at_ns[0]);
    assert(m.ingest(f, 3000000));
    assert(m.rows().at(f.key).average_interval_ms == 2.0);
    assert(m.ingest(f, 7000000));
    assert(m.rows().at(f.key).average_interval_ms == 3.0);
    assert(m.ingest(f, 7000000));
    assert(!m.rows().at(f.key).average_interval_ms);
    assert(m.ingest(f, 6000000));
    assert(!m.rows().at(f.key).average_interval_ms);
    assert(m.ingest(f, 9000000));
    assert(m.rows().at(f.key).average_interval_ms == 3.0);
    assert(m.rows().at(f.key).count == 6);
    FrameModel extreme;
    assert(extreme.ingest(f, std::numeric_limits<int64_t>::min()));
    assert(extreme.ingest(f, std::numeric_limits<int64_t>::max()));
    assert(std::isfinite(*extreme.rows().at(f.key).average_interval_ms));
    assert(*extreme.rows().at(f.key).average_interval_ms > 0);
}

void changes() {
    FrameModel m;
    auto f = make_frame();
    assert(m.ingest(f, 1));
    f.data[1] = 2;
    assert(m.ingest(f, 2));
    assert(!m.rows().at(f.key).changed_at_ns[0]);
    assert(m.rows().at(f.key).changed_at_ns[1] == 2);
    f.length = 3; f.data.push_back(9);
    assert(m.ingest(f, 3));
    assert(m.rows().at(f.key).changed_at_ns[1] == 2);
    assert(m.rows().at(f.key).changed_at_ns[2] == 3);
    f.length = 1; f.data.resize(1);
    assert(m.ingest(f, 4));
    assert(!m.rows().at(f.key).changed_at_ns[1]);
    assert(!m.rows().at(f.key).changed_at_ns[2]);
    f.length = 2; f.data.push_back(2);
    assert(m.ingest(f, 5));
    assert(m.rows().at(f.key).changed_at_ns[1] == 5);
}

void validation() {
    FrameModel m;
    auto f = make_frame();
    assert(m.ingest(f, 10));
    const auto before = m.rows().at(f.key);
    auto invalid = [&](const Frame& bad) {
        assert(!m.ingest(bad, 20));
        assert(m.rows().size() == 1);
        const auto& after = m.rows().at(f.key);
        assert(after.count == before.count);
        assert(after.frame.key == before.frame.key);
        assert(after.frame.length == before.frame.length);
        assert(after.frame.data == before.frame.data);
        assert(after.average_interval_ms == before.average_interval_ms);
        assert(after.changed_at_ns == before.changed_at_ns);
    };
    auto bad = f; bad.length = 9; bad.data.resize(9); invalid(bad);
    bad = f; bad.data.pop_back(); invalid(bad);
    bad = f; bad.key.id = 0x800; invalid(bad);
    bad = f; bad.key.extended = true; bad.key.id = 0x20000000; invalid(bad);
    bad = f; bad.key.error = true; bad.key.id = 0x20000000; invalid(bad);
    bad = f; bad.key.format = FrameFormat::FD; bad.length = 65; bad.data.resize(65); invalid(bad);
    bad = f; bad.key.format = FrameFormat::FD; bad.key.remote = true; bad.data.clear(); invalid(bad);
    bad = f; bad.key.remote = true; invalid(bad);
    assert(m.ingest(f, 1000010));
    assert(m.rows().at(f.key).average_interval_ms == 1.0);
    FrameModel boundaries;
    auto classic = make_frame(8); classic.key.id = 0x7ff;
    assert(boundaries.ingest(classic, 1));
    auto extended = classic; extended.key.extended = true; extended.key.id = 0x1fffffff;
    assert(boundaries.ingest(extended, 1));
    auto error = classic; error.key.error = true; error.key.id = 0x1fffffff;
    assert(boundaries.ingest(error, 1));
    auto fd = make_frame(64); fd.key.format = FrameFormat::FD;
    assert(boundaries.ingest(fd, 1));
    assert(boundaries.rows().at(fd.key).frame.data.size() == 64);
    fd.data[63] = 7; assert(boundaries.ingest(fd, 2));
    assert(boundaries.rows().at(fd.key).changed_at_ns[63] == 2);
    fd.length = 0; fd.data.clear(); assert(boundaries.ingest(fd, 3));
    assert(!boundaries.rows().at(fd.key).changed_at_ns[63]);
    auto rtr = make_frame(8); rtr.key.remote = true; rtr.data.clear();
    assert(boundaries.ingest(rtr, 1));
    assert(boundaries.rows().at(rtr.key).frame.length == 8);
    assert(boundaries.rows().at(rtr.key).frame.data.empty());
}

void distinct_keys() {
    FrameModel m;
    auto base = make_frame();
    assert(m.ingest(base, 1));
    auto f = base; f.key.source = "can1"; assert(m.ingest(f, 1));
    f = base; f.key.extended = true; assert(m.ingest(f, 1));
    f = base; f.key.error = true; assert(m.ingest(f, 1));
    f = base; f.key.remote = true; f.data.clear(); assert(m.ingest(f, 1));
    f = base; f.key.format = FrameFormat::FD; assert(m.ingest(f, 1));
    assert(m.rows().size() == 6);
    for (const auto& row : m.rows()) assert(row.second.count == 1);
}

int main() {
    basic_and_timing();
    changes();
    validation();
    distinct_keys();
    std::cout << "frame model tests passed\n";
}
