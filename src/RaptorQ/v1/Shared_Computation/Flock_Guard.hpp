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

namespace RaptorQ__v1 {
namespace Impl {

class RAPTORQ_LOCAL flock_Guard
{
public:
    Flock_Guard (int32_t fd);
    Flock_Guard (const Flock_Guard&) = delete;
    Flock_Guard& operator=(const Flock_Guard&) = delete;
    Flock_Guard (Flock_Guard&&) = default;
    Flock_Guard& operator=(Flock_Guard&&) = default;
    ~Flock_Guard();   // ensures unlock

    void early_unlock();    // helps in efficiency?
private:
    int32_t _fd;
    bool locked;
};

} // namespace Impl
} // namespace RaptorQ__v1

