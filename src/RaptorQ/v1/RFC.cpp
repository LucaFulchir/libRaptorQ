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

#include "RaptorQ/v1/API_Wrapper.hpp"
#include "RaptorQ/v1/caches.hpp"
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"

namespace RFC6330__v1 {

bool set_thread_pool (const size_t threads,
										const uint16_t max_block_concurrency,
										const RaptorQ__v1::Work_State exit_type)
{
	if (max_block_concurrency == 0 || max_block_concurrency > threads)
		return false;
	Impl::max_block_decoder_concurrency = max_block_concurrency;
	Impl::Thread_Pool::get().resize_pool (threads, exit_type);
	return true;
}

}   // namespace RFC6330__v1
