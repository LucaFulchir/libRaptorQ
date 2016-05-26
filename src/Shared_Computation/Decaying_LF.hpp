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

#ifndef RAPTORQ_DLF_HPP
#define RAPTORQ_DLF_HPP

#include "../common.hpp"
#include "../Operation.hpp"
#include <algorithm>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>


namespace RaptorQ {
namespace Impl {


// easy compress from/to eigen matrix
std::vector<uint8_t> RAPTORQ_API Mtx_to_raw (const DenseMtx &mtx);
std::vector<uint8_t> RAPTORQ_API raw_compress (
											const std::vector<uint8_t> &raw);
std::vector<uint8_t> RAPTORQ_API compress_to_raw (
										const std::vector<uint8_t> &compressed);
DenseMtx RAPTORQ_API raw_to_Mtx (const std::vector<uint8_t> &raw,
														const uint16_t cols);




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
											const std::vector<bool> &repairs)
		:  _lost(lost), _mt_size (matrix_size), bitmask (repairs)
	{}
	uint16_t _lost;
	uint16_t _mt_size;
	std::vector<bool> bitmask;

	bool operator< (const Cache_Key &rhs) const;
	bool operator== (const Cache_Key &rhs) const;
};


///////////////////////////////////////////////////////////////////////
// Decaying Least Frequency algorithm.
//   "Normal" LFU just tracks the most frequently used, so if an item
//   is used a lot at the beginning, and never again, it will still sit
//   at the top of the cache.
// The idea behind the "Decaying" LF is to slowly drop the hit count
// for the cached elements.
//  The basic idea is based on "ticks" (representing access time)
//   * global tick, increased every hit.
//   * each element has 2 ticks:
//     * last access
//     * score: cache entry tick + appropriate increase for every hit
//  At every hit, the element score is increased by the number of
//  elements in the cache, plus a value depending on how close
//  last_tick was to the global_tick. Closer mean higher frequency access,
//  so a higher increase will happen.
///////////////////////////////////////////////////////////////////////


// NOTE: no locking!! it is done in the memory section.

template<typename User_Data, typename Key>
class RAPTORQ_LOCAL DLF
{
public:
    DLF (const DLF&) = delete; // Don't Implement
    DLF& operator= (const DLF&) = delete;// Don't implement
    DLF (DLF &&) = delete; // Don't Implement
    DLF& operator= (DLF &&) = delete;// Don't implement

    static DLF *get()
    {
		// just drop the cache on exit time. don't bother destroying
		// it correctly
        static DLF<User_Data, Key> *instance = new DLF<User_Data, Key>();
        return instance;
    }

    size_t get_size() const;
    bool resize (const size_t new_size);
    bool add (User_Data &raw, const Key &key);
    User_Data get (const Key &key);
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
															User_Data &_raw)
			: key (k), score (_score), tick(_tick), raw(_raw)
		{}
        Key key;
        std::atomic<uint32_t> score;
        std::atomic<uint32_t> tick;
        User_Data raw;

        bool operator< (const DLF_Data &rhs) const;
		DLF_Data& operator= (const DLF_Data &rhs);
    };
    std::mutex biglock;
    std::atomic<uint32_t> global_tick;
    size_t actual_size;
    size_t max_size;
    std::vector<DLF_Data> data;
};


template<typename User_Data, typename Key>
DLF<User_Data, Key>::DLF_Data::DLF_Data (const DLF_Data &d)
	:key (d.key)
{
	auto _score = d.score.load();
	auto _tick = d.tick.load();
	score = _score;
	tick = _tick;
	raw = d.raw;
}

template<typename User_Data, typename Key>
typename DLF<User_Data, Key>::DLF_Data&
   DLF<User_Data, Key>::DLF_Data::operator= (const DLF_Data &d)
{
	auto _score = d.score.load();
	auto _tick = d.tick.load();
	key = d.key;
	score = _score;
	tick = _tick;
	raw = d.raw;

	return *this;
}

template<typename User_Data, typename Key>
bool DLF<User_Data, Key>::DLF_Data::operator< (const DLF_Data &rhs) const
{
    return score > rhs.score;
}

template<typename User_Data, typename Key>
DLF<User_Data, Key>::DLF()
{
	global_tick = 0;
	max_size = 0;
}

template<typename User_Data, typename Key>
size_t DLF<User_Data, Key>::get_size() const
{
    return max_size;
}

template<typename User_Data, typename Key>
bool DLF<User_Data, Key>::resize (const size_t new_size)
{
    std::lock_guard<std::mutex> guard (biglock);
    while (actual_size < new_size) {
        uint32_t item = static_cast<uint32_t> (data.size());
        if (item == 0)
            break;
        --item;
        data.erase (data.begin() + item);
    }
    max_size = new_size;
    return true;
}

template<typename User_Data, typename Key>
User_Data DLF<User_Data, Key>::get (const Key &key)
{
    std::lock_guard<std::mutex> guard (biglock);
    for (auto &tmp : data) {
        if (tmp.key == key)
            return tmp.raw;
    }
    return User_Data ();
}

template<typename User_Data, typename Key>
bool DLF<User_Data, Key>::add (User_Data &raw, const Key &key)
{
    std::lock_guard<std::mutex> guard (biglock);
    for (auto &tmp : data) {
        if (tmp.key == key) {
            ++global_tick;
			if (tmp.score <= global_tick) {
				auto tick = global_tick.load();
                tmp.tick = tick;
			}
            tmp.score += static_cast<uint32_t> (data.size());
            std::sort(data.begin(), data.end());
            return true;
        }
    }
    // key not present.
    if (max_size < sizeof(DLF_Data) + raw.size()) {
        auto tmp_tick = global_tick.load();
		//DLF_Data tmp (key, tmp_tick + 1, tmp_tick, raw);
		//data.push_back(tmp);
		data.emplace_back (key, tmp_tick + 1, tmp_tick, raw);
        max_size += raw.size() + sizeof (DLF_Data);
        std::sort (data.begin(), data.end());
        actual_size += sizeof(RaptorQ::Impl::DLF<User_Data, Key>) + raw.size();
        return true;
    } else {
        auto last_item = data.size();
        if (last_item == 0)
            return false;
        --last_item;
        if (max_size - data[last_item].raw.size() < raw.size()) {
            ++global_tick;
            auto tmp_tick = global_tick.load();
            data[last_item].score = tmp_tick + 1;
            data[last_item].tick = tmp_tick;
            actual_size -= data[last_item].raw.size();
            data[last_item].raw = std::move(raw);
            max_size += raw.size() + sizeof (DLF_Data);
            std::sort (data.begin(), data.end());
            actual_size += sizeof(RaptorQ::Impl::DLF<User_Data, Key>) +
																	raw.size();
            return true;
        } else {
            return false;
        }
    }
}


} // namespace Impl
} // namespace RaptorQ

#endif
