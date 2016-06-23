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

#include "Thread_Pool.hpp"

namespace RaptorQ__v1 {
namespace Impl {

Thread_Pool::Thread_Pool()
{
	resize_pool (std::thread::hardware_concurrency(),
												Work_State::ABORT_COMPUTATION);
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
			//FIXME: pool.erase (th); NOW?
			*locked = static_cast<Work_State_Overlay> (
												Work_State::ABORT_COMPUTATION);
		}
	}
//	for (auto &th : wait_for_exit) {
//		auto locked = th.second.lock();
//		if (locked != nullptr) {
//			*locked = static_cast<Work_State_Overlay> (
//												Work_State::ABORT_COMPUTATION);
//		}
//	}
	cond.notify_all();	// unlock threads stuck on condition_wait
	data_lock.unlock();
	pool_lock.unlock();

	pool.clear();
	//wait_for_exit.clear();
}

void Thread_Pool::resize_pool (const size_t size, const Work_State exit_t)
{
    if (size == pool.size())
        return;
    std::lock_guard<std::mutex> guard_pool (pool_mtx);
    UNUSED(guard_pool);

    while (pool.size() > size) {
		std::lock_guard<std::mutex> guard_data (data_mtx);
		UNUSED(guard_data);
		auto it = pool.begin();
		for (; it != pool.end(); ++it) {
			// delete a thread that is still waiting
			auto sec = it->second;
			auto state = sec.lock();
			if (state != nullptr) {
				if (*state == Work_State_Overlay::WAITING) {
					it->first.detach();
					pool.erase (it);
					*state = static_cast<Work_State_Overlay> (exit_t);
					//wait_for_exit.emplace_back (std::move(*it));
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
		if (it == pool.end()) {
			// all threads are busy, but we must terminate one :(
			auto end = pool.rbegin();
			auto locked = end->second.lock();
			if (locked != nullptr) {
				end->first.detach();
				pool.erase (end.base());
				*locked = static_cast<Work_State_Overlay> (exit_t);
			}
			//wait_for_exit.emplace_back (std::move(*end));
			cond.notify_all();
			//pool.erase (end.base());
		}
    }
    while (pool.size() < size) {
        auto state = std::make_shared<Work_State_Overlay> (
											Work_State_Overlay::KEEP_WORKING);
        pool.emplace_back (std::thread (working_thread, this, state),
									std::weak_ptr<Work_State_Overlay> (state));
    }

	for (auto &wat : pool) {
		auto asd = wat.second.lock();
		if (asd == nullptr)
			;
	}
    // cleanup
//    for (auto it = wait_for_exit.begin(); it != wait_for_exit.end(); ++it) {
//		auto is_working = it->second.lock();
//		if (is_working == nullptr) {
//			wait_for_exit.erase (it);
//		}
//    }
}

bool Thread_Pool::add_work (std::unique_ptr<Pool_Work> work)
{
    if (pool.size() == 0)
        return false;

    std::unique_lock<std::mutex> lock_data (data_mtx);

    queue.push_back (std::move(work));
	cond.notify_one();
	lock_data.unlock();

	std::unique_lock<std::mutex> pool_lock (pool_mtx);
	// cleanup expired threads
//	for (auto it = wait_for_exit.begin(); it != wait_for_exit.end(); ++it) {
//		auto is_working = it->second.lock();
//		if (is_working == nullptr) {
//			wait_for_exit.erase (it);
//		}
//    }

    return true;
}

void Thread_Pool::working_thread (Thread_Pool *obj,
									std::shared_ptr<Work_State_Overlay> state)
{
	while (*state == Work_State_Overlay::KEEP_WORKING) {
		std::unique_lock<std::mutex> lock_data (obj->data_mtx);
		if (*state != Work_State_Overlay::KEEP_WORKING)
			break;
		if (obj->queue.size() == 0) {
			*state = Work_State_Overlay::WAITING;
			obj->cond.wait (lock_data);
			if (*state != Work_State_Overlay::WAITING)	// => abort
				break;
			if (obj->queue.size() == 0)	{ // unlock intended for other threads?
				lock_data.unlock();
				continue;
			}
			*state = Work_State_Overlay::KEEP_WORKING;
		}

		std::unique_ptr<Pool_Work> my_work = std::move(obj->queue.front());
		obj->queue.pop_front();
		lock_data.unlock();

		auto exit_stat = my_work->do_work (
								reinterpret_cast<Work_State *> (state.get()));

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
