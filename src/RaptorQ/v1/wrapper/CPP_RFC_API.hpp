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
#include "RaptorQ/v1/wrapper/CPP_RFC_API_void.hpp"
#include "RaptorQ/v1/RFC_Iterators.hpp"
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

template <typename Rnd_It, typename Fwd_It = Rnd_It>
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
    size_t encode (Fwd_It &output, const Fwd_It end, const uint32_t &id);
    void free (const uint8_t sbn);
    uint8_t blocks() const;
    uint32_t block_size (const uint8_t sbn) const;
    uint16_t symbol_size() const;
    uint16_t symbols (const uint8_t sbn) const;
    uint32_t max_repair (const uint8_t sbn) const;
private:
    Impl::Encoder_void *_encoder;
};

////////////////////
//// Decoder
////////////////////

template <typename In_It, typename Fwd_It = In_It>
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

    void end_of_input (const uint8_t block);
    void end_of_input();

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
    void free (const uint8_t sbn);
    uint64_t bytes() const;
    uint8_t blocks() const;
    uint32_t block_size (const uint8_t sbn) const;
    uint16_t symbol_size() const;
    uint16_t symbols (const uint8_t sbn) const;
private:
    Impl::Decoder_void *_decoder;
};


///////////////////
//// Encoder
///////////////////


template <>
Encoder<uint8_t*>::Encoder (uint8_t* const data_from, uint8_t* const data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block)
{
    _encoder =  new Impl::Encoder_void (RaptorQ_type::RQ_ENC_8, data_from,
                                                    data_to, min_subsymbol_size,
                                                    symbol_size, max_sub_block);
}

template <>
Encoder<uint16_t*>::Encoder (uint16_t* const data_from, uint16_t* const data_to,
                                             const uint16_t min_subsymbol_size,
                                             const uint16_t symbol_size,
                                             const size_t max_sub_block)
{
    _encoder =  new Impl::Encoder_void (RaptorQ_type::RQ_ENC_16, data_from,
                                                 data_to, min_subsymbol_size,
                                                 symbol_size, max_sub_block);
}

template <>
Encoder<uint32_t*>::Encoder (uint32_t* const data_from, uint32_t* const data_to,
                                             const uint16_t min_subsymbol_size,
                                             const uint16_t symbol_size,
                                             const size_t max_sub_block)
{
    _encoder =  new Impl::Encoder_void (RaptorQ_type::RQ_ENC_32, data_from,
                                                 data_to, min_subsymbol_size,
                                                 symbol_size, max_sub_block);
}

template <>
Encoder<uint64_t*>::Encoder (uint64_t* const data_from, uint64_t* const data_to,
                                             const uint16_t min_subsymbol_size,
                                             const uint16_t symbol_size,
                                             const size_t max_sub_block)
{
    _encoder =  new Impl::Encoder_void (RaptorQ_type::RQ_ENC_64, data_from,
                                                 data_to, min_subsymbol_size,
                                                 symbol_size, max_sub_block);
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::Encoder (const Rnd_It data_from, const Rnd_It data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block)
{
    RQ_UNUSED (data_from);
    RQ_UNUSED (data_to);
    RQ_UNUSED (min_subsymbol_size);
    RQ_UNUSED (symbol_size);
    RQ_UNUSED (max_sub_block);
    static_assert (false,
            "RaptorQ: sorry, only uint8_t*, uint16_t*, uint32_t*, uint64_t*, "
                "supported for the linked library. For more please use the "
                                                        "header-only version");
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::~Encoder()
    { delete _encoder; }

template <typename Rnd_It, typename Fwd_It>
It::Encoder::Block_Iterator<Rnd_It, Fwd_It> Encoder<Rnd_It, Fwd_It>::begin()
    { return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (_encoder, 0); }

template <typename Rnd_It, typename Fwd_It>
const It::Encoder::Block_Iterator<Rnd_It, Fwd_It> Encoder<Rnd_It, Fwd_It>::end()
{
    if (_encoder == nullptr)
        return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
    return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (_encoder, blocks());
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::operator bool() const
{
    if (_encoder == nullptr)
        return false;
    return static_cast<bool> (*_encoder);
}

template <typename Rnd_It, typename Fwd_It>
RFC6330_OTI_Common_Data Encoder<Rnd_It, Fwd_It>::OTI_Common() const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->OTI_Common();
}

template <typename Rnd_It, typename Fwd_It>
RFC6330_OTI_Scheme_Specific_Data
                            Encoder<Rnd_It, Fwd_It>::OTI_Scheme_Specific() const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->OTI_Common();
}

#if __cplusplus >= 201103L
template <typename Rnd_It, typename Fwd_It>
std::future<std::pair<Error, uint8_t>> Encoder<Rnd_It, Fwd_It>::compute (
                                                            const Compute flags)
{
    if (_encoder != nullptr)
        return _encoder->compute (flags);
    std::promise<std::pair<Error, uint8_t>> ret;
    ret.set_value ({Error::INITIALIZATION, 0});
    return ret.get_future();
}
#endif

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::precompute_max_memory ()
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->precompute_max_memory();
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t esi,
                                                            const uint8_t sbn)
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->encode (output, end, esi, sbn);
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t &id)
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->encode (output, end, id);
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::free (const uint8_t sbn)
{
    if (_encoder == nullptr)
        return;
    return _encoder->free (sbn);
}

