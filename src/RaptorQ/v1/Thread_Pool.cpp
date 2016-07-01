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

#include "RaptorQ/v1/Thread_Pool.hpp"

namespace RFC6330__v1 {
namespace Impl {

Thread_Pool::Thread_Pool()
{
	resize_pool (std::thread::hardware_concurrency(),
									RaptorQ__v1::Work_State::ABORT_COMPUTATION);
}

size_t Thread_Pool::size()
{
	return pool.size();
}

Thread_Pool::~Thread_Pool()
{
	queue.clear();
	std::unique_lock<std::mutex> pool_lock (pool_mtx);
	std::unique_lock<std::mutex> data_lock (data_mtx);
	for (auto &th : pool) {
		auto locked = th.second.lock();
		if (locked != nullptr) {
			th.first.detach();
			*locked = static_cast<Work_State_Overlay> (
									RaptorQ__v1::Work_State::ABORT_COMPUTATION);
		}
	}
	cond.notify_all();	// unlock threads stuck on condition_wait
	data_lock.unlock();
	pool_lock.unlock();

	pool.clear();
}

void Thread_Pool::resize_pool (const size_t size,
										const RaptorQ__v1::Work_State exit_t)
{
    if (size == pool.size())
        return;
    std::lock_guard<std::mutex> guard_pool (pool_mtx);
    UNUSED(guard_pool);

	uint16_t x = 1;

    while (pool.size() > size) {
		std::lock_guard<std::mutex> guard_data (data_mtx);
		UNUSED(guard_data);
		auto it2 = pool.begin();
		for (; it2 != pool.end(); ++it2) {
			std::weak_ptr<Work_State_Overlay> sec = it2->second;
			std::shared_ptr<Work_State_Overlay> state = sec.lock();
			if (nullptr != state) {
				++x;
			} else {
				if (it2->first.joinable()) {
					++x;
				} else {
					++x;
				}
			}
		}
		auto it = pool.begin();
		for (; it != pool.end(); ++it) {
			// delete a thread that is still waiting
			std::weak_ptr<Work_State_Overlay> sec = it->second;
			std::shared_ptr<Work_State_Overlay> state = sec.lock();
			if (nullptr != state) {
				if (Work_State_Overlay::WAITING == *state) {
					it->first.detach();
					pool.erase (it);
					it = pool.begin();
					*state = static_cast<Work_State_Overlay> (exit_t);
					cond.notify_all();
					break;
				}
			} else {
				// thread terminated.
				// will never happen here, but whatever...
				assert (false);
				it->first.detach();
				pool.erase (it);
				break;
			}
		}
		if (pool.size() > size && it == pool.end()) {
			// all threads are busy, but we must terminate one :(
			auto end = pool.rbegin();
			auto locked = end->second.lock();
			if (nullptr != locked) {
				end->first.detach();
				pool.erase (end.base());
				*locked = static_cast<Work_State_Overlay> (exit_t);
			}
			cond.notify_all();
		}
    }
    while (pool.size() < size) {
        auto state = std::make_shared<Work_State_Overlay> (
											Work_State_Overlay::KEEP_WORKING);
        pool.emplace_back (std::thread (working_thread, this, state),
									std::weak_ptr<Work_State_Overlay> (state));
    }
}

bool Thread_Pool::add_work (std::unique_ptr<Pool_Work> work)
{
    if (pool.size() == 0)
        return false;

    std::unique_lock<std::mutex> lock_data (data_mtx);

    queue.push_back (std::move(work));
	cond.notify_one();

    return true;
}

void Thread_Pool::working_thread (Thread_Pool *obj,
									std::shared_ptr<Work_State_Overlay> state)
{
	while (Work_State_Overlay::KEEP_WORKING == *state) {
		std::unique_lock<std::mutex> lock_data (obj->data_mtx);
		if (Work_State_Overlay::KEEP_WORKING != *state)
			break;
		if (obj->queue.size() == 0) {
			*state = Work_State_Overlay::WAITING;
			obj->cond.wait (lock_data);
			if (Work_State_Overlay::WAITING != *state)	// => abort
				break;
			if (obj->queue.size() == 0)	{ // unlock intended for other threads?
				*state = Work_State_Overlay::KEEP_WORKING;
				lock_data.unlock();
				continue;
			}
			*state = Work_State_Overlay::KEEP_WORKING;
		}

		std::unique_ptr<Pool_Work> my_work = std::move(obj->queue.front());
		obj->queue.pop_front();
		lock_data.unlock();

		auto exit_stat = my_work->do_work (
					reinterpret_cast<RaptorQ__v1::Work_State *> (state.get()));

		switch (exit_stat) {
		case Work_Exit_Status::DONE:
			break;
		case Work_Exit_Status::STOPPED:
			lock_data.lock();
			obj->queue.push_front (std::move(my_work));
			obj->cond.notify_one();
			lock_data.unlock();
			break;
		case Work_Exit_Status::REQUEUE:
			lock_data.lock();
			obj->queue.push_back (std::move(my_work));
			obj->cond.notify_one();
			lock_data.unlock();
			break;
		}
	}
}

} // namespace Impl
} // namespave RaptorQ__v1
