/*
 * Copyright (c) 2015-2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include <vector>

namespace RaptorQ__v1 {
namespace Impl {

class RAPTORQ_LOCAL Graph
{
public:
    explicit Graph (const uint16_t size)
    {
        _connections.reserve(size);
        for (uint16_t i = 0; i < size; ++i)
            _connections.emplace_back(1, i);
    }

    void connect (const uint16_t node_a, const uint16_t node_b)
    {
        uint16_t rep_a = find(node_a), rep_b = find(node_b);

        _connections[rep_a] = { _connections[rep_a].first +
                                _connections[rep_b].first, rep_a };
        _connections[rep_b] = _connections[rep_a];
        if (node_a != rep_a)
            _connections[node_a] = _connections[rep_a];
        if (node_b != rep_b)
            _connections[node_b] = _connections[rep_a];

        if (_max_connections < _connections[rep_a].first) {
            _max_connections = _connections[rep_a].first;
        }
    }
    bool is_max (const uint16_t id) const
        { return _max_connections == _connections[find(id)].first; }
private:
    uint16_t find (const uint16_t id) const
    {
        uint16_t tmp = id;
        while (_connections[tmp].second != tmp)
            tmp = _connections[tmp].second;
        return tmp;
    }

    // pair: conected_nodes, representative.
    // remember make-union-find? use it to track connected components
    std::vector<std::pair<uint16_t, uint16_t>> _connections;
    uint16_t _max_connections = 1;
};

}   // namespace Impl
}   // namespace RaptorQ
