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
#include "RaptorQ/v1/util/div.hpp"
#include <vector>

namespace RaptorQ__v1 {
namespace Impl {

// track the bitmask of holes for received symbols.
// also make it easy to know how many non-repair symbols we are missing.
class RAPTORQ_LOCAL Bitmask
{
public:
    const uint16_t _max_nonrepair;

    Bitmask (const uint16_t symbols)
        : _max_nonrepair(symbols)
    {
        _holes = _max_nonrepair;
        size_t max_element = div_ceil<size_t> (_max_nonrepair, sizeof(size_t));
        _mask.reserve(max_element + 1);
        for (size_t i = 0; i <= max_element; ++i)
            _mask.push_back(0);
    }

    void add (const size_t id)
    {
        size_t element = div_floor<size_t> (id, sizeof(size_t));
        while (element >= _mask.size())
            _mask.push_back(0);
        if (exists(id))
            return;

        size_t add_mask = 1 << (id - (element * sizeof(size_t)));
        _mask[element] |= add_mask;
        if (id < _max_nonrepair)
            --_holes;
    }
    void drop (const size_t id)
    {
        size_t element = div_floor<size_t> (id, sizeof(size_t));
        if (element >= _mask.size())
            return;
        if (!exists(id))
            return;

        size_t drop_mask = 1 << (id - (element * sizeof(size_t)));
        _mask[element] &= ~drop_mask;
        if (id < _max_nonrepair)
            ++_holes;
    }
    bool exists (const size_t id) const
    {
        size_t element = div_floor<size_t> (id, sizeof(size_t));
        if (element >= _mask.size())
            return false;

        size_t check_mask = 1 << (id - (element * sizeof(size_t)));
        return (_mask[element] & check_mask) != 0;
    }
    uint16_t get_holes () const
        { return _holes; }

    void free()
    {
        _mask.clear();
        _mask.resize(0);
        _holes = 0;
    }

private:
    // NOTE: this is not just vector<bool> 'cause this gives us a bit more
    // breating space and less reallocations, but really, why don't we just
    // use vector<bool> with more allocated space?
    std::vector<size_t> _mask;
    uint16_t _holes;
};

} // namespace Impl
} // namespace RaptorQ__v1
