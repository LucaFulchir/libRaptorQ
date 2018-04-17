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
#include "RaptorQ/v1/Decoder.hpp"
#include "RaptorQ/v1/Interleaver.hpp"

namespace RFC6330__v1 {
namespace Impl {

template <typename Fwd_It>
class RAPTORQ_LOCAL De_Interleaver
{
public:
    De_Interleaver() = delete;
    De_Interleaver (const De_Interleaver&) = default;
    De_Interleaver& operator= (const De_Interleaver&) = default;
    De_Interleaver (De_Interleaver&&) = default;
    De_Interleaver& operator= (De_Interleaver &&) = default;
    De_Interleaver (const RaptorQ__v1::Impl::DenseMtx *symbols,
                                                    const Partition sub_blocks,
                                                    const uint16_t max_esi,
                                                    const uint8_t alignment)
        :_symbols (symbols), _sub_blocks (sub_blocks), _max_esi (max_esi),
                                                                _al (alignment)
    {
        IS_FORWARD(Fwd_It, "RaptorQ__v1::Impl::De_Interleaver");
    }
    size_t operator() (Fwd_It &start, const Fwd_It end, const size_t max_bytes,
                            const uint8_t skip, const uint16_t from_esi = 0);
    std::vector<bool> symbols_to_bytes (const size_t block_bytes,
                                    const std::vector<bool> &real_syms) const;
private:
    const RaptorQ__v1::Impl::DenseMtx *_symbols;
    const Partition _sub_blocks;
    const uint16_t _max_esi;
    const uint8_t _al;
};

template <typename Fwd_It>
size_t De_Interleaver<Fwd_It>::operator() (Fwd_It &start, const Fwd_It end,
                                                    const size_t max_bytes,
                                                    const uint8_t skip,
                                                    const uint16_t from_esi)
{
    if (start == end)
        return 0;
    // return number of BYTES written
    size_t written = 0;
    int32_t byte = 0;
    uint32_t subsym_byte = 0;
    uint16_t esi = from_esi;
    uint16_t sub_blk = 0;
    uint16_t sub_sym_size = _al *(_sub_blocks.num(0) > 0 ? _sub_blocks.size(0) :
                                                      _sub_blocks.size(1));
    // if the Fwd_It::value_type is not aligned with the block size,
    // we need to skip a certain amount of data in the first output element
    // so that we do not overwrite the old data or add unnecessary zeros
    // to (start-1) during the decoding of the previous block.
    uint8_t offset_al = skip;
    using T = typename std::iterator_traits<Fwd_It>::value_type;
    T element = static_cast<T> (0);
    if (skip != 0) {
        assert (skip < sizeof(T) && "De_Interleaver: skip too big");
        uint8_t *p = reinterpret_cast<uint8_t *> (&*start);
        for (size_t keep = 0; keep < skip; ++keep) {
            element += static_cast<T> (*(p++)) << keep * 8;
        }
    }
    while ((start != end && sub_blk < (_sub_blocks.num(0) + _sub_blocks.num(1)))
                            && (written + offset_al) < (max_bytes + skip)) {
        element += static_cast<T> (static_cast<uint8_t>((*_symbols)(esi, byte)))
                                                            << offset_al * 8;
        ++offset_al;
        if (offset_al == sizeof(T)) {
            *start = element;
            ++start;
            written += sizeof(T);
            element = static_cast<T> (0);
            offset_al = 0;
        }
        ++byte;
        ++subsym_byte;
        if (subsym_byte == sub_sym_size) {
            subsym_byte = 0;
            ++esi;
            if (esi >= _max_esi) {
                esi = 0;
                ++sub_blk;
            }
            if (sub_blk < _sub_blocks.num (0)) {
                sub_sym_size = _sub_blocks.size (0) * _al;
                byte = sub_sym_size * sub_blk;
            } else {
                sub_sym_size = _sub_blocks.size (1) * _al;
                byte = _sub_blocks.tot (0) * _al +
                                sub_sym_size * (sub_blk - _sub_blocks.num (0));
            }
        }
    }
    assert(!(start == end && offset_al != 0) && "De_Interleaver: can't write");
    if (start != end && offset_al != 0) {
        // we have more stuff in "element", but not enough to fill
        // the iterator. Do not overwrite additional data of the iterator.
        uint8_t *out = reinterpret_cast<uint8_t *> (&*start);
        uint8_t *in = reinterpret_cast<uint8_t *> (&element);
        for (size_t idx = 0; idx < offset_al; ++idx, ++out, ++in)
            *out = *in;
        ++start;
        written += offset_al;
    }
    written -= skip;
    assert (written <= max_bytes && "De_Interleaver: too much writing");
    return written;
}

template <typename Fwd_It>
std::vector<bool> De_Interleaver<Fwd_It>::symbols_to_bytes (
                                    const size_t block_bytes,
                                    const std::vector<bool> &real_syms) const
{
    std::vector<bool> ret;
    if (real_syms.size() != _symbols->rows())
        return ret;

    enum class  syms_state : uint8_t {
        BOTH = 0x00,
        ONLY_TRUE = 0x01,
        ONLY_FALSE = 0x02
    };

    // try to be smart. Do not check each byte if we only have true/false
    syms_state be_quick = real_syms[0] ? syms_state::ONLY_TRUE :
                                                        syms_state::ONLY_FALSE;
    for (const auto b : real_syms) {
        if (be_quick == syms_state::ONLY_TRUE) {
            if (!b) {
                be_quick = syms_state::BOTH;
                break;
            }
        } else {
            // ONLY_FALSE
            be_quick = syms_state::BOTH;
            break;
        }
    }

    switch (be_quick) {
    case syms_state::BOTH:
        break;
    case syms_state::ONLY_FALSE:
        ret.resize (block_bytes, false);
        return ret;
    case syms_state::ONLY_TRUE:
        ret.resize (block_bytes, true);
        return ret;
    }

    ret.resize (block_bytes, false);

    uint32_t byte = 0;
    uint32_t subsym_byte = 0;
    uint16_t esi = 0;
    uint16_t sub_blk = 0;
    uint16_t sub_sym_size = _al *(_sub_blocks.num(0) > 0 ? _sub_blocks.size(0) :
                                                      _sub_blocks.size(1));
    bool we_have_current_symbol = real_syms[esi];

    while (byte != block_bytes) {
        ret[byte] = we_have_current_symbol;
        ++byte;
        ++subsym_byte;
        if (subsym_byte == sub_sym_size) {
            subsym_byte = 0;
            ++esi;
            we_have_current_symbol = real_syms[esi];
            if (esi >= _max_esi) {
                esi = 0;
                ++sub_blk;
            }
            if (sub_blk < _sub_blocks.num (0)) {
                sub_sym_size = _sub_blocks.size (0) * _al;
            } else {
                sub_sym_size = _sub_blocks.size (1) * _al;
            }
        }
    }
    return ret;
}


}   // namespace Impl
}   // namespace RFC6330__v1
