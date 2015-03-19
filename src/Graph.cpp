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

#include "Graph.hpp"

RaptorQ::Impl::Graph::Graph (const uint16_t size)
{
	connections.reserve (size);
	for (uint16_t i = 0; i < size; ++i)
		connections.emplace_back (1, i);
}

uint16_t RaptorQ::Impl::Graph::find (const uint16_t id) const
{
	uint16_t tmp = id;
	while (connections[tmp].second != tmp)
		tmp = connections[tmp].second;
	return tmp;
}

void RaptorQ::Impl::Graph::connect (const uint16_t node_a,
														const uint16_t node_b)
{
	uint16_t rep_a = find (node_a), rep_b = find (node_b);

	connections[rep_a] = {connections[rep_a].first + connections[rep_b].first,
																		rep_a};
	connections[rep_b] = connections[rep_a];
	if (node_a != rep_a)
		connections[node_a] = connections[rep_a];
	if (node_b != rep_b)
		connections[node_b] = connections[rep_a];

	if (max_connections < connections[rep_a].first) {
		max_connections = connections[rep_a].first;
	}
}

bool RaptorQ::Impl::Graph::is_max (const uint16_t id) const
{
	return max_connections == connections[find (id)].first;
}

