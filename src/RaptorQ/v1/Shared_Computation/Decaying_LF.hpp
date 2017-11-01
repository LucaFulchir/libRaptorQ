/*
 * Copyright (c) 2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
 *
 * This file is part of "libRaptorQ".
 *
 * libRaptorQ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libRaptorQ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and a copy of the GNU Lesser General Public License
 * along with libRaptorQ.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/Operation.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>
#include <utility>


namespace RaptorQ__v1 {
namespace Impl {


// easy compress from/to eigen matrix
std::vector<uint8_t> RAPTORQ_API Mtx_to_raw (const DenseMtx &mtx);
inline std::vector<uint8_t> RAPTORQ_API Mtx_to_raw (const DenseMtx &mtx)
{
    std::vector<uint8_t> ret;
    ret.reserve (static_cast<size_t> (mtx.rows() * mtx.cols()));
    for (uint32_t row = 0; row < static_cast<uint32_t> (mtx.rows()); ++row) {
        for (uint32_t col = 0; col < static_cast<uint32_t> (mtx.cols()); ++col)
            ret.emplace_back (static_cast<uint8_t> (mtx (row, col)));
    }
    return ret;
}

DenseMtx RAPTORQ_API raw_to_Mtx (const std::vector<uint8_t> &raw,
                                                        const uint32_t cols);
inline DenseMtx RAPTORQ_API raw_to_Mtx (const std::vector<uint8_t> &raw,
                                                        const uint32_t cols)
{
    uint16_t rows = static_cast<uint16_t> (raw.size() / cols);
    DenseMtx ret (rows, cols);
    auto raw_it = raw.begin();
    for (uint32_t row = 0; row < rows; ++row) {
        for (uint32_t col = 0; col < cols; ++col, ++raw_it)
            ret (row, col) = *raw_it;
    }
    return ret;
}


// TODO: keys and search: we might be able to decode things without using
// all repair symbols! so the cached mtx could have less symbols than the
// working one


// Track precomputed matrices.
// we need to know the matrix size
// and the bitmask of the repair symbols.
// although RaptorQ can go much further, we only cache up to the 2^16th
// repair symbol.
//   "lost" field is the number of the soure symbols lost.
//   it's used to have a quick way to distinguish cache keys
//   without parsing the whole bitmask.
class RAPTORQ_API Cache_Key
{
public:
    Cache_Key (const uint16_t matrix_size, const uint16_t lost,
                    const uint32_t repair, const std::vector<bool> &lost_mask,
                                        const std::vector<bool> &repair_mask)
        :  _lost(lost), _mt_size (matrix_size), _repair (repair),
                        _lost_bitmask (lost_mask), _repair_bitmask (repair_mask)
    {}
    uint16_t _lost;
    uint16_t _mt_size;
    uint32_t _repair;
    std::vector<bool> _lost_bitmask;
    std::vector<bool> _repair_bitmask;

    bool operator< (const Cache_Key &rhs) const
    {
        if (_mt_size < rhs._mt_size)
            return true;
        if (_mt_size > rhs._mt_size)
            return false;
        // _mt_size == rhs._mt_size
        if (_lost < rhs._lost)
            return true;
        if (_lost > rhs._lost)
            return false;
        // _lost == rhs._lost
        if (_lost_bitmask.size() < rhs._lost_bitmask.size())
            return true;
        if (_lost_bitmask.size() > rhs._lost_bitmask.size())
            return false;
        // _lost_bitmask.size() == rhs._lost_bitmask.size()
        int32_t idx = static_cast<int32_t> (_lost_bitmask.size() - 1);
        for (; idx >= 0; --idx) {
            uint32_t i = static_cast<uint32_t> (idx);
            if (rhs._lost_bitmask[i] == false &&
                    _lost_bitmask[i] == true) {
                return false;
            }
            if (rhs._lost_bitmask[i] == true &&
                    _lost_bitmask[i] == false) {
                return true;
            }
        }
        // _lost_bitmask == rhs.lost_bitmask
        if (_repair_bitmask.size() < rhs._repair_bitmask.size())
            return true;
        if (_repair_bitmask.size() > rhs._repair_bitmask.size())
            return false;
        // _repair_bitmask.size() ==  rhs._repair_bitmask.size()
        idx = static_cast<int32_t> (_repair_bitmask.size() - 1);
        for (; idx >= 0; --idx) {
            uint32_t i = static_cast<uint32_t> (idx);
            if (rhs._repair_bitmask[i] == false &&
                    _repair_bitmask[i] == true) {
                return false;
            }
            if (rhs._repair_bitmask[i] == true &&
                    _repair_bitmask[i] == false) {
                return true;
            }
        }
        // all the same
        return false;
    }
    bool operator== (const Cache_Key &rhs) const
    {
        return _mt_size == rhs._mt_size && _lost == rhs._lost &&
                _repair == rhs._repair &&
                            _lost_bitmask.size() == rhs._lost_bitmask.size() &&
                                            _lost_bitmask == rhs._lost_bitmask &&
                            _repair_bitmask.size() == rhs._repair_bitmask.size() &&
                                            _repair_bitmask == rhs._repair_bitmask;
    }

    uint32_t out_size() const
        { return (_mt_size - _lost) + _repair; }
};


///////////////////////////////////////////////////////////////////////
// Decaying Least Frequency algorithm.
//   "Normal" LFU just tracks the most frequently used, so if an item
//   is used a lot at the beginning, and never again, it will still sit
//   at the top of the cache.
// The idea behind the "Decaying" LF is to slowly drop the hit count
// for all the cached elements.
//  The basic idea is based on "ticks" (representing access time)
//   * global tick, increased every hit.
//   * each element has 2 ticks:
//     * last tick
//     * score: last tick + appropriate increase for every hit
//  At every cache hit, the element score is increased by up to the number of
//  elements in the cache, depending on how close
//  last_tick was to the global_tick. Closer means higher frequency access,
//  so a higher lesser will happen to avoid starvation in case where
//  one element is accessed too many times and then forgotten
///////////////////////////////////////////////////////////////////////


// NOTE: no locking!! it is done in the memory section.

template<typename User_Data, typename Key>
class RAPTORQ_LOCAL DLF
{
public:
    DLF (const DLF&) = delete;
    DLF& operator= (const DLF&) = delete;
    DLF (DLF &&) = delete;
    DLF& operator= (DLF &&) = delete;

    static inline DLF *get()
    {
        // just drop the cache on exit time. Don't bother destroying
        // it correctly
        static DLF<User_Data, Key> *instance = new DLF<User_Data, Key>();
        return instance;
    }

    size_t get_size() const;
    size_t resize (const size_t new_size);
    bool add (const Compress algo, User_Data &raw, const Key &key);
    std::pair<Compress, User_Data> get (const Key &key);
private:
    DLF ();
    // keep everything in a linked list, ordered by score.
    // later on we might more to a b-tree, keep it simple for now.
    // TODO: inefficient. All of this.
    // not even using "ghost" elements (tracking requests of non-cached elements
    // FIXME: test-only. Doesn't take into account overflow.

    class DLF_Data
    {
    public:
        DLF_Data (const DLF_Data &d);
        DLF_Data (const Key k, const uint32_t _score, const uint32_t _tick,
                                        User_Data &_raw, const Compress alg)
            : key (k), score (_score), tick(_tick), raw(std::move(_raw)),
              algorithm (alg)
        {}
        Key key;
        uint32_t score;
        uint32_t tick;
        User_Data raw;
        Compress algorithm;

        bool operator< (const DLF_Data &rhs) const;
        DLF_Data& operator= (const DLF_Data &rhs);
    };
    std::mutex biglock;
    // works without atomic, we lock everything anyway.
    // which is bad, reaally. using atomic would be better, but more complex
    std::atomic<uint32_t> global_tick;
    size_t actual_size;
    size_t max_size;
    std::vector<DLF_Data> data;

    void test_and_reset_scores();
    void update_element (DLF_Data &el);
};


template<typename User_Data, typename Key>
DLF<User_Data, Key>::DLF_Data::DLF_Data (const DLF_Data &d)
    :key (d.key)
{
    algorithm = d.algorithm;
    score = d.score;
    tick = d.tick;
    raw = d.raw;
}

template<typename User_Data, typename Key>
typename DLF<User_Data, Key>::DLF_Data&
   DLF<User_Data, Key>::DLF_Data::operator= (const DLF_Data &d)
{
    key = d.key;
    algorithm = d.algorithm;
    score = d.score;
    tick = d.tick;
    raw = d.raw;

    return *this;
}

template<typename User_Data, typename Key>
bool DLF<User_Data, Key>::DLF_Data::operator< (const DLF_Data &rhs) const
    { return score < rhs.score; }

template<typename User_Data, typename Key>
DLF<User_Data, Key>::DLF()
{
    global_tick = 0;
    actual_size = 0;
    max_size = 0;
}

template<typename User_Data, typename Key>
size_t DLF<User_Data, Key>::get_size() const
    { return max_size; }

template<typename User_Data, typename Key>
size_t DLF<User_Data, Key>::resize (const size_t new_size)
{
    std::lock_guard<std::mutex> guard (biglock);
    RQ_UNUSED (guard);
    while (actual_size > new_size) {
        auto r_it = data.rbegin();
        assert (r_it != data.rend() && "RQ: DLF: r_it should have data.");
        actual_size -= (sizeof(DLF_Data) + r_it->raw.size());
        data.pop_back();
    }
    max_size = new_size;
    return max_size;
}

template<typename User_Data, typename Key>
void DLF<User_Data, Key>::test_and_reset_scores()
{
    // we overflowed. play it safe. reset all scores from zero.
    uint32_t g_tick = global_tick.load();
    if (g_tick != 0)
        return;
    for (auto &el : data) {
        const uint32_t abs_score = el.score - el.tick;
        const uint32_t till_overflow =
                                std::numeric_limits<uint32_t>::max() - el.tick;
        if (abs_score > till_overflow) {
            el.score = abs_score - till_overflow;
        } else {
            el.score = 0;
        }
        el.tick = 0;
    }
}

template<typename User_Data, typename Key>
void DLF<User_Data, Key>::update_element (
                                    typename DLF<User_Data, Key>::DLF_Data &el)
{
    // update the score & timers on the given element
    auto g_tick = ++global_tick;
    test_and_reset_scores();
    uint32_t tick_diff = g_tick - el.tick;
    uint32_t abs_score = el.score - el.tick;
    if (abs_score < tick_diff) {
        abs_score = 0;
    } else {
        abs_score -= tick_diff;
    }
    el.tick = g_tick;
    el.score = el.tick + abs_score +
                                std::min (static_cast<uint32_t> (data.size()),
                                                                1 + tick_diff);
    std::sort(data.begin(), data.end());
}

template<typename User_Data, typename Key>
std::pair<Compress, User_Data> DLF<User_Data, Key>::get (const Key &key)
{
    std::lock_guard<std::mutex> guard (biglock);
    RQ_UNUSED(guard);
    for (auto &tmp : data) {
        if (tmp.key == key) {
            std::pair<Compress, User_Data> ret_data = {tmp.algorithm, tmp.raw};
            update_element (tmp);
            return ret_data;
        }
    }
    return {Compress::NONE, User_Data ()};
}

template<typename User_Data, typename Key>
bool DLF<User_Data, Key>::add (const Compress algorithm, User_Data &raw,
                                                                const Key &key)
{
    std::lock_guard<std::mutex> guard (biglock);
    RQ_UNUSED(guard);
    for (auto &tmp : data) {
        if (tmp.key == key) {
            update_element (tmp);
            return true;
        }
    }
    // key not present.
    if (max_size - actual_size > sizeof(DLF_Data) + raw.size()) {
        // free space is the best
        auto g_tick = ++global_tick;
        test_and_reset_scores();
        data.emplace_back (key, g_tick + data.size(), g_tick, raw, algorithm);
        std::sort (data.begin(), data.end());
        actual_size += sizeof(DLF_Data) + raw.size();
        return true;
    } else {
        // need to delete some element?
        auto g_tick = ++global_tick;
        test_and_reset_scores();
        size_t usable_bytes = max_size - actual_size;
        size_t delete_from_end = 0;
        for (auto r_it = data.rbegin(); r_it != data.rend() &&
                                usable_bytes < (sizeof(DLF_Data) + raw.size());
                                                                    ++r_it) {
            // score is less than tick. check for overflows
            if ((g_tick - r_it->tick) > (r_it->score - r_it->tick)) {
                ++delete_from_end;
                usable_bytes +=  sizeof(DLF_Data) + r_it->raw.size();
            } else {
                break;
            }
        }
        if (usable_bytes < (sizeof(DLF_Data) + raw.size())) {
            // can't delete enough cached items, the new item requires
            // too much space, and fresher elements are present.
            return false;
        }
        while (delete_from_end > 0) {
            data.pop_back();
            --delete_from_end;
        }
        data.emplace_back (key, g_tick + data.size(), g_tick, raw, algorithm);
        std::sort (data.begin(), data.end());
        actual_size -= usable_bytes;
        actual_size += sizeof(DLF_Data) + raw.size();
        return true;
    }
}


} // namespace Impl
} // namespace RaptorQ__v1
