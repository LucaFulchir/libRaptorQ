/*
 * Copyright (c) 2015-2018, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include "RaptorQ/v1/wrapper/CPP_RFC_API_void.hpp"
#include "RaptorQ/v1/RFC_Iterators.hpp"
#include <vector>
#include <cmath>
#if __cplusplus >= 201103L || _MSC_VER > 1900
#include <future>
#endif

/////////////////////
//
//  These templates are just a wrapper around the
//  functionalities offered by the RaptorQ__v1::Impl namespace
//  So if you want to see what the algorithm looks like,
//  you are in the wrong place
//
/////////////////////

namespace RFC6330__v1 {

////////////////////
//// Encoder
////////////////////

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Encoder
{
public:
    Encoder (const Rnd_It data_from, const Rnd_It data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block);
    Encoder() = delete;
    Encoder (const Encoder&) = delete;
    Encoder& operator= (const Encoder&) = delete;
    Encoder (Encoder&&) = default;
    Encoder& operator= (Encoder&&) = default;
    ~Encoder();

    It::Encoder::Block_Iterator<Rnd_It, Fwd_It> begin();
    const It::Encoder::Block_Iterator<Rnd_It, Fwd_It> end();

    operator bool() const;
    RFC6330_OTI_Common_Data OTI_Common() const;
    RFC6330_OTI_Scheme_Specific_Data OTI_Scheme_Specific() const;

    #if __cplusplus >= 201103L || _MSC_VER > 1900
    std::future<std::pair<Error, uint8_t>> compute (const Compute flags);
    #endif

    size_t precompute_max_memory ();
    size_t encode (Fwd_It &output, const Fwd_It end, const uint32_t esi,
                                                            const uint8_t sbn);
    size_t encode (Fwd_It &output, const Fwd_It end, const uint32_t id);
    void free (const uint8_t sbn);
    uint8_t blocks() const;
    uint32_t block_size (const uint8_t sbn) const;
    uint16_t symbol_size() const;
    uint16_t symbols (const uint8_t sbn) const;
    Block_Size extended_symbols (const uint8_t sbn) const;
    uint32_t max_repair (const uint8_t sbn) const;
private:
    Impl::Encoder_void _encoder;
};

////////////////////
//// Decoder
////////////////////

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Decoder
{
public:
    Decoder (const RFC6330_OTI_Common_Data common,
                            const RFC6330_OTI_Scheme_Specific_Data scheme);

    Decoder (const uint64_t size, const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment);
    Decoder() = delete;
    Decoder (const Decoder&) = delete;
    Decoder& operator= (const Decoder&) = delete;
    Decoder (Decoder&&) = default;
    Decoder& operator= (Decoder&&) = default;
    ~Decoder();

    It::Decoder::Block_Iterator<In_It, Fwd_It> begin ();
    const It::Decoder::Block_Iterator<In_It, Fwd_It> end ();

    operator bool() const;

    #if __cplusplus >= 201103L || _MSC_VER > 1900
    std::future<std::pair<Error, uint8_t>> compute (const Compute flags);
    #endif

    std::vector<bool> end_of_input (const Fill_With_Zeros fill,
                                                        const uint8_t block);
    std::vector<bool> end_of_input (const Fill_With_Zeros fill);

    uint64_t decode_symbol (Fwd_It &start, const Fwd_It end, const uint16_t esi,
                                                            const uint8_t sbn);
    uint64_t decode_bytes (Fwd_It &start, const Fwd_It end, const uint8_t skip);
    size_t decode_block_bytes (Fwd_It &start, const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn);
    struct aligned_res
    {
        uint64_t written;
        uint8_t offset;
    };
    aligned_res decode_aligned (Fwd_It &start,const Fwd_It end,
                                                            const uint8_t skip);
    aligned_res decode_block_aligned (Fwd_It &start, const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn);
    Error add_symbol (In_It &start, const In_It end, const uint32_t id);
    Error add_symbol (In_It &start, const In_It end, const uint32_t esi,
                                                            const uint8_t sbn);
    uint8_t blocks_ready();
    bool is_ready();
    bool is_block_ready (const uint8_t block);
    void free (const uint8_t sbn);
    uint64_t bytes() const;
    uint8_t blocks() const;
    uint32_t block_size (const uint8_t sbn) const;
    uint16_t symbol_size() const;
    uint16_t symbols (const uint8_t sbn) const;
    Block_Size extended_symbols (const uint8_t sbn) const;
private:
    Impl::Decoder_void _decoder;
};


///////////////////
//// Encoder
///////////////////


template <>
inline Encoder<uint8_t*, uint8_t*>::Encoder (uint8_t* const data_from,
                                            uint8_t* const data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block)
    : _encoder (RaptorQ_type::RQ_ENC_8, data_from, data_to, min_subsymbol_size,
                                                    symbol_size, max_sub_block)
    {}

template <>
inline Encoder<uint16_t*, uint16_t*>::Encoder (uint16_t* const data_from,
                                            uint16_t* const data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block)
    : _encoder (RaptorQ_type::RQ_ENC_16, data_from, data_to, min_subsymbol_size,
                                                    symbol_size, max_sub_block)
    {}

template <>
inline Encoder<uint32_t*, uint32_t*>::Encoder (uint32_t* const data_from,
                                            uint32_t* const data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block)
    : _encoder (RaptorQ_type::RQ_ENC_32, data_from, data_to, min_subsymbol_size,
                                                    symbol_size, max_sub_block)
    {}

template <>
inline Encoder<uint64_t*, uint64_t*>::Encoder (uint64_t* const data_from,
                                            uint64_t* const data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block)
    : _encoder (RaptorQ_type::RQ_ENC_64, data_from, data_to, min_subsymbol_size,
                                                    symbol_size, max_sub_block)
    {}

template <typename Rnd_It, typename Fwd_It>
inline Encoder<Rnd_It, Fwd_It>::~Encoder()
    {}

template <typename Rnd_It, typename Fwd_It>
inline It::Encoder::Block_Iterator<Rnd_It, Fwd_It>
                                                Encoder<Rnd_It, Fwd_It>::begin()
    { return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (&_encoder, 0); }

template <typename Rnd_It, typename Fwd_It>
inline const It::Encoder::Block_Iterator<Rnd_It, Fwd_It>
                                                Encoder<Rnd_It, Fwd_It>::end()
    { return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (&_encoder, blocks());}

template <typename Rnd_It, typename Fwd_It>
inline Encoder<Rnd_It, Fwd_It>::operator bool() const
    { return static_cast<bool> (_encoder); }

template <typename Rnd_It, typename Fwd_It>
inline RFC6330_OTI_Common_Data Encoder<Rnd_It, Fwd_It>::OTI_Common() const
    { return _encoder.OTI_Common(); }

template <typename Rnd_It, typename Fwd_It>
inline RFC6330_OTI_Scheme_Specific_Data
                            Encoder<Rnd_It, Fwd_It>::OTI_Scheme_Specific() const
    { return _encoder.OTI_Scheme_Specific(); }

#if __cplusplus >= 201103L
template <typename Rnd_It, typename Fwd_It>
inline std::future<std::pair<Error, uint8_t>> Encoder<Rnd_It, Fwd_It>::compute (
                                                            const Compute flags)
    { return _encoder.compute (flags); }
#endif

template <typename Rnd_It, typename Fwd_It>
inline size_t Encoder<Rnd_It, Fwd_It>::precompute_max_memory ()
    { return _encoder.precompute_max_memory(); }

template <typename Rnd_It, typename Fwd_It>
inline size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t esi,
                                                            const uint8_t sbn)
{
    void **_from = reinterpret_cast<void**> (&output);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _encoder.encode (_from, _to, esi, sbn);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    output = *tmp;
    return ret;
}

template <typename Rnd_It, typename Fwd_It>
inline size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t id)
{
    void **_from = reinterpret_cast<void**> (&output);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _encoder.encode (_from, _to, id);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    output = *tmp;
    return ret;
}

template <typename Rnd_It, typename Fwd_It>
inline void Encoder<Rnd_It, Fwd_It>::free (const uint8_t sbn)
    { return _encoder.free (sbn); }

template <typename Rnd_It, typename Fwd_It>
inline uint8_t Encoder<Rnd_It, Fwd_It>::blocks() const
    { return _encoder.blocks(); }

template <typename Rnd_It, typename Fwd_It>
inline uint32_t Encoder<Rnd_It, Fwd_It>::block_size (const uint8_t sbn) const
    { return _encoder.block_size (sbn); }

template <typename Rnd_It, typename Fwd_It>
inline uint16_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
    { return _encoder.symbol_size(); }

template <typename Rnd_It, typename Fwd_It>
inline uint16_t Encoder<Rnd_It, Fwd_It>::symbols (const uint8_t sbn) const
    { return _encoder.symbols (sbn); }

template <typename Rnd_It, typename Fwd_It>
inline Block_Size Encoder<Rnd_It, Fwd_It>::extended_symbols (const uint8_t sbn)
                                                                        const
    { return _encoder.extended_symbols (sbn); }

template <typename Rnd_It, typename Fwd_It>
inline uint32_t Encoder<Rnd_It, Fwd_It>::max_repair (const uint8_t sbn) const
    { return _encoder.max_repair (sbn); }



////////////////////
//// Decoder
////////////////////

template <>
inline Decoder<uint8_t*, uint8_t*>::Decoder (
                                const RFC6330_OTI_Common_Data common,
                                const RFC6330_OTI_Scheme_Specific_Data scheme)
    : _decoder (RaptorQ_type::RQ_DEC_8, common, scheme)
    {}

template <>
inline Decoder<uint16_t*, uint16_t*>::Decoder (
                                const RFC6330_OTI_Common_Data common,
                                const RFC6330_OTI_Scheme_Specific_Data scheme)
    : _decoder (RaptorQ_type::RQ_DEC_16, common, scheme)
    {}

template <>
inline Decoder<uint32_t*, uint32_t*>::Decoder (
                                const RFC6330_OTI_Common_Data common,
                                const RFC6330_OTI_Scheme_Specific_Data scheme)
    : _decoder (RaptorQ_type::RQ_DEC_32, common, scheme)
    {}

template <>
inline Decoder<uint64_t*, uint64_t*>::Decoder (
                                const RFC6330_OTI_Common_Data common,
                                const RFC6330_OTI_Scheme_Specific_Data scheme)
    : _decoder (RaptorQ_type::RQ_DEC_64, common, scheme)
    {}

template <>
inline Decoder<uint8_t*, uint8_t*>::Decoder (const uint64_t size,
                                                    const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)

    : _decoder (RaptorQ_type::RQ_DEC_8, size,
                                    symbol_size, sub_blocks, blocks, alignment)
    {}

template <>
inline Decoder<uint16_t*, uint16_t*>::Decoder (const uint64_t size,
                                                    const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
    : _decoder (RaptorQ_type::RQ_DEC_16, size,
                                    symbol_size, sub_blocks, blocks, alignment)
    {}

template <>
inline Decoder<uint32_t*, uint32_t*>::Decoder (const uint64_t size,
                                                    const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
    : _decoder (RaptorQ_type::RQ_DEC_32, size,
                                    symbol_size, sub_blocks, blocks, alignment)
    {}

template <>
inline Decoder<uint64_t*, uint64_t*>::Decoder (const uint64_t size,
                                                    const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
    : _decoder (RaptorQ_type::RQ_DEC_64, size,
                                    symbol_size, sub_blocks, blocks, alignment)
    {}

template <typename Rnd_It, typename Fwd_It>
inline Decoder<Rnd_It, Fwd_It>::~Decoder()
    {}

template <typename Rnd_It, typename Fwd_It>
inline It::Decoder::Block_Iterator<Rnd_It, Fwd_It>
                                                Decoder<Rnd_It, Fwd_It>::begin()
    { return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (&_decoder, 0); }

template <typename Rnd_It, typename Fwd_It>
inline const It::Decoder::Block_Iterator<Rnd_It, Fwd_It>
                                                Decoder<Rnd_It, Fwd_It>::end()
    { return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (&_decoder, blocks()); }

template <typename In_It, typename Fwd_It>
inline Decoder<In_It, Fwd_It>::operator bool() const
    { return static_cast<bool> (_decoder); }

#if __cplusplus >= 201103L
template <typename In_It, typename Fwd_It>
inline std::future<std::pair<Error, uint8_t>> Decoder<In_It, Fwd_It>::compute (
                                                            const Compute flags)
{ return _decoder.compute(flags); }

#endif

template <typename In_It, typename Fwd_It>
inline std::vector<bool> Decoder<In_It, Fwd_It>::end_of_input (
                                                    const Fill_With_Zeros fill,
                                                    const uint8_t block)
    { return _decoder.end_of_input (fill, block); }

template <typename In_It, typename Fwd_It>
inline std::vector<bool> Decoder<In_It, Fwd_It>::end_of_input (
                                                    const Fill_With_Zeros fill)
    { return _decoder.end_of_input (fill); }

template <typename In_It, typename Fwd_It>
inline uint64_t Decoder<In_It, Fwd_It>::decode_symbol (Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint16_t esi,
                                                            const uint8_t sbn)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret =  _decoder.decode_symbol (_from, _to, esi, sbn);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    start = *tmp;
    return ret;
}

template <typename In_It, typename Fwd_It>
inline uint64_t Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret =  _decoder.decode_bytes (_from, _to, skip);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    start = *tmp;
    return ret;
}

template <typename In_It, typename Fwd_It>
inline size_t Decoder<In_It, Fwd_It>::decode_block_bytes (Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _decoder.decode_block_bytes (_from, _to, skip, sbn);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    start = *tmp;
    return ret;
}

template <typename In_It, typename Fwd_It>
inline typename Decoder<In_It, Fwd_It>::aligned_res
                                        Decoder<In_It, Fwd_It>::decode_aligned (
                                                            Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _decoder.decode_aligned (_from, _to, skip);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    start = *tmp;
    return {ret.written, ret.offset};
}

template <typename In_It, typename Fwd_It>
inline typename Decoder<In_It, Fwd_It>::aligned_res
                                Decoder<In_It, Fwd_It>::decode_block_aligned (
                                                            Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _decoder.decode_block_aligned (_from, _to, skip, sbn);
    Fwd_It *tmp = reinterpret_cast<Fwd_It*> (_from);
    start = *tmp;
    return {ret.written, ret.offset};
}

template <typename In_It, typename Fwd_It>
inline Error Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
                                                            const uint32_t id)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _decoder.add_symbol (_from, _to, id);
    In_It *tmp = reinterpret_cast<In_It*> (_from);
    start = *tmp;
    return ret;
}

template <typename In_It, typename Fwd_It>
inline Error Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
                                                            const uint32_t esi,
                                                            const uint8_t sbn)
{
    void **_from = reinterpret_cast<void**> (&start);
    void *_to = reinterpret_cast<void*> (end);
    auto ret = _decoder.add_symbol (_from, _to, esi, sbn);
    In_It *tmp = reinterpret_cast<In_It*> (_from);
    start = *tmp;
    return ret;
}

template <typename In_It, typename Fwd_It>
inline uint8_t Decoder<In_It, Fwd_It>::blocks_ready()
    { return _decoder.blocks_ready(); }

template <typename In_It, typename Fwd_It>
inline bool Decoder<In_It, Fwd_It>::is_ready()
    { return _decoder.is_ready(); }

template <typename In_It, typename Fwd_It>
inline bool Decoder<In_It, Fwd_It>::is_block_ready (const uint8_t sbn)
    { return _decoder.is_block_ready (sbn); }

template <typename In_It, typename Fwd_It>
inline void Decoder<In_It, Fwd_It>::free (const uint8_t sbn)
    { return _decoder.free (sbn); }

template <typename In_It, typename Fwd_It>
inline uint64_t Decoder<In_It, Fwd_It>::bytes() const
    { return _decoder.bytes(); }

template <typename In_It, typename Fwd_It>
inline uint8_t Decoder<In_It, Fwd_It>::blocks() const
    { return _decoder.blocks(); }

template <typename In_It, typename Fwd_It>
inline uint32_t Decoder<In_It, Fwd_It>::block_size (const uint8_t sbn) const
    { return _decoder.block_size (sbn); }

template <typename In_It, typename Fwd_It>
inline uint16_t Decoder<In_It, Fwd_It>::symbol_size() const
    { return _decoder.symbol_size(); }

template <typename In_It, typename Fwd_It>
inline uint16_t Decoder<In_It, Fwd_It>::symbols (const uint8_t sbn) const
    { return _decoder.symbols (sbn); }

template <typename In_It, typename Fwd_It>
inline Block_Size Decoder<In_It, Fwd_It>::extended_symbols (const uint8_t sbn)
                                                                        const
    { return _decoder.extended_symbols (sbn); }


}   // namespace RFC6330__v1
