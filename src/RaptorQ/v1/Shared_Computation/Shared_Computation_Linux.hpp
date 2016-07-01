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

#include "../common.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace RaptorQ__v1 {
namespace Impl {

// architecture:
// one thread will keep polling the shared memory segment every 5 seconds.
// this will give time to sync on the new max size of the segment,
// regardless of the number of processes involved.
//
// Obviously spawn a thread only if shared memory is needed



struct RAPTORQ_LOCAL Main_Shm
{
    uint64_t size;
    uint64_t next_size;
    std::atomic<uint32_t> tick_all;
    uint8_t version;
}

struct RAPTORQ_LOCAL Data_Shm
{
    std::atomic<uint32_t> tick_owner;
}

class RAPTORQ_LOCAL Shared_Computation
{
public:
    enum class Resize : uint8_t { AT_LEAST=1, AT_MOST=2 };

    Shared_Computation (const Shared_Computation&) = delete; // Don't Implement
    Shared_Computation& operator= (const Shared_Computation&) = delete;
    Shared_Computation (Shared_Computation &&) = delete; // Don't Implement
    Shared_Computation& operator= (Shared_Computation &&) = delete;
    ~Shared_Computation();
    bool resize_shared (const uint64_t shared_mem, const Resize resize_type);
    bool resize_dedicated (const uint64_t shared_mem, const Resize resize_type);
    static Shared_Computation& get()
    {
        static Shared_Computation instance();
        return instance;
    }
private:
    uint64_t _shared_mem;
    uint64_t dedicated_mem;
    Main_Shm *shm_main;
    Data_Shm *shm_data;
    int32_t sem_id;
    uint8_t own_version;
    static int libRaptorQ_fd;   // used for flock
    static std::mutex lock;
    static std::thread polly;
    static std::mutex keep_polling; // used to quickly stop polling

    static void poller (Shared_Computation &comp) const;
    static const std::string get_lib_path () const;

    Shared_Computation();
    void *get_shm(const uint64_t size, const uint8_t version,const bool create);
    void drop_shared();
};


} // namespace Impl
} // namespace RaptorQ__v1