template <typename Rnd_It, typename Fwd_It>
uint8_t Encoder<Rnd_It, Fwd_It>::blocks() const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->blocks();
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::block_size (const uint8_t sbn) const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->block_size (sbn);
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols (const uint8_t sbn) const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->symbols (sbn);
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair (const uint8_t sbn) const
{
    if (_encoder == nullptr)
        return 0;
    return _encoder->max_repair (sbn);
}



////////////////////
//// Decoder
////////////////////

template <>
Decoder<uint8_t*>::Decoder (const RFC6330_OTI_Common_Data common,
                        const RFC6330_OTI_Scheme_Specific_Data scheme)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_8, common, scheme);
}

template <>
Decoder<uint16_t*>::Decoder (const RFC6330_OTI_Common_Data common,
                        const RFC6330_OTI_Scheme_Specific_Data scheme)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_16, common, scheme);
}

template <>
Decoder<uint32_t*>::Decoder (const RFC6330_OTI_Common_Data common,
                        const RFC6330_OTI_Scheme_Specific_Data scheme)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_32, common, scheme);
}

template <>
Decoder<uint64_t*>::Decoder (const RFC6330_OTI_Common_Data common,
                        const RFC6330_OTI_Scheme_Specific_Data scheme)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_64, common, scheme);
}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::Decoder (const RFC6330_OTI_Common_Data common,
                        const RFC6330_OTI_Scheme_Specific_Data scheme)
{
    RQ_UNUSED (common);
    RQ_UNUSED (scheme);;
    static_assert (false,
            "RaptorQ: sorry, only uint8_t*, uint16_t*, uint32_t*, uint64_t*, "
                "supported for the linked library. For more please use the "
                                                        "header-only version");
}

template <>
Decoder<uint8_t*>::Decoder (const uint64_t size,const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_8, size,
                                    symbol_size, sub_blocks, blocks, alignment);
}

template <>
Decoder<uint16_t*>::Decoder (const uint64_t size,const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_16, size,
                                    symbol_size, sub_blocks, blocks, alignment);
}

template <>
Decoder<uint32_t*>::Decoder (const uint64_t size,const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_32, size,
                                    symbol_size, sub_blocks, blocks, alignment);
}

