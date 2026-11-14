#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace DeCANstructor2 {
enum class FrameFormat { Classic, FD };
struct FrameKey {
    std::string source;
    uint32_t id = 0;
    bool extended = false;
    bool error = false;
    bool remote = false;
    FrameFormat format = FrameFormat::Classic;
    bool operator<(const FrameKey& other) const;
    bool operator==(const FrameKey& other) const;
};
struct Frame {
    FrameKey key;
    uint8_t length = 0;
    std::vector<uint8_t> data;
};
struct MessageState {
    Frame frame;
    uint64_t count = 0;
    std::optional<double> average_interval_ms;
    std::array<std::optional<int64_t>, 64> changed_at_ns{};
};
class FrameModel {
public:
    bool ingest(const Frame& frame, int64_t received_ns);
    const std::map<FrameKey, MessageState>& rows() const { return rows_; }
private:
    struct Timing {
        int64_t last_ns = 0;
        uint64_t intervals = 0;
        long double mean_ms = 0;
    };
    std::map<FrameKey, MessageState> rows_;
    std::map<FrameKey, Timing> timing_;
};
}
