#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace hf {
inline std::uint64_t mix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
struct Stats { std::uint64_t probes = 0, moves = 0, rebuilds = 0; };
struct Slot { std::uint64_t key = 0; std::uint8_t state = 0; };
class TableBase {
protected:
    std::vector<Slot> slots_;
    std::size_t mask_, count_ = 0;
    std::uint64_t salt_;
    mutable Stats stats_{};
    std::size_t home(std::uint64_t k) const { return static_cast<std::size_t>(mix64(k ^ salt_)) & mask_; }
    std::size_t distance(std::size_t from, std::size_t to) const { return (to - from) & mask_; }
    void touch() const { ++stats_.probes; }
public:
    explicit TableBase(std::size_t capacity, std::uint64_t salt = 0)
        : slots_(std::bit_ceil(capacity < 2 ? 2 : capacity)), mask_(slots_.size() - 1), salt_(salt) {}
    std::size_t size() const { return count_; }
    std::size_t capacity() const { return slots_.size(); }
    std::size_t bytes() const { return slots_.capacity() * sizeof(Slot); }
    Stats stats() const { return stats_; }
    void reset_stats() const { stats_ = {}; }
};
// Linear probing, lazy tombstones. Deliberately no automatic resizing.
class LinearTombstone : public TableBase {
public:
    using TableBase::TableBase;
    bool contains(std::uint64_t key) const {
        auto i = home(key);
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_) {
            touch();
            if (slots_[i].state == 0) return false;
            if (slots_[i].state == 1 && slots_[i].key == key) return true;
        }
        return false;
    }
    bool insert(std::uint64_t key) {
        auto i = home(key), first_dead = capacity();
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_) {
            touch();
            if (slots_[i].state == 1 && slots_[i].key == key) return false;
            if (slots_[i].state == 2 && first_dead == capacity()) first_dead = i;
            if (slots_[i].state == 0) {
                const auto dst = first_dead == capacity() ? i : first_dead;
                slots_[dst] = {key, 1}; ++count_; return true;
            }
        }
        if (first_dead != capacity()) { slots_[first_dead] = {key, 1}; ++count_; return true; }
        return false;
    }
    bool erase(std::uint64_t key) {
        auto i = home(key);
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_) {
            touch();
            if (slots_[i].state == 0) return false;
            if (slots_[i].state == 1 && slots_[i].key == key) {
                slots_[i].state = 2; --count_; return true;
            }
        }
        return false;
    }
};
// Stop-the-world tombstone cleanup control. Not Zombie Hashing or Graveyard Hashing.
// Rebuilds the SAME capacity when tombstones exceed a table-size or free-space budget.
// Count scans and reinserts explicitly; temporary scratch allocation may double
// the peak slot storage, though `bytes()` records only steady-state slot storage.
template <unsigned ThresholdPercent, bool SlackBased = false> class LinearPeriodicRebuild : public LinearTombstone {
    static_assert(ThresholdPercent >= 1 && ThresholdPercent <= 100);
    std::size_t tombstones_ = 0;

    void rebuild() {
        // Safe to reconstruct because all live keys are reinserted with their
        // original homes and no key is removed from the logical set.
        std::vector<Slot> rebuilt(this->capacity());
        for (const auto& entry : this->slots_) {
            this->touch();
            if (entry.state != 1) continue;
            auto dst = this->home(entry.key);
            while (rebuilt[dst].state != 0) {
                this->touch();
                dst = (dst + 1) & this->mask_;
            }
            rebuilt[dst] = entry;
            ++this->stats_.moves;
        }
        this->slots_.swap(rebuilt);
        this->tombstones_ = 0;
        ++this->stats_.rebuilds;
    }
public:
    using LinearTombstone::LinearTombstone;
    bool insert(std::uint64_t key) {
        auto i = this->home(key), first_dead = this->capacity();
        for (std::size_t step = 0; step < this->capacity(); ++step, i = (i + 1) & this->mask_) {
            this->touch();
            if (this->slots_[i].state == 1 && this->slots_[i].key == key) return false;
            if (this->slots_[i].state == 2 && first_dead == this->capacity()) first_dead = i;
            if (this->slots_[i].state == 0) {
                const auto dst = first_dead == this->capacity() ? i : first_dead;
                this->slots_[dst] = {key, 1};
                if (dst == first_dead) --tombstones_;
                ++this->count_;
                return true;
            }
        }
        if (first_dead != this->capacity()) {
            this->slots_[first_dead] = {key, 1};
            --tombstones_; ++this->count_;
            return true;
        }
        return false;
    }
    bool erase(std::uint64_t key) {
        if (!LinearTombstone::erase(key)) return false;
        ++tombstones_;
        const auto scale = SlackBased ? this->capacity() - this->size() : this->capacity();
        if (tombstones_ * 100 >= scale * ThresholdPercent) rebuild();
        return true;
    }
};

