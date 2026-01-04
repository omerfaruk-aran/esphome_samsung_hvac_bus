// log.cpp
#include "log.h"
#include "esphome/core/hal.h"
#include <map>
#include <cmath>

namespace esphome {
namespace samsung_ac {

bool debug_log_raw_bytes = false;
bool debug_log_messages = false;
bool debug_log_undefined_messages = false;

// NEW:
bool debug_log_messages_on_change = false;

struct DedupEntry {
  double last = 0.0;
  uint32_t last_ms = 0;
  bool has = false;
};

static std::map<uint32_t, DedupEntry> s_dedup;
static constexpr size_t MAX_DEDUP_ENTRIES = 256;

uint32_t fnv1a_32(const char *data, size_t len) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < len; i++) {
    h ^= (uint8_t)data[i];
    h *= 16777619u;
  }
  return h;
}

uint32_t fnv1a_32(const std::string &s) {
  return fnv1a_32(s.data(), s.size());
}

uint32_t log_dedup_key(const std::string &source, const std::string &dest, uint16_t id) {
  // Simple stable key: hash(src) ^ rot(hash(dst)) ^ id
  uint32_t hs = fnv1a_32(source);
  uint32_t hd = fnv1a_32(dest);
  uint32_t rot = (hd << 1) | (hd >> 31);
  return hs ^ rot ^ (uint32_t)id;
}

static void evict_if_needed() {
  if (s_dedup.size() <= MAX_DEDUP_ENTRIES) return;
  // Cheap eviction: remove oldest map entry (not perfect LRU, but good enough for debug)
  s_dedup.erase(s_dedup.begin());
}

bool log_should_print(uint32_t key, double value, double epsilon, uint32_t min_interval_ms) {
  const uint32_t now = millis();

  auto &e = s_dedup[key];
  evict_if_needed();

  if (!e.has) {
    e.has = true;
    e.last = value;
    e.last_ms = now;
    return true;
  }

  const bool changed = std::fabs(e.last - value) > epsilon;
  const bool due = (min_interval_ms > 0) && ((uint32_t)(now - e.last_ms) >= min_interval_ms);

  if (changed || due) {
    e.last = value;
    e.last_ms = now;
    return true;
  }
  return false;
}

} // namespace samsung_ac
} // namespace esphome
