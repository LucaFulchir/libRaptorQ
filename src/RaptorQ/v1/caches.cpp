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

#include "RaptorQ/v1/caches.hpp"
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#ifdef RQ_USE_LZ4
    #include "RaptorQ/v1/Shared_Computation/LZ4_Wrapper.hpp"
#endif


namespace RaptorQ__v1 {

namespace Impl {
static Compress compression = Compress::NONE;
} // namespace Impl

Compress supported_compressions()
{
#ifdef RQ_USE_LZ4
	return Compress::NONE | Compress::LZ4;
#else
	return Compress::NONE;
#endif
}

Compress get_compression()
{
	return Impl::compression;
}

bool set_compression (const Compress compression)
{
	switch (compression) {
	case Compress::NONE:
		Impl::compression = compression;
		return true;
	case Compress::LZ4:
#ifdef RQ_USE_LZ4
		Impl::compression = compression;
		return true;
#else
		return false;
#endif
	}
	return false;
}

size_t shared_cache_size (const size_t shared_cache)
{
    return 0;
}

bool local_cache_size (const size_t local_cache)
{
    return RaptorQ__v1::Impl::DLF<std::vector<uint8_t>,
									RaptorQ__v1::Impl::Cache_Key>::
												get()->resize (local_cache);
}

size_t get_shared_cache_size()
{
    return 0;
}

size_t get_local_cache_size()
{
    return RaptorQ__v1::Impl::DLF<std::vector<uint8_t>,
									RaptorQ__v1::Impl::Cache_Key>::
															get()->get_size();
}

namespace Impl {
std::pair<Compress, std::vector<uint8_t>> compress (
											const std::vector<uint8_t> &data)
{
	Compress algo = get_compression();
    if (algo == Compress::NONE)
		return {algo, data};
#ifdef RQ_USE_LZ4
    if (algo == Compress::LZ4) {
        LZ4<LZ4_t::ENCODER> lz4;
		return {algo, lz4.encode (data)};
    }
#endif
	return {Compress::NONE, std::vector<uint8_t>()};
}

std::vector<uint8_t> decompress (const Compress algorithm,
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


namespace RFC6330__v1 {
Compress supported_compressions()
{ return RaptorQ__v1::supported_compressions(); }

Compress get_compression()
{ return RaptorQ__v1::get_compression(); }

bool set_compression (const Compress compression)
{ return RaptorQ__v1::set_compression (compression); }

size_t shared_cache_size (const size_t shared_cache)
{ return RaptorQ__v1::shared_cache_size (shared_cache); }

bool local_cache_size (const size_t local_cache)
{ return RaptorQ__v1::local_cache_size (local_cache); }

size_t get_shared_cache_size()
{ return RaptorQ__v1::get_shared_cache_size(); }

size_t get_local_cache_size()
{ return RaptorQ__v1::get_local_cache_size(); }

} // namespace RFC6330__v1
