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

#include <array>
#include <fstream>
#include <iostream>

//
// We work on the "ar" format. refer to
// http://en.wikipedia.org/wiki/Ar_%28Unix%29
// for more details.
// we ned to delete the timestamps and uids/gids in the archive.
//

uint64_t erase (const uint64_t offset, std::fstream &lib);
uint64_t erase (const uint64_t offset, std::fstream &lib)
{
    // ar header format:
    // offset, length, description
    // 0    16  File name, ASCII
    // 16   12  File modification timestamp, Decimal
    // 28   6   Owner ID, Decimal
    // 34   6   Group ID, Decimal
    // 40   8   File mode, Octal
    // 48   10  File size in bytes, Decimal
    // 58   2   File magic, 0x60 0x0A

    // we will use this a lot. this are right-padded.
    const char zero = 0x30;
    const char pad = 0x20;

    // first of all, archive creation timestamp goes to zero
    lib.seekp (static_cast<int64_t> (offset + 16));
    lib.put (zero);
    for (size_t i = 0; i < 11; ++i)
        lib.put (pad);

    // now the uid:
    lib.seekp (static_cast<int64_t> (offset + 28));
    lib.put (zero);
    for (size_t i = 0; i < 5; ++i)
        lib.put (pad);
    // and gid
    lib.seekp (static_cast<int64_t> (offset + 34));
    lib.put (zero);
    for (size_t i = 0; i < 5; ++i)
        lib.put (pad);

    // return the size:
    lib.seekg (static_cast<int64_t> (offset + 48));
    char size[10];
    lib.read (size, 10);
    for (int32_t i = 9; i >= 0; --i) {
        if (size[i] == pad)
            size[i] = '\0';
    }
    uint64_t ret = static_cast<uint64_t> (std::atol (size));
    if (ret % 2 != 0)
        ++ret;  // everything is aligned to uint16_t, but file size is the
                // actual file size.
    ret += 60;  // add header size
    return ret;
}

int main (const int argc, const char **argv)
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " static_library.a\n";
        return 1;
    }

    std::string filename (argv[1]);
    std::fstream lib;

    lib.open (filename);
    if (!lib.is_open()) {
        std::cout << "Can not open the library\n";
        std::cout << "Usage: " << argv[0] << " static_library.a\n";
        return 1;
    }

    // check the magic number for the file:
    std::string magic (8, '\0');
    lib.read (&magic[0], 8);
    if (magic.compare("!<arch>\n") != 0) {
        std::cout << "This is not a static library. Can't do anything here.\n";
        return 1;
    }

    // get file size
    lib.seekg (0, std::ios_base::end);
    uint64_t file_size = static_cast<uint64_t> (lib.tellg());

    // we have a static library. now start deleting the timestamps and uid/gid.
    uint64_t offset = 8;
    while (offset < file_size)
        offset += erase (offset, lib);



    lib.close();
    std::cout << "Static library archive is deterministic now.\n";
    return 0;
}
