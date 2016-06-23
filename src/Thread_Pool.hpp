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

#include "common.hpp"
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <thread>

namespace RaptorQ__v1 {

enum class RAPTORQ_API Work_Exit_Status : uint8_t {
	DONE = 0,
	STOPPED = 1,
	REQUEUE = 2
};

namespace Impl {


enum class RAPTORQ_LOCAL Work_State_Overlay : uint8_t {
		KEEP_WORKING = static_cast<uint8_t> (Work_State::KEEP_WORKING),
		ABORT_COMPUTATION = static_cast<uint8_t>(Work_State::ABORT_COMPUTATION),
		WAITING = 100
		};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
class RAPTORQ_API Pool_Work
{
public:
	Work_Exit_Status virtual do_work (Work_State *state) = 0;
	virtual ~Pool_Work() {}
};
#pragma clang diagnostic pop

class RAPTORQ_API Thread_Pool
{
public:
	static Thread_Pool& get()
	{
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wexit-time-destructors"
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wglobal-constructors"
		static Thread_Pool pool;
		#pragma clang diagnostic pop
		#pragma GCC diagnostic pop
		return pool;
	}

	size_t size();
    void resize_pool (const size_t size, const Work_State exit_t);
    bool add_work (std::unique_ptr<Pool_Work> work);

private:
	Thread_Pool();
    ~Thread_Pool();
    std::mutex data_mtx, pool_mtx;
	std::condition_variable cond;
    // map will not invalidate references on add/delete.
	// pair (thread, &keep_working)
	using th_state = std::pair<std::thread, std::weak_ptr<Work_State_Overlay>>;
    std::deque<th_state> pool;
    //std::deque<th_state> wait_for_exit;
	std::deque<std::unique_ptr<Pool_Work>> queue;

    static void working_thread (Thread_Pool *obj,
									std::shared_ptr<Work_State_Overlay> state);
};

} // namespace Impl
} // namespave RaptorQ__v1
