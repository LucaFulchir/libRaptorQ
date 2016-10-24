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

#include "RaptorQ/v1/common.hpp"
#include <condition_variable>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace RFC6330__v1 {

bool RAPTORQ_API set_thread_pool (const size_t threads,
                                    const uint16_t max_block_concurrency,
                                    const RaptorQ__v1::Work_State exit_type);

enum class RAPTORQ_API Work_Exit_Status : uint8_t {
    DONE = 0,
    STOPPED = 1,
    REQUEUE = 2
};


namespace Impl {

// maximum times a single block can be decoded at the same time.
// the decoder can be launched multiple times with different combinations
// of repair symbols. This can be useful as the decoding is actually
// probabilistic, and dropping a set of repair symbols *MIGHT* make things
// decodable again.
// keep this low. 1, 2, 3 should be ok.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint16_t max_block_decoder_concurrency = 1;
#pragma GCC diagnostic pop

enum class RAPTORQ_LOCAL Work_State_Overlay : uint8_t {
        KEEP_WORKING = static_cast<uint8_t> (
                                    RaptorQ__v1::Work_State::KEEP_WORKING),
        ABORT_COMPUTATION = static_cast<uint8_t>(
                                    RaptorQ__v1::Work_State::ABORT_COMPUTATION),
        WAITING = 100
        };

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
class RAPTORQ_LOCAL Pool_Work
{
public:
    Work_Exit_Status virtual do_work (RaptorQ__v1::Work_State *state) = 0;
    virtual ~Pool_Work() {}
};
#pragma clang diagnostic pop

class RAPTORQ_API Thread_Pool
{
public:
    Thread_Pool(Thread_Pool const&) = delete;
    Thread_Pool(Thread_Pool&&) = delete;
    Thread_Pool& operator=(Thread_Pool const&) = delete;
    Thread_Pool& operator=(Thread_Pool &&) = delete;

    static Thread_Pool& get();

    size_t size();
    void resize_pool (const size_t size, const RaptorQ__v1::Work_State exit_t);
    bool add_work (std::unique_ptr<Pool_Work> work);

private:
    Thread_Pool();
    ~Thread_Pool();
    std::mutex _data_mtx, _pool_mtx;
    std::condition_variable _cond;
    // map will not invalidate references on add/delete.
    // pair (thread, &keep_working)
    using th_state = std::pair<std::thread, std::weak_ptr<Work_State_Overlay>>;
    std::list<th_state> _pool, _exiting;
    std::deque<std::unique_ptr<Pool_Work>> _queue;

    static void working_thread (Thread_Pool *obj,
                                    std::shared_ptr<Work_State_Overlay> state);
};

} // namespace Impl
} // namespave RFC6330__v1