// Linear probing with backward-shift deletion, without tombstones.
class LinearBackshift : public TableBase {
public:
    using TableBase::TableBase;
    bool contains(std::uint64_t key) const {
        auto i = home(key);
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_) {
            touch();
            if (!slots_[i].state) return false;
            if (slots_[i].key == key) return true;
        }
        return false;
    }
    bool insert(std::uint64_t key) {
        auto i = home(key);
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_) {
            touch();
            if (slots_[i].state && slots_[i].key == key) return false;
            if (!slots_[i].state) { slots_[i] = {key, 1}; ++count_; return true; }
        }
        return false;
    }
    bool erase(std::uint64_t key) {
        auto i = home(key);
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_) {
            touch();
            if (!slots_[i].state) return false;
            if (slots_[i].key != key) continue;
            auto hole = i; slots_[hole].state = 0;
            for (auto next = (hole + 1) & mask_; slots_[next].state; next = (next + 1) & mask_) {
                touch();
                const auto h = home(slots_[next].key);
                if (distance(h, next) > distance(h, hole)) {
                    slots_[hole] = slots_[next]; slots_[next].state = 0; hole = next; ++stats_.moves;
                }
            }
            slots_[hole].state = 0; --count_; return true;
        }
        return false;
    }
};
// Classic Robin Hood insertion + early-stop lookup + backward-shift deletion.
class RobinHood : public TableBase {
public:
    using TableBase::TableBase;
    bool contains(std::uint64_t key) const {
        auto i = home(key);
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_) {
            touch();
            if (!slots_[i].state) return false;
            if (slots_[i].key == key) return true;
            if (distance(home(slots_[i].key), i) < step) return false;
        }
        return false;
    }
    bool insert(std::uint64_t key) {
        if (contains(key)) return false;
        if (count_ == capacity()) return false;
        auto i = home(key);
        std::size_t dist = 0;
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_, ++dist) {
            touch();
            if (!slots_[i].state) { slots_[i] = {key, 1}; ++count_; return true; }
            const auto occupant_dist = distance(home(slots_[i].key), i);
            if (occupant_dist < dist) {
                std::swap(key, slots_[i].key); dist = occupant_dist; ++stats_.moves;
            }
        }
        throw std::logic_error("robin hood displacement invariant violated");
    }
    bool erase(std::uint64_t key) {
        auto i = home(key);
        for (std::size_t step = 0; step < capacity(); ++step, i = (i + 1) & mask_) {
            touch();
            if (!slots_[i].state) return false;
            if (slots_[i].key == key) {
                auto hole = i; slots_[hole].state = 0;
                for (auto next = (hole + 1) & mask_; slots_[next].state; next = (next + 1) & mask_) {
                    touch();
                    if (distance(home(slots_[next].key), next) == 0) break;
                    slots_[hole] = slots_[next]; slots_[next].state = 0; hole = next; ++stats_.moves;
                }
                slots_[hole].state = 0; --count_; return true;
            }
            if (distance(home(slots_[i].key), i) < step) return false;
        }
        return false;
    }
};
}
