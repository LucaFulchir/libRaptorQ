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
#include <cassert>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>


RaptorQ__v1::Impl::Flock_Guard::Flock_Guard (int32_t fd)
    :_fd(fd)
{
    assert (fd != -1);
    int ret;
    do {
        // signals might interrupt us. kernel out of memory. whatever.
        ret = flock(_fd, LOCK_EX);  
    } while (ret != 0);
    locked = true;
}

RaptorQ__v1::Impl::Flock_Guard::early_unlock()
{
    if (locked)
        flock(_fd, LOCK_UN);
}

RaptorQ__v1::Impl::Flock_Guard::~Flock_Guard()
{
    early_unlock();
}


