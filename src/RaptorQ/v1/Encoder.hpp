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
#include "RaptorQ/v1/caches.hpp"
#include "RaptorQ/v1/Interleaver.hpp"
#include "RaptorQ/v1/multiplication.hpp"
#include "RaptorQ/v1/Parameters.hpp"
#include "RaptorQ/v1/Precode_Matrix.hpp"
#include "RaptorQ/v1/Rand.hpp"
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#include "RaptorQ/v1/Thread_Pool.hpp"
#include <Eigen/Dense>
#include <memory>
#include <type_traits>
#include <utility>

namespace RaptorQ__v1 {
namespace Impl {

using with_interleaver    = std::true_type;
using without_interleaver = std::false_type;


// NOTE: enabled_if methods
// instead of having 3-4 really similar methods, we use enable_if
// to enable or disable contructors and methods, so that you are forced
// to use the right ones and do not use a non-interleaved method on an
// interleaved encoder.

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
class RAPTORQ_LOCAL Raw_Encoder
{
public:
    Raw_Encoder (const Block_Size symbols, const size_t symbol_size);
    // constructor for interleaver mode
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<I::value, int>::type = 0>
    Raw_Encoder (RFC6330__v1::Impl::Interleaver<Rnd_It> *interleaver,
                                                            const uint8_t sbn);
    ~Raw_Encoder();
    Raw_Encoder() = delete;
    Raw_Encoder (const Raw_Encoder&) = delete;
    Raw_Encoder& operator= (const Raw_Encoder&) = delete;
    Raw_Encoder (Raw_Encoder&&) = delete;
    Raw_Encoder& operator= (Raw_Encoder&&) = delete;

    // set data early so we can encode source symbols without
    // having done any precomputation.
    // only for NON-INTERLEAVED
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<!I::value, int>::type = 0>
    void set_data (Rnd_It *from, Rnd_It *to);

    // "Enc" will have two implementations, depending os whether the
    // interleaver was used or not.
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<I::value, int>::type = 0>
    size_t Enc (const uint32_t ESI, Fwd_It &output, const Fwd_It end) const;
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<!I::value, int>::type = 0>
    size_t Enc (const uint32_t ESI, Fwd_It &output, const Fwd_It end) const;


    // for both interleaved and non-interleaved
    DenseMtx get_precomputed (RaptorQ__v1::Work_State *thread_keep_working);

    // interleaver-only, precomputed
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<I::value, int>::type = 0>
    bool generate_symbols (const DenseMtx &precomputed);
    // interleaver-only, non precomputed
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<I::value, int>::type = 0>
    bool generate_symbols (RaptorQ__v1::Work_State *thread_keep_working);


    // NOTE: these two automatically add padding if needed.
    // non-interleaved: requires source symbols. precomputed
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<!I::value, int>::type = 0>
    bool generate_symbols (const DenseMtx &precomputed,
                                        const Rnd_It *from, const Rnd_It *to);
    // non-interleaved: requires source symbols. non-precomputed
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<!I::value, int>::type = 0>
    bool generate_symbols (RaptorQ__v1::Work_State *thread_keep_working,
                                        const Rnd_It *from, const Rnd_It *to);


    void stop();
    bool is_stopped() const;
    void clear_data();
    bool ready() const;

private:
    const size_t _symbol_size;
    const uint16_t _symbols;
    uint8_t _SBN;
    bool  keep_working;
    const Save_Computation _type;
    std::unique_ptr<Precode_Matrix<Save_Computation::ON>> precode_on;
    std::unique_ptr<Precode_Matrix<Save_Computation::OFF>> precode_off;
    RFC6330__v1::Impl::Interleaver<Rnd_It> *_interleaver;
    Rnd_It *_from, *_to;

    DenseMtx encoded_symbols;

    // interleaved and non-interleaved functions. same signature, though.
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<!I::value, int>::type = 0>
    DenseMtx get_raw_symbols (const uint16_t K_S_H, const uint16_t S_H) const;
    template <typename R_It = Rnd_It,
        typename F_It = Fwd_It, typename I = Interleaved,
        typename std::enable_if<I::value, int>::type = 0>
    DenseMtx get_raw_symbols (const uint16_t K_S_H, const uint16_t S_H) const;


