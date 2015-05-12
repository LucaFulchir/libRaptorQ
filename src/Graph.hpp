/*
 * Copyright (c) 2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#ifndef RAPTORQ_GRAPH_HPP
#define RAPTORQ_GRAPH_HPP

#include "common.hpp"
#include <vector>

namespace RaptorQ {
namespace Impl {

class RAPTORQ_LOCAL Graph
{
public:
	explicit Graph (const uint16_t size);

	void connect (const uint16_t node_a, const uint16_t node_b);
	bool is_max (const uint16_t id) const;
private:
	uint16_t find (const uint16_t id) const;
	// pair: conected_nodes, representative.
	// remember make-union-find? use it to track connected components
	std::vector<std::pair<uint16_t, uint16_t>> connections;
	uint16_t max_connections = 1;
};

}	// namespace Impl
}	// namespace RaptorQ

#endif

