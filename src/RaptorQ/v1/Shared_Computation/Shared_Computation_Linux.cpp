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

#include "Flock_Guard.hpp"
#include "Shared_Computation_Linux.hpp"
#include <cassert>
#include <crono>
#include <fcntl.h>
#include <string>
#include <string.h> // memcpy
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


RaptorQ__v1::Impl::Shared_Computation::Shared_Computation ()
                    : _shared_mem (0), sem_id (0), shm (nullptr), sem(),
                    shm_main (nullptr), shm_data (nullptr), own_version (0)
{
    const std::string lockpath = get_lock_path();
    libRaptorQ_fd = open (libpath, O_RDONLY | O_CREAT, // mode: 644
                                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    // if libRaptorQ_fd == -1, no shared memory will be available
}

const std::string RaptorQ__v1::Impl::Shared_Computation::get_lock_path() const
{
    // FIXME: hardcoded lib path
    // linux: ? use /run/lock/$NAME-$VERSION.lock
    // *bsd : ? use as linux?
    // mac: ? http://stackoverflow.com/questions/1627998/retrieving-the-memory-map-of-its-own-process-in-os-x-10-5-10-6

    // FIXME: lock file must exists
    //  which is not granted by our initial check, as there are multiple
    //  calls to shmget, with different versions
    return RQ_SHMPATH;
}

void RaptorQ__v1::Impl::Shared_Computation::poller (Shared_Computation &comp)
                                                                        const
{
    // if libRaptorQ_fd == -1, no shared memory will be available
    if (comp.libRaptorQ_fd == -1)
        return;
    uint32_t old_tick;
    const uint8_t secs_wait = 5;
    const uint8_t secs_quick_poll = 1;
    const uint8_t max_retries = 4;
    uint8_t retries = max_retries;
    while (true) {
        // polly waits for 5 secs, then does its work.
        // but it needs to quickly exit in case the destructor is called.
        if (comp.keep_polling.try_lock_for (std::crono::seconds (secs_wait)))
            break;
        if (shm_main == nullptr)
            continue;
        auto main_size = shm_main->size;
        if (main_size == _shared_mem) {
            // data shm is the same size we wanted. Increase tick.
            ++shm_main->tick_all;
            ++shm_data->tick_owner;
            continue;
        } else if (main_size > _shared_mem) {
            // shm data is not our size. Do we have to downsize it?
            // let's decide the size with the other pollers
            if (old_tick == main_shm->tick_all) {
                --retries;
            } else {
                retries = max_retries;
                old_tick = main_shm->tick_all;
                continue;
            }
            if (retries == 0) {
                retries = max_retries;
                // ok, waited more than enough. Obviously some process exited.
                // let's see who has the next max size
                for (uint8_t i = 0; i < 4; ++i) {
                    Flock_Guard memlock (comp.libRaptorQ_fd);
                    if (shm_main->size > _shared_mem) {
                        if (shm_main->next_size < _shared_mem)
                            shm_main->next_size = _shared_mem;
                    }
                    memlock.early_unlock();
                    std::this_thread::sleep_for(
                                        std::crono::seconds (secs_quick_poll));
                }
                Flock_Guard memlock (comp.libRaptorQ_fd);
                if (shm_main->next_size == _shared_mem) {
                    // we won the resize match.
                    // TODO
                }
                memlock.early_unlock();
            }
        } else {
            // we need more size? This should not happen...
            // launch a resize?
        }
    }
    // the thread might be restarted again in the future
    comp.keep_polling.unlock();
}


void *RaptorQ__v1::Impl::Shared_Computation::get_shm (const uint64_t size,
                                                        const uint8_t version,
                                                        const bool create)
{
    assert(version != 0);
    auto lockpath = get_lock_path();
    // make sure the file we use as shm identifier exists
    auto fd = open (lockpath, O_RDONLY | O_CREAT, // mode: 644
                                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
        return nullptr;
    close (fd);
    auto shm_key = ftok (lockpath, version);
    uint32_t flag = 0;
    if (create)
        flag=IPC_EXCL;
    uint32_t shmid;
    if ((shmid = shmget(shm_key, size, IPC_CREAT | flag | 0666)) < 0)
        return nullptr;
    void *shm = (shmat (shmid, NULL, 0));
    if (shm == reinterpret_cast<void *> (-1))
        return nullptr;
    return shm;
}

bool RaptorQ__v1::Impl::Shared_Computation::real_resize_shared (
                    const uint64_t shared_mem,
                    const RaptorQ__v1::Impl::Shared_Computation::Resize resize_type)
{
    if (libRaptorQ_fd == -1)
        return false;

    std::lock_guard guard (lock);
    Flock_Guard (libRaptorQ_fd);

    if (shared_mem == 0 && _shared_mem != 0) {
        keep_polling.unlock();
        polly.join();
        _shared_mem = 0;
        drop_shared();
        return true;
    }

    // allocate new shm
    // NOTE:
    // track the versions of the shm data.
    // id 001 reserved for main segment. 2+ for data segment reallocations
    // id is only uint8_t, although api get int. watch for overflow.

    // allocate/get main shm
    if (shm_main == nullptr) {
        void *ret = get_shm (sizeof(Main_Shm), 1, false);
        if (ret == nullptr)
            return false;
        shm_main = reinterpret_cast<Main_Shm *> (ret);
    }

    // some other process might already have allocated more memory
    uint64_t mem = (shared_mem > shm_main->size ? shared_mem : shm_main->size);
    mem += sizeof(Data_Shm);

    if (shared_mem != 0) {
        uint8_t version = shm_main->version + 1;
        while (version <= 1)    // Only uint8_t. Can overflow.
            ++version;
        bool create_new = (shm_main->size < mem);
        // maybe some other process got stuck and we cycled all
        // the uint8_t? try some more times.
        void *new_data = nullptr;
        for (uint8_t retry = 0; retry < 5; ++retry) {
            new_data = get_shm (mem, version, create_new);
            if (new_data == nullptr) {
                // Only uint8_t. Can overflow. We might also be *really*
                // behind in versions, so skip own own number.
                while (version <= 1 || version == own_version)
                    ++version;
            } else {
                break;
            }
        }
        if (new_data == nullptr) {
            return false;
        }

        if (create_new) {
            // copy old data
            // TODO
        }
        // switch
        if (shm_data != nullptr)
            shmdt (reinterpret_cast<void *> (shm_data));
        shm_data = reinterpret_cast<Data_Shm *> (new_data);
    }
    _shared_mem = shared_mem;
    // make sure polly is started
    if (!polly.joinable())
        polly = std::thread (poller, this);
    return true;
}

void RaptorQ__v1::Impl::Shared_Computation::drop_shared()
{
    if (shm_data != nullptr) {
        shmdt (reinterpret_cast<void *> (shm_data));
        shm_data = nullptr;
    }
    if (shm_main != nullptr) {
        shmdt (reinterpret_cast<void *> (shm_main));
        shm_main = nullptr;
    }
}

RaptorQ__v1::Impl::Shared_Computation::~Shared_Computation ()
{
    std::lock_guard guard (lock);
    UNUSED(lock)
    if (_shared_mem != 0) {
        keep_polling.unlock();
        polly.join(); // FIXME: only if still running/joinable
        drop_shared();
    }
}

