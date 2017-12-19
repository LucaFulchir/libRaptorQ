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

#include "RaptorQ/v1/caches.hpp"
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#ifdef RQ_USE_LZ4
    #include "RaptorQ/v1/Shared_Computation/LZ4_Wrapper.hpp"
#endif

namespace RaptorQ__v1 {
namespace Impl {
static Compress compression = Compress::NONE;
} // namespace Impl

RQ_HDR_INLINE Compress supported_compressions()
{
#ifdef RQ_USE_LZ4
    return Compress::NONE | Compress::LZ4;
#else
    return Compress::NONE;
#endif
}

RQ_HDR_INLINE Compress get_compression()
    { return Impl::compression; }

RQ_HDR_INLINE bool set_compression (const Compress _compression)
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

RQ_HDR_INLINE size_t local_cache_size (const size_t local_cache)
{
    return RaptorQ__v1::Impl::DLF<std::vector<uint8_t>,
                                    RaptorQ__v1::Impl::Cache_Key>::
                                                get()->resize (local_cache);
}

RQ_HDR_INLINE size_t get_local_cache_size()
{
    return RaptorQ__v1::Impl::DLF<std::vector<uint8_t>,
                                    RaptorQ__v1::Impl::Cache_Key>::
                                                            get()->get_size();
}

namespace Impl {
RQ_HDR_INLINE std::pair<Compress, std::vector<uint8_t>> compress (
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

RQ_HDR_INLINE std::vector<uint8_t> decompress (const Compress algorithm,
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
