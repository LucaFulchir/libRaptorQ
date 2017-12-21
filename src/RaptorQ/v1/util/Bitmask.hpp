/*
 * Copyright (c) 2016-2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include "RaptorQ/v1/util/div.hpp"
#include <vector>

namespace RaptorQ__v1 {
namespace Impl {

// track the bitmask of holes for received symbols.
// also make it easy to know how many non-repair symbols we are missing.
class RAPTORQ_LOCAL Bitmask
{
public:
    uint16_t _max_nonrepair;

    Bitmask (const uint16_t symbols)
        : _max_nonrepair(symbols)
    {
        _holes = _max_nonrepair;
        _mask = std::vector<bool> (_max_nonrepair, false);
    }
    ~Bitmask() = default;
    Bitmask (const Bitmask&) = default;
    Bitmask& operator= (const Bitmask&) = default;
    Bitmask (Bitmask&&) = default;
    Bitmask& operator= (Bitmask&&) = default;

    void add (const size_t id)
    {
        if (id >= _mask.size()) {
            _mask.reserve (id + 1);
            _mask.insert (_mask.end(), ((id + 1) - _mask.size()), false);
        }
        if (_mask[id])
            return;
        _mask[id] = true;
        if (id < _max_nonrepair)
            --_holes;
    }
    void drop (const size_t id)
    {
        if (id >= _mask.size() || !_mask[id])
            return;
        _mask[id] = false;
        if (id < _max_nonrepair)
            ++_holes;
    }
    bool exists (const size_t id) const
    {
        if (id >= _mask.size())
            return false;
        return _mask[id];
    }
    uint16_t get_holes () const
        { return _holes; }
    const std::vector<bool>& get_bitmask () const
        { return _mask; }

    void free()
    {
        _mask.clear();
        _mask.resize(0);
        _holes = 0;
    }

private:
    std::vector<bool> _mask;
    uint16_t _holes;
};

} // namespace Impl
} // namespace RaptorQ__v1
