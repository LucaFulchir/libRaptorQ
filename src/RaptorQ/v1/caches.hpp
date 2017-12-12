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
#include <vector>
#include <utility>

namespace RaptorQ__v1 {

RAPTORQ_API Compress supported_compressions();
RAPTORQ_API Compress get_compression();
RAPTORQ_API bool     set_compression (const Compress compression);

RAPTORQ_API size_t local_cache_size (const size_t local_cache);
RAPTORQ_API size_t get_local_cache_size();

namespace Impl {

RAPTORQ_API std::pair<Compress, std::vector<uint8_t>> compress (
                                            const std::vector<uint8_t> &data);
RAPTORQ_API std::vector<uint8_t> decompress (const Compress algorithm,
                                            const std::vector<uint8_t> &data);

} // namespace Impl

}   // namespace RaptorQ__v1

namespace RFC6330__v1 {

using RaptorQ__v1::supported_compressions;
using RaptorQ__v1::get_compression;
using RaptorQ__v1::set_compression;
using RaptorQ__v1::local_cache_size;
using RaptorQ__v1::get_local_cache_size;

} // namespace RFC6330__v1
