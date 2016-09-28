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
#include <vector>
#include <utility>

namespace RaptorQ__v1 {

Compress RAPTORQ_API supported_compressions();
Compress RAPTORQ_API get_compression();
bool     RAPTORQ_API set_compression (const Compress compression);

size_t RAPTORQ_API shared_cache_size (const size_t shared_cache);
bool RAPTORQ_API local_cache_size (const size_t local_cache);
size_t RAPTORQ_API get_shared_cache_size();
size_t RAPTORQ_API get_local_cache_size();

namespace Impl {

std::pair<Compress, std::vector<uint8_t>> RAPTORQ_API compress (
											const std::vector<uint8_t> &data);
std::vector<uint8_t> RAPTORQ_API decompress (const Compress algorithm,
											const std::vector<uint8_t> &data);

} // namespace Impl

}   // namespace RaptorQ__v1

namespace RFC6330__v1 {

// NOTE: Copy of the above. keep in sync!
Compress RAPTORQ_API supported_compressions();
Compress RAPTORQ_API get_compression();
bool     RAPTORQ_API set_compression (const Compress compression);
size_t RAPTORQ_API shared_cache_size (const size_t shared_cache);
bool RAPTORQ_API local_cache_size (const size_t local_cache);
size_t RAPTORQ_API get_shared_cache_size();
size_t RAPTORQ_API get_local_cache_size();

} // namespace RFC6330__v1