    std::pair<uint16_t, uint16_t> init_ksh() const;
    bool compute_intermediate (DenseMtx &D,
                                RaptorQ__v1::Work_State *thread_keep_working);

    size_t Enc_repair (const uint32_t ESI, Fwd_It &output,
                                                        const Fwd_It end) const;
    std::pair<uint16_t, uint16_t> init_ksh();
    static Save_Computation test_computation()
    {
        if (DLF<std::vector<uint8_t>, Cache_Key>::get()->get_size() != 0)
            return Save_Computation::ON;
        return Save_Computation::OFF;
    }
};

//
//
// Implementation
//
//

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::Raw_Encoder (const Block_Size symbols,
                                                       const size_t symbol_size)
    : _symbol_size (symbol_size), _symbols (static_cast<uint16_t> (symbols)),
    _type (test_computation()), precode_on  (nullptr), precode_off (nullptr),
                        _interleaver (nullptr), _from (nullptr), _to (nullptr)
{
    IS_RANDOM(Rnd_It, "RaptorQ__v1::Impl::Encoder");
    IS_FORWARD(Fwd_It, "RaptorQ__v1::Impl::Encoder");
    keep_working = true;
}

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<I::value, int>::type>
Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::Raw_Encoder (
        RFC6330__v1::Impl::Interleaver<Rnd_It> *interleaver, const uint8_t sbn)
    : _symbol_size (interleaver->symbol_size()),
      _symbols (static_cast<uint16_t> (interleaver->extended_symbols (sbn))),
      _SBN (sbn), _type (test_computation()),
      precode_on  (nullptr), precode_off (nullptr),
     _interleaver (interleaver), _from (nullptr), _to (nullptr)
{
    IS_RANDOM(Rnd_It, "RaptorQ__v1::Impl::Encoder");
    IS_FORWARD(Fwd_It, "RaptorQ__v1::Impl::Encoder");
    keep_working = true;
}

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::~Raw_Encoder()
    { stop(); }

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
void Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::stop()
    { keep_working = false; }

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
bool Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::is_stopped() const
    { return !keep_working; }

// NON-interleaved only
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<!I::value, int>::type>
void Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::set_data (Rnd_It *from,
                                                            Rnd_It *to)
{
    _from = from;
    _to = to;
}

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
void Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::clear_data()
{
    encoded_symbols = DenseMtx();
    _interleaver = nullptr;
    _from = nullptr;
    _to = nullptr;
}

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
bool Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::ready() const
    { return encoded_symbols.cols() != 0; }

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
DenseMtx Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::get_precomputed (
                                RaptorQ__v1::Work_State *thread_keep_working)
{
    keep_working = true;

    if (precode_on == nullptr) {
        precode_on = std::unique_ptr<Precode_Matrix<Save_Computation::ON>> (
                new Precode_Matrix<Save_Computation::ON>(Parameters(_symbols)));
        precode_on->gen(0);
    }
    if (_type == Save_Computation::ON) {
        const uint16_t size = precode_on->_params.L;
        const auto tmp_bool = std::vector<bool>();
        const Cache_Key key (size, 0, 0, tmp_bool, tmp_bool);
        auto compressed = DLF<std::vector<uint8_t>, Cache_Key>::
                                                            get()->get (key);
        if (compressed.second.size() != 0) {
            auto uncompressed = decompress (compressed.first,compressed.second);
            DenseMtx precomputed = raw_to_Mtx (uncompressed, key.out_size());
            if (precomputed.rows() != 0) {
                return precomputed;
            }
            return DenseMtx();
        }
        // else not found, generate one.
    }

    uint16_t S_H;
    uint16_t K_S_H;
    S_H = precode_on->_params.S + precode_on->_params.H;
    K_S_H = precode_on->_params.K_padded + S_H;

    // we only want the precomputex matrix.
    // we can generate that without input data.
    // each symbol now has size '1' and val '0'
    DenseMtx D;
    D.setZero (K_S_H, 1);

    Precode_Result precode_res;
    std::deque<Operation> ops;
    DenseMtx encoded_no_symbols;
    std::tie (precode_res, encoded_no_symbols) = precode_on->intermediate (D,
                                                        ops, keep_working,
                                                        thread_keep_working);
    if (precode_res != Precode_Result::DONE || encoded_no_symbols.cols() == 0)
        return DenseMtx();

    // RaptorQ succeded.
    // build the precomputed matrix.
    DenseMtx res;
    // don't save really small matrices.

    const uint16_t size = precode_on->_params.L;
    const auto tmp_bool = std::vector<bool>();
    const Cache_Key key (size, 0, 0, tmp_bool, tmp_bool);
    res.setIdentity (size, size);
    for (const auto &op : ops)
        op.build_mtx (res);
    if (_type == Save_Computation::ON) {
        auto raw_mtx = Mtx_to_raw (res);
        auto compressed = compress (raw_mtx);
        DLF<std::vector<uint8_t>, Cache_Key>::get()->add (compressed.first,
                                                        compressed.second, key);
    }
    return res;
}



// GET-RAW - non interleaved
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<!I::value, int>::type>
DenseMtx Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::get_raw_symbols(
                                 const uint16_t K_S_H, const uint16_t S_H) const
{
    using T = typename std::iterator_traits<Rnd_It>::value_type;
    assert (_to != nullptr && _from != nullptr && "RQ: get raw what?");

    DenseMtx D = DenseMtx (K_S_H, _symbol_size);

    // fill matrix D: full zero for the first S + H symbols
    D.block (0, 0, S_H, D.cols()).setZero();
    uint16_t row = S_H;

    // now the C[0...K] symbols follow
    std::vector<uint8_t> padding (sizeof(T), 0);
    Rnd_It it = *_from;
    uint8_t *p = reinterpret_cast<uint8_t*> (&*it);
    if (it >= *_to)
        p = padding.data();
    bool pad = false;
    size_t in_align = 0;
    for (; row < S_H + _symbols; ++row) {
        for (ssize_t col = 0; col < static_cast<ssize_t>(_symbol_size); ++col) {
            auto val = *(p++);
            D (row, col) = val;
            ++in_align;
            if (in_align == sizeof(T)) {
                in_align = 0;
                if (!pad) // windows debug: can't  ++iterator past the end()
                    ++it;
                if (it < *_to) {
                    p = reinterpret_cast<uint8_t*> (&*it);
                } else {
                    pad = true;
                    p = padding.data();
                }
            }
        }
    }
    // finally fill with eventual padding symbols (K...K_padded)
    D.block (row, 0, D.rows() - row, D.cols()).setZero();
    return D;
}

// GET-RAW - interleaved
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<I::value, int>::type>
DenseMtx Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::get_raw_symbols(
                                                    const uint16_t K_S_H,
                                                    const uint16_t S_H) const
{
    using T = typename std::iterator_traits<Rnd_It>::value_type;
    assert (_interleaver != nullptr);

    DenseMtx D = DenseMtx (K_S_H, sizeof(T) * _interleaver->symbol_size());
    auto C = (*_interleaver)[_SBN];

    // fill matrix D: full zero for the first S + H symbols
    D.block (0, 0, S_H, D.cols()).setZero();
    uint16_t row = S_H;
    // now the C[0...K] symbols follow
    for (; row < S_H + _interleaver->source_symbols (_SBN); ++row) {
        auto symbol = C[row - S_H];
        uint16_t col = 0;
        for (uint16_t i = 0; i < _interleaver->symbol_size(); ++i) {
            T val = symbol[i];
            uint8_t *octet = reinterpret_cast<uint8_t *> (&val);
            for (uint8_t byte = 0; byte < sizeof(T); ++byte)
                D (row, col++) = *(octet++);
        }
    }

    // finally fill with eventual padding symbols (K...K_padded)
    D.block (row, 0, D.rows() - row, D.cols()).setZero();
    return D;
}



// GENERATE - interleaved, precomputed
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<I::value, int>::type>
bool Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::generate_symbols (
                                                    const DenseMtx &precomputed)
{
    if (precomputed.rows() == 0)
        return false;
    keep_working = true;

    const uint16_t S_H = precode_on->_params.S + precode_on->_params.H;
    const uint16_t K_S_H = precode_on->_params.K_padded + S_H;
    const DenseMtx D = get_raw_symbols (K_S_H, S_H);
    encoded_symbols = precomputed * D;
    return true;
}

// GENERATE - interleaved, NON precomputed
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<I::value, int>::type>
bool Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::generate_symbols (
                                RaptorQ__v1::Work_State *thread_keep_working)
{
    if (encoded_symbols.cols() != 0)
        return true;
    keep_working = true;

    auto ksh = init_ksh();
    DenseMtx D = get_raw_symbols (ksh.first, ksh.second);
    return compute_intermediate (D, thread_keep_working);
}

// GENERATE - NON interleaved, precomputed
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<!I::value, int>::type>
bool Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::generate_symbols (
                                        const DenseMtx &precomputed,
                                        const Rnd_It *from, const Rnd_It *to)
{
    if (precomputed.rows() == 0 || from == nullptr || to == nullptr)
        return false;
    keep_working = true;

    _from = const_cast<Rnd_It*> (from);
    _to = const_cast<Rnd_It*> (to);
    const uint16_t S_H = precode_on->_params.S + precode_on->_params.H;
    const uint16_t K_S_H = precode_on->_params.K_padded + S_H;

    const DenseMtx D = get_raw_symbols (K_S_H, S_H);
    encoded_symbols = precomputed * D;
    return true;
}

// GENERATE - NON interleaved, NON precomputed
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<!I::value, int>::type>
bool Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::generate_symbols (
                                RaptorQ__v1::Work_State *thread_keep_working,
                                const Rnd_It *from, const Rnd_It *to)
{
    // do not bother checking for multithread. that is done in the caller
    if (from == nullptr || to == nullptr)
        return false;
    if (encoded_symbols.cols() != 0)
        return true;
    keep_working = true;

    _from = const_cast<Rnd_It*> (from);
    _to = const_cast<Rnd_It*> (to);
    keep_working = true;

    auto ksh = init_ksh();
    DenseMtx D = get_raw_symbols (ksh.first, ksh.second);
    return compute_intermediate (D, thread_keep_working);
}




template <typename Rnd_It, typename Fwd_It, typename Interleaved>
std::pair<uint16_t, uint16_t> Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::
                                                                    init_ksh()
{
    uint16_t S_H;
    uint16_t K_S_H;
    if (_type == Save_Computation::ON) {
        if (precode_on == nullptr) {
            precode_on = std::unique_ptr<Precode_Matrix<Save_Computation::ON>> (
                                    new Precode_Matrix<Save_Computation::ON> (
                                                        Parameters(_symbols)));
            precode_on->gen(0);
        }
        S_H = precode_on->_params.S + precode_on->_params.H;
        K_S_H = precode_on->_params.K_padded + S_H;
    } else {
        if (precode_off == nullptr) {
            precode_off =std::unique_ptr<Precode_Matrix<Save_Computation::OFF>>(
                                    new Precode_Matrix<Save_Computation::OFF> (
                                                        Parameters(_symbols)));
            precode_off->gen(0);
        }
        S_H = precode_off->_params.S + precode_off->_params.H;
        K_S_H = precode_off->_params.K_padded + S_H;
    }
    return {K_S_H, S_H};
}

template <typename Rnd_It, typename Fwd_It, typename Interleaved>
bool Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::compute_intermediate (
                    DenseMtx &D, RaptorQ__v1::Work_State *thread_keep_working)
{
    Precode_Result precode_res;
    std::deque<Operation> ops;
    if (_type == Save_Computation::ON) {
        const uint16_t size = precode_on->_params.L;
        const auto tmp_bool = std::vector<bool>();
        const Cache_Key key (size, 0, 0, tmp_bool, tmp_bool);
        auto compressed = DLF<std::vector<uint8_t>, Cache_Key>::
                                                            get()->get (key);
        if (compressed.second.size() != 0) {
            auto decompressed = decompress (compressed.first,compressed.second);
            DenseMtx precomputed = raw_to_Mtx (decompressed, key.out_size());
            if (precomputed.rows() != 0) {
                // we have a precomputed matrix! let's use that!
                encoded_symbols = precomputed * D;
                // result is granted. we only save matrices that work
                return true;
            }
        }
        std::tie (precode_res, encoded_symbols) = precode_on->intermediate (D,
                                                        ops, keep_working,
                                                        thread_keep_working);

        if (precode_res != Precode_Result::DONE || encoded_symbols.cols() == 0)
            return false;

        // RaptorQ succeded.
        // build the precomputed matrix.
        DenseMtx res;
        if (encoded_symbols.cols() != 0) {
            res.setIdentity (size, size);
            for (const auto &op : ops)
                op.build_mtx (res);
            auto raw_mtx = Mtx_to_raw (res);
            compressed = compress (raw_mtx);
            DLF<std::vector<uint8_t>, Cache_Key>::get()->add (compressed.first,
                                                        compressed.second, key);
        }
    } else {
        std::tie (precode_res, encoded_symbols) = precode_off->intermediate (D,
                                                        ops, keep_working,
                                                        thread_keep_working);
    }
    return (Precode_Result::DONE == precode_res) && 0 != encoded_symbols.cols();
}

// interleaved encoding
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<I::value, int>::type>
size_t Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::Enc (const uint32_t ESI,
                                                        Fwd_It &output,
                                                        const Fwd_It end) const
{
    // returns iterators written
    // ESI means that the first _symbols.source_symbols() are the
    // original symbols, and the next ones are repair symbols.

    // The alignment of "Fwd_It" might *NOT* be the alignment of "Rnd_It"

    size_t written = 0;
    if (_interleaver == nullptr)
        return written;
    auto non_repair = _interleaver->source_symbols (_SBN);

    if (ESI < non_repair) {
        // just return the source symbol.
        auto block = (*_interleaver)[_SBN];
        auto requested_symbol = block[static_cast<uint16_t> (ESI)];

        typedef typename std::iterator_traits<Fwd_It>::value_type out_al;
        size_t byte = 0;
        out_al tmp_out = 0;
        for (auto al : requested_symbol) {
            uint8_t *p;
            for (p = reinterpret_cast<uint8_t *> (&al);
                            p != reinterpret_cast<uint8_t *>(&al) + sizeof(al);
                                                                        ++p) {
                tmp_out += static_cast<out_al> (*p) << (byte * 8);
                ++byte;
                if (byte % sizeof(out_al) == 0) {
                    *(output++) = tmp_out;
                    ++written;
                    byte = 0;
                    tmp_out = 0;
                    if (output == end)
                        return written;
                }
            }
        }
        if (byte % sizeof(out_al) != 0) {
            *(output++) = tmp_out;
            ++written;
        }
        return written;
    } else {
        return Enc_repair (ESI, output, end);
    }
}

// NON interleaved encoding
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
template <typename R_It, typename F_It, typename I,
                                typename std::enable_if<!I::value, int>::type>
size_t Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::Enc (const uint32_t ESI,
                                                        Fwd_It &output,
                                                        const Fwd_It end) const
{
    // returns iterators written
    // ESI means that the first _symbols.source_symbols() are the
    // original symbols, and the next ones are repair symbols.

    // The alignment of "Fwd_It" might *NOT* be the alignment of "Rnd_It"

    size_t written = 0;
    if (_from == nullptr || _to == nullptr)
        return written;

    if (ESI < _symbols) {
        // just return the source symbol.
        typedef typename std::iterator_traits<Rnd_It>::value_type in_T;
        typedef typename std::iterator_traits<Fwd_It>::value_type out_T;
        const in_T padding = static_cast<in_T> (0);
        const size_t skip_it = (ESI * _symbol_size) / sizeof(in_T);
              size_t in_al   = (ESI * _symbol_size) % sizeof(in_T);
        Rnd_It it = *_from + static_cast<int64_t> (skip_it);
        uint8_t *p_in = reinterpret_cast<uint8_t*> (&*it);
        if (it >= *_to)
            p_in = reinterpret_cast<uint8_t*> (const_cast<in_T*> (&padding));
        p_in += in_al;
        size_t out_al = 0;
        out_T tmp_out = static_cast<out_T> (0);
        uint8_t *p_out = reinterpret_cast<uint8_t*> (&tmp_out);
        size_t byte = 0;
        while (output != end && byte != _symbol_size) {
            *(p_out++) = *(p_in++);
            ++out_al;
            ++in_al;
            ++byte;
            if (in_al == sizeof(in_T)) {
                in_al = 0;
                ++it;
                if (it < *_to) {
                    p_in = reinterpret_cast<uint8_t*> (&*it);
                } else {
                    p_in = reinterpret_cast<uint8_t*> (const_cast<in_T*> (
                                                                    &padding));
                }
            }
            if (out_al == sizeof(out_T)) {
                out_al = 0;
                ++written;
                *(output++) = tmp_out;
                tmp_out = static_cast<out_T> (0);
                p_out = reinterpret_cast<uint8_t*> (&tmp_out);
            }
        }
        if (out_al != 0) {
            *(output++) = tmp_out;
            ++written;
        }
        return written;
    } else {
        return Enc_repair (ESI, output, end);
    }
}

// repair symbol only - no need to diffenretiate between (non)interleaved
template <typename Rnd_It, typename Fwd_It, typename Interleaved>
size_t Raw_Encoder<Rnd_It, Fwd_It, Interleaved>::Enc_repair (const uint32_t ESI,
                                                        Fwd_It &output,
                                                        const Fwd_It end) const
{
    size_t written = 0;
    // repair symbol requested.
    if (!ready())
        return written;
    uint16_t K;
    if (_type == Save_Computation::ON) {
        if (precode_on == nullptr)
            return 0;
        K = precode_on->_params.K_padded;
    } else {
        if (precode_off == nullptr) {
            if (precode_on == nullptr)
                return 0;
            // we might have used the precode, and thus forced the "precode_on".
            K = precode_on->_params.K_padded;
        } else {
            K = precode_off->_params.K_padded;
        }
    }
    auto ISI = ESI + (K - _symbols);
    DenseMtx tmp;
    if (_type == Save_Computation::ON) {
        tmp = precode_on->encode (encoded_symbols, ISI);
    } else {
        if (precode_off == nullptr) {
            // we were forced to use "precode_on". see earlier "if"
            tmp = precode_on->encode (encoded_symbols, ISI);
        } else {
            tmp = precode_off->encode (encoded_symbols, ISI);
        }
    }

    // put "tmp" in output, but the alignment is different

    using T = typename std::iterator_traits<Fwd_It>::value_type;
    T al = static_cast<T> (0);
    uint8_t *p = reinterpret_cast<uint8_t *>  (&al);
    for (int32_t i = 0; i < tmp.cols(); ++i) {
        *p = static_cast<uint8_t> (tmp (0, i));
        ++p;
        if (p == reinterpret_cast<uint8_t *>  (&al) + sizeof(T)) {
            *output = al;
            ++output;
            al = static_cast<T> (0);
            p = reinterpret_cast<uint8_t *>  (&al);
            ++written;
            if (output == end)
                return written;
        }
    }
    if (p != reinterpret_cast<uint8_t *>  (&al) + sizeof(T)) {
        // symbol size is not aligned with Fwd_It type
        while (p != reinterpret_cast<uint8_t *>  (&al) + sizeof(T))
            *(p++) = 0;
        *output = al;
        ++output;
        ++written;
    }
    return written;
}

}   // namespace Impl
}   // namespace RFC6330__v1
