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

#include "RaptorQ/v1/common.hpp"
#include <lz4.h>
#include <vector>

namespace RaptorQ__v1 {
namespace Impl {

enum class LZ4_t : uint8_t { ENCODER=0, DECODER=1 };

template<LZ4_t type>
class RAPTORQ_LOCAL LZ4
{
public:
    LZ4();
    ~LZ4();
    LZ4 (const LZ4&) = delete;
    LZ4& operator= (const LZ4&) = delete;
    LZ4 (LZ4&&) = default;
    LZ4& operator= (LZ4&&) = default;

    std::vector<uint8_t> encode (const std::vector<uint8_t> &in);
    std::vector<uint8_t> decode (const std::vector<uint8_t> &in);
private:
    void *stream;
};

template<LZ4_t type>
LZ4<type>::LZ4()
{
    if (type == LZ4_t::ENCODER) {
        stream = static_cast<void *> (LZ4_createStream());
    } else {
        stream = static_cast<void *> (LZ4_createStreamDecode());
    }
}

template<LZ4_t type>
LZ4<type>::~LZ4()
{
    if (type == LZ4_t::ENCODER) {
        LZ4_freeStream (static_cast<LZ4_stream_t *> (stream));
    } else {
        LZ4_freeStreamDecode (static_cast<LZ4_streamDecode_t *> (stream));
    }
}

template<LZ4_t type>
std::vector<uint8_t> LZ4<type>::encode (const std::vector<uint8_t> &in)
{
    std::vector<uint8_t> ret;
    if (type == LZ4_t::DECODER || in.size() == 0 ||
                                            in.size() >= LZ4_MAX_INPUT_SIZE) {
        return ret;
    }

    auto max_size = LZ4_compressBound (static_cast<int32_t> (in.size()));
    if (stream == nullptr || max_size == 0)
        return ret;

    ret.resize (static_cast<size_t> (max_size) + sizeof(uint32_t), 0);

    // put original size in the first 4 bytes, for decoding.
    // int32 is fine, LZ4 has a limit of 2^31 bytes.
    int32_t *size = reinterpret_cast<int32_t *> (ret.data());
    *size = static_cast<int32_t> (in.size());

    // now the compressed data.
    auto written = LZ4_compress_fast_continue (
                    static_cast<LZ4_stream_t *> (stream),
                    reinterpret_cast<const char *> (in.data()),
                    reinterpret_cast<char *> (ret.data()) + sizeof(int32_t),
                    static_cast<int32_t> (in.size()),
                    max_size, 1);

    ret.resize (static_cast<size_t> (written) + sizeof(int32_t));

    return ret;
}

template<LZ4_t type>
std::vector<uint8_t> LZ4<type>::decode (const std::vector<uint8_t> &in)
{
    std::vector<uint8_t> ret;
    if (type == LZ4_t::ENCODER || in.size() < sizeof(uint32_t))
        return ret;

    if (stream == nullptr)
        return ret;

    // get the original uncompresseed size:
    int32_t *orig_size = reinterpret_cast<int32_t *> (
                                            const_cast<uint8_t *> (in.data()));
    // now the compressed data.
    ret.reserve (static_cast<size_t> (*orig_size));
    ret.resize (static_cast<size_t> (*orig_size), 0);
    auto written = LZ4_decompress_safe_continue (
                static_cast<LZ4_streamDecode_t *> (stream),
                reinterpret_cast<const char *> (in.data()) + sizeof(uint32_t),
                reinterpret_cast<char *> (ret.data()),
                static_cast<int32_t> (in.size() - sizeof(uint32_t)),
                *orig_size);
    if (written <= 0) {
        return std::vector<uint8_t>();
    }
    ret.resize (static_cast<size_t> (written));

    return ret;
}


} // namepace Impl
} // namespace RaptorQ__v1
