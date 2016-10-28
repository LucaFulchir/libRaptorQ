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
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#ifdef RQ_USE_LZ4
    #include "RaptorQ/v1/Shared_Computation/LZ4_Wrapper.hpp"
#endif
#include <vector>
#include <utility>

namespace RaptorQ__v1 {

Compress RAPTORQ_API supported_compressions();
Compress RAPTORQ_API get_compression();
bool     RAPTORQ_API set_compression (const Compress compression);

size_t RAPTORQ_API shared_cache_size (const size_t shared_cache);
size_t RAPTORQ_API local_cache_size (const size_t local_cache);
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

using RaptorQ__v1::supported_compressions;
using RaptorQ__v1::get_compression;
using RaptorQ__v1::set_compression;
using RaptorQ__v1::shared_cache_size;
using RaptorQ__v1::local_cache_size;
using RaptorQ__v1::get_shared_cache_size;
using RaptorQ__v1::get_local_cache_size;

} // namespace RFC6330__v1


// Implementation

namespace RaptorQ__v1 {

namespace Impl {
static Compress compression = Compress::NONE;
} // namespace Impl

inline Compress supported_compressions()
{
#ifdef RQ_USE_LZ4
    return Compress::NONE | Compress::LZ4;
#else
    return Compress::NONE;
#endif
}

inline Compress get_compression()
    { return Impl::compression; }

inline bool set_compression (const Compress _compression)
{
    switch (_compression) {
    case Compress::NONE:
        Impl::compression = Compress::NONE;
        return true;
    case Compress::LZ4:
#ifdef RQ_USE_LZ4
        Impl::compression = Compress::LZ4;
        return true;
#else
        return false;
#endif
    }
    return false;
}

inline size_t shared_cache_size (const size_t shared_cache)
    { return 0; }

inline size_t local_cache_size (const size_t local_cache)
{
    return RaptorQ__v1::Impl::DLF<std::vector<uint8_t>,
                                    RaptorQ__v1::Impl::Cache_Key>::
                                                get()->resize (local_cache);
}

inline size_t get_shared_cache_size()
    { return 0; }

inline size_t get_local_cache_size()
{
    return RaptorQ__v1::Impl::DLF<std::vector<uint8_t>,
                                    RaptorQ__v1::Impl::Cache_Key>::
                                                            get()->get_size();
}

namespace Impl {
inline std::pair<Compress, std::vector<uint8_t>> compress (
                                            const std::vector<uint8_t> &data)
{
    if (Impl::compression == Compress::NONE)
        return {Compress::NONE, data};
#ifdef RQ_USE_LZ4
    if (Impl::compression == Compress::LZ4) {
        LZ4<LZ4_t::ENCODER> lz4;
        return {Compress::LZ4, lz4.encode (data)};
    }
#endif
    return {Compress::NONE, std::vector<uint8_t>()};
}

inline std::vector<uint8_t> decompress (const Compress algorithm,
                                            const std::vector<uint8_t> &data)
{
    if (algorithm == Compress::NONE)
        return data;
#ifdef RQ_USE_LZ4
    if (algorithm == Compress::LZ4) {
        LZ4<LZ4_t::DECODER> lz4;
        return lz4.decode (data);
    }
#endif
    return std::vector<uint8_t>();
}

} // namespace Impl
} // namespace RaptorQ__v1
