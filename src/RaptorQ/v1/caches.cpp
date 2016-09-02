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
namespace RaptorQ__v1 {

uint64_t shared_cache_size (const uint64_t shared_cache)
{
    return 0;
}

bool local_cache_size (const uint64_t local_cache)
{
    return RaptorQ__v1::Impl::DLF<std::vector<uint8_t>,
									RaptorQ__v1::Impl::Cache_Key>::
													get()->resize (local_cache);
}

uint64_t get_shared_cache_size()
{
    return 0;
}

uint64_t get_local_cache_size()
{
    return RaptorQ__v1::Impl::DLF<std::vector<uint8_t>,
									RaptorQ__v1::Impl::Cache_Key>::
															get()->get_size();
}

}   // namespace RaptorQ__v1