template <>
Decoder<uint64_t*>::Decoder (const uint64_t size,const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
{
    _decoder = new Impl::Decoder_void (RaptorQ_type::RQ_DEC_64, size,
                                    symbol_size, sub_blocks, blocks, alignment);
}


template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::Decoder (const uint64_t size, const uint16_t symbol_size,
                                                     const uint16_t sub_blocks,
                                                     const uint8_t blocks,
                                                     const uint8_t alignment)
{
    RQ_UNUSED (size);
    RQ_UNUSED (symbol_size);
    RQ_UNUSED (sub_blocks);
    RQ_UNUSED (blocks);
    RQ_UNUSED (alignment);
    static_assert (false,
            "RaptorQ: sorry, only uint8_t*, uint16_t*, uint32_t*, uint64_t*, "
                "supported for the linked library. For more please use the "
                                                        "header-only version");
}

template <typename Rnd_It, typename Fwd_It>
Decoder<Rnd_It, Fwd_It>::~Decoder()
    { delete _decoder; }

template <typename Rnd_It, typename Fwd_It>
It::Decoder::Block_Iterator<Rnd_It, Fwd_It> Decoder<Rnd_It, Fwd_It>::begin()
    { return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (_decoder, 0); }

template <typename Rnd_It, typename Fwd_It>
const It::Decoder::Block_Iterator<Rnd_It, Fwd_It> Decoder<Rnd_It, Fwd_It>::end()
{
    if (_decoder == nullptr)
        return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (nullptr, 0);
    return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (_decoder, blocks());
}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::operator bool() const
{
    if (_decoder == nullptr)
        return false;
    return static_cast<bool> (*_decoder);
}

#if __cplusplus >= 201103L
template <typename In_It, typename Fwd_It>
std::future<std::pair<Error, uint8_t>> Decoder<In_It, Fwd_It>::compute (
                                                            const Compute flags)
{
    if (_decoder != nullptr)
        return _decoder->compute(flags);
    std::promise<std::pair<Error, uint8_t>> p;
    p.set_value ({Error::INITIALIZATION, 0});
    return p.get_future();
}

#endif

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::end_of_input (const uint8_t block)
{
    if (_decoder != nullptr)
        return _decoder->end_of_input (block);
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::end_of_input()
{
    if (_decoder != nullptr)
        return _decoder->end_of_input();
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start, const Fwd_It end,
                                                            const uint8_t skip)
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->decode_bytes (start, end, skip);
}

template <typename In_It, typename Fwd_It>
size_t Decoder<In_It, Fwd_It>::decode_block_bytes (Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn)
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->decode_block_bytes (start, end, skip, sbn);
}

template <typename In_It, typename Fwd_It>
typename Decoder<In_It, Fwd_It>::aligned_res
                                        Decoder<In_It, Fwd_It>::decode_aligned (
                                                            Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip)
{
    if (_decoder == nullptr)
        return {0, 0};
    return _decoder->decode_aligned (start, end, skip);
}

template <typename In_It, typename Fwd_It>
typename Decoder<In_It, Fwd_It>::aligned_res
                                Decoder<In_It, Fwd_It>::decode_block_aligned (
                                                            Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn)
{
    if (_decoder == nullptr)
        return {0, 0};
    return _decoder->decode_block_aligned (start, end, skip, sbn);
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
                                                            const uint32_t id)
{
    if (_decoder == nullptr)
        return Error::INITIALIZATION;
    return _decoder->add_symbol (start, end, id);
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
                                                            const uint32_t esi,
                                                            const uint8_t sbn)
{
    if (_decoder == nullptr)
        return Error::INITIALIZATION;
    return _decoder->add_symbol (start, end, esi, sbn);
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::free (const uint8_t sbn)
{
    if (_decoder != nullptr)
        return _decoder->free (sbn);
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::bytes() const
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->bytes();
}

template <typename In_It, typename Fwd_It>
uint8_t Decoder<In_It, Fwd_It>::blocks() const
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->blocks();
}

template <typename In_It, typename Fwd_It>
uint32_t Decoder<In_It, Fwd_It>::block_size (const uint8_t sbn) const
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->block_size (sbn);
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbol_size() const
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->symbol_size();
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbols (const uint8_t sbn) const
{
    if (_decoder == nullptr)
        return 0;
    return _decoder->symbols (sbn);
}


}   // namespace RFC6330__v1
