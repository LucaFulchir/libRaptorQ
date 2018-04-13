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
#include "RaptorQ/v1/Octet.hpp"
#include "RaptorQ/v1/Parameters.hpp"
#include "RaptorQ/v1/Precode_Matrix.hpp"
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#include "RaptorQ/v1/Thread_Pool.hpp"
#include "RaptorQ/v1/util/Bitmask.hpp"
#include "RaptorQ/v1/util/Graph.hpp"
#include <memory>
#include <mutex>
#include <random>
#include <tuple>
#include <utility>
#include <vector>
#include <Eigen/Dense>


namespace RaptorQ__v1 {

namespace Impl {

template <typename In_It>
class RAPTORQ_LOCAL Raw_Decoder
{
    // decode () can be launched multiple times,
    // But each time the list of source and repair symbols might
    // change.
    using Vect = Eigen::Matrix<Impl::Octet, 1, Eigen::Dynamic, Eigen::RowMajor>;
    using T_in = typename std::iterator_traits<In_It>::value_type;
public:

    bool end_of_input;

    Raw_Decoder (const Block_Size symbols, const size_t symbol_size)
        :keep_working (true), type (test_computation()),
                    _symbols (static_cast<uint16_t> (symbols)), mask (_symbols)
    {
        IS_INPUT(In_It, "RaptorQ__v1::Impl::Decoder");
        // symbol size is in octets, but we save it in "T" sizes.
        // so be aware that "symbol_size" != "_symbol_size" for now
        source_symbols = DenseMtx (_symbols, symbol_size);
        concurrent = 0;
        can_retry = false;
        end_of_input = false;
    }
    Raw_Decoder (const Block_Size symbols, const size_t symbol_size,
                                                const uint16_t padding_symbols)
        :Raw_Decoder (symbols, symbol_size)
    {
        assert (padding_symbols <= static_cast<uint16_t> (symbols) &&
                                            "RQ RFC Decoder: too much padding");

        uint16_t to_pad = (static_cast<uint16_t> (symbols) - padding_symbols);
        source_symbols.block(to_pad, 0, padding_symbols, symbol_size).setZero();
        for (; to_pad < static_cast<uint16_t> (symbols); ++to_pad)
            mask.add (to_pad);
    }
    ~Raw_Decoder();
    Raw_Decoder() = delete;
    Raw_Decoder (const Raw_Decoder&) = delete;
    Raw_Decoder& operator= (const Raw_Decoder&) = delete;
    Raw_Decoder (Raw_Decoder&&) = delete;
    Raw_Decoder& operator= (Raw_Decoder&&) = delete;

    Error add_symbol (In_It &start, const In_It end, const uint32_t esi,
                                                                bool padded);
    Decoder_Result decode (Work_State *thread_keep_working);
    DenseMtx* get_symbols();
    bool has_symbol (const uint16_t symbol) const;

    void stop();
    void clear_data();
    bool is_stopped() const;
    // can start a computation (with different data)
    bool can_decode() const;
    // has everything been decoded?
    bool ready() const;
    // do we have enough symbols (source + repair)
    uint16_t needed_symbols() const;
    // should we add work? we have a maximum amount of concurent decoders
    bool add_concurrent (const uint16_t max_concurrent);
    uint16_t threads() const;
    void drop_concurrent();
    // you know you will not receive additional data, and can not decode.
    // fill with zeros and return what you have
    // returns the bitmask of the SYMBOLS we had (true) or not (false)
    std::vector<bool> fill_with_zeros();

private:
    bool keep_working, can_retry;
    const Save_Computation type;
    std::mutex lock;
    const uint16_t _symbols;
    uint16_t concurrent;    // currently running decoders retry
    Bitmask mask;
    DenseMtx source_symbols;
    std::vector<std::pair<uint32_t, Vect>> received_repair;

    // to help making things const
    static Save_Computation test_computation()
    {
        if (DLF<std::vector<uint8_t>, Cache_Key>::get()->get_size() != 0){
            return Save_Computation::ON;
        }
        return Save_Computation::OFF;
    }
    // to help making things const
    Precode_Matrix<Save_Computation::ON> *init_precode_on (
                                                const uint16_t symbols) const
    {
        if (type == Save_Computation::ON) {
            return new Precode_Matrix<Save_Computation::ON> (
                                                        Parameters(symbols));
        }
        return nullptr;
    }
    // to help making things const
    Precode_Matrix<Save_Computation::OFF> *init_precode_off(
                                                const uint16_t symbols) const
    {
        if (type == Save_Computation::OFF) {
            return new Precode_Matrix<Save_Computation::OFF> (
                                                        Parameters(symbols));
        }
        return nullptr;
    }
};


///////////////////////////////////
//
// IMPLEMENTATION OF ABOVE TEMPLATE
//
///////////////////////////////////

template <typename In_It>
Raw_Decoder<In_It>::~Raw_Decoder()
    { stop(); }

template <typename In_It>
void Raw_Decoder<In_It>::stop()
    { keep_working = false; }

template <typename In_It>
void Raw_Decoder<In_It>::clear_data()
{
    std::unique_lock<std::mutex> lock_all (lock);
    RQ_UNUSED (lock_all);
    stop();
    // not needed, we will need to reallocate it anyway,
    // and the bitmask already considers the symbols as unkowns
    //source_symbols = DenseMtx (_symbols, symbol_size);
    concurrent = 0;
    can_retry = false;
    end_of_input = false;
    mask = Bitmask (_symbols);
    received_repair.clear();
}

template <typename In_It>
bool Raw_Decoder<In_It>::is_stopped() const
    { return !keep_working; }

template <typename In_It>
bool Raw_Decoder<In_It>::ready() const
    { return mask.get_holes() == 0; }

template <typename In_It>
bool Raw_Decoder<In_It>::can_decode() const
    { return can_retry; }

template <typename In_It>
uint16_t Raw_Decoder<In_It>::needed_symbols() const
{
    if (can_retry)
        return 0;
    int32_t needed = static_cast<int32_t> (mask.get_holes()) -
                        static_cast<int32_t> (received_repair.size());
    if (needed < 0)
        needed = 0;
    if (needed == 0 && !can_retry && concurrent == 0)
        return 1;   // tried, but failed. need one more repair symbol
    return static_cast<uint16_t> (needed);
}

template <typename In_It>
bool Raw_Decoder<In_It>::add_concurrent (const uint16_t max_concurrent)
{
    std::unique_lock<std::mutex> guard (lock);
    RQ_UNUSED(guard);
    if (max_concurrent > concurrent) {
        ++concurrent;
        return true;
    }
    return false;
}

template <typename In_It>
uint16_t Raw_Decoder<In_It>::threads() const
    { return concurrent; }

template <typename In_It>
void Raw_Decoder<In_It>::drop_concurrent()
{
    std::unique_lock<std::mutex> guard (lock);
    RQ_UNUSED(guard);
    // "if" should not be necessary. But I forgot to add --make-bug-free flag.
    if (concurrent > 0)
        --concurrent;
}

template <typename In_It>
DenseMtx* Raw_Decoder<In_It>::get_symbols()
    { return &source_symbols; }

template <typename In_It>
bool Raw_Decoder<In_It>::has_symbol (const uint16_t symbol) const
    { return mask.get_holes() == 0 || mask.exists (symbol); }

template <typename In_It>
Error Raw_Decoder<In_It>::add_symbol (In_It &start, const In_It end,
                                            const uint32_t esi, bool padded)
{
    // true if added succesfully

    // only the last symbol from the RFC interface should be padded,
    // but we do not track here which one that is, so we leave that to
    // the rfc interface

    // if we were lucky to get a random access iterator, quickly check that
    // the we have enough data for the symbol.
    if (std::is_same<typename std::iterator_traits<In_It>::iterator_category,
                                    std::random_access_iterator_tag>::value) {
        if (static_cast<size_t>(end - start) * sizeof(T_in) <
                                static_cast<size_t> (source_symbols.cols()))
            return Error::WRONG_INPUT;
    }

    // TODO: move this on the interface, so the RAW API can use
    // the full 32 bits
    if (esi >= std::pow (2, 20))
        return Error::WRONG_INPUT;

    std::lock_guard<std::mutex> guard (lock);
    RQ_UNUSED(guard);

    if (mask.get_holes() == 0 || mask.exists (esi))
        return Error::NOT_NEEDED;   // not even needed.

    uint16_t col = 0;
    if (esi < _symbols) {
        for (; start != end && col != source_symbols.cols(); ++start) {
            T_in al = *start;
            for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
                        p != reinterpret_cast<uint8_t *> (&al) + sizeof(T_in)
                                        && col != source_symbols.cols(); ++p) {
                source_symbols (static_cast<int32_t>(esi), col++) = *p;
            }
        }
        // input iterator might reach end before we get enough data
        // for the symbol.
        if (col != source_symbols.cols()) {
            if (padded) {
                while (col != source_symbols.cols())
                    source_symbols (static_cast<int32_t>(esi), col++) = 0;
            } else {
                return Error::WRONG_INPUT;
            }
        }
    } else {
        Vect v = Vect (source_symbols.cols());
        for (; start != end && col != source_symbols.cols(); ++start) {
            T_in al = *start;
            for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
                        p != reinterpret_cast<uint8_t *> (&al) + sizeof(T_in)
                                        && col != source_symbols.cols(); ++p) {
                v (col++) = *p;
            }
        }
        // input iterator might reach end before we get enough data
        // for the symbol.
        if (col != v.cols())
            return Error::WRONG_INPUT;
        received_repair.emplace_back (esi, std::move(v));
        // reorder the received_repair:
        // ordering the repair packets lets us have more deterministic
        // matrices, that we can use for precomputation.
        // this *should* not be a big performance hit as the repair symbols
        // should already be *almost* in order.
        // TODO: b-tree? map?
        int32_t idx = static_cast<int32_t> (received_repair.size()) - 1;
        while (idx >= 1) {
            const uint32_t idx_u = static_cast<uint32_t> (idx);
            const uint32_t prev = idx_u - 1;
            if (received_repair[idx_u].first < received_repair[prev].first) {
                std::swap (received_repair[idx_u], received_repair[prev]);
                --idx;
            } else {
                break;
            }
        }
    }
    mask.add (esi);

    if (mask.get_holes() <= received_repair.size())
        can_retry = true;
    return Error::NONE;
}

template <typename In_It>
std::vector<bool> Raw_Decoder<In_It>::fill_with_zeros()
{
    std::lock_guard<std::mutex> dec_lock (lock);
    RQ_UNUSED(dec_lock);
    stop();
    // free mem;
    received_repair = std::vector<std::pair<uint32_t, Vect>>();

    std::vector<bool> ret (_symbols, false);

    for (uint16_t idx = 0; idx < _symbols; ++idx) {
        if (mask.exists (idx))
            continue;
        ret[idx] = true;
        source_symbols.block(idx, 0, 1, source_symbols.cols()).setZero();
        mask.add (idx);
    }
    end_of_input = true;
    return ret;
}

template <typename In_It>
Decoder_Result Raw_Decoder<In_It>::decode (Work_State *thread_keep_working)
{
    // this method can be launched concurrently multiple times.
    // TODO:  do not build matrices with more than 4 overhead elements,
    // but don't lose received elements either

    // rfc 6330: can decode when received >= K_padded
    // actually: (K_padded - K) are padding and thus constant and NOT
    // transmitted. so really N >= K.
    if (mask.get_holes() == 0)
        return Decoder_Result::DECODED;

    if (received_repair.size() < mask.get_holes())
        return Decoder_Result::NEED_DATA;

    const std::unique_ptr<Precode_Matrix< Save_Computation::ON>> precode_on (
                                                init_precode_on (_symbols));
    const std::unique_ptr<Precode_Matrix<Save_Computation::OFF>> precode_off (
                                                init_precode_off (_symbols));
    std::unique_lock<std::mutex> shared (lock);
    if (!can_retry)
        return Decoder_Result::NEED_DATA;
    can_retry = false;
    const uint32_t overhead = static_cast<uint32_t> (
                                    received_repair.size() - mask.get_holes());

    if (type == Save_Computation::ON) {
        precode_on->gen (static_cast<uint32_t> (overhead));
    } else {
        precode_off->gen (static_cast<uint32_t> (overhead));
    }

    uint16_t S_H;
    uint16_t L_rows;
    std::vector<bool> bitmask_repair;
    if (type == Save_Computation::ON) {
        L_rows = precode_on->_params.L;
        S_H = precode_on->_params.S + precode_on->_params.H;
        // repair.rbegin() is the highest repair symbol
        const auto it = received_repair.crbegin();
        bitmask_repair.reserve ((it->first - _symbols));
        uint32_t idx = _symbols;
        for (auto rep = received_repair.begin();
                                rep != received_repair.end(); ++rep, ++idx) {
            for (;idx < rep->first; ++idx)
                bitmask_repair.push_back (false);
            bitmask_repair.push_back (true);
        }
    } else {
        L_rows = precode_off->_params.L;
        S_H = precode_off->_params.S + precode_off->_params.H;
    }
    const Cache_Key key (L_rows, mask.get_holes(),
                                static_cast<uint32_t> (received_repair.size()),
                                            mask.get_bitmask(), bitmask_repair);

    DenseMtx D = DenseMtx (L_rows + overhead, source_symbols.cols());

    // initialize D: first S_H rows == 0
    D.block(0, 0, S_H, D.cols()).setZero();
    // put non-repair symbols (source symbols) in place
    if (mask.get_holes() == 0) {
        // other thread completed its work before us?
        return Decoder_Result::DECODED;
    }
    D.block (S_H, 0, source_symbols.rows(), D.cols()) = source_symbols;

    // mask must be copied to avoid threading problems, same with tracking
    // the repair esi.
    const Bitmask mask_safe = mask;
    std::vector<uint32_t> repair_esi;
    repair_esi.reserve (received_repair.size());
    for (auto rep : received_repair)
        repair_esi.push_back (rep.first);

    // fill holes with the first repair symbols available
    auto symbol = received_repair.begin();
    uint16_t hole = 0;
    while (hole < _symbols && symbol != received_repair.end()) {
        if (mask_safe.exists (static_cast<size_t> (hole))) {
            ++hole;
            continue;
        }
        const uint16_t row = S_H + hole;
        D.row (row) = symbol->second;
        ++symbol;
        ++hole;
    }
    // fill the padding symbols (always zero)
    D.block (S_H + _symbols, 0, (L_rows - S_H) - _symbols, D.cols()).setZero();
    // fill the remaining (redundant) repair symbols
    for (uint16_t row = L_rows; symbol != received_repair.end(); ++symbol) {
        D.row (row) = symbol->second;
        ++row;
    }

    // do not lock this part, as it's the expensive part
    shared.unlock();
    bool DO_NOT_SAVE = false;
    std::deque<Operation> ops;

    Precode_Result precode_res = Precode_Result::DONE;
    DenseMtx missing;
    if (type == Save_Computation::ON) {
        auto compressed = DLF<std::vector<uint8_t>, Cache_Key>::
                                                            get()->get (key);
        auto decompressed = decompress (compressed.first, compressed.second);
        DenseMtx precomputed = raw_to_Mtx (decompressed, key.out_size());
        if (precomputed.rows() != 0) {
            DO_NOT_SAVE = true;
            missing = precomputed * D;
            missing = precode_on->get_missing (std::move(missing), mask_safe);
        } else {
            std::tie (precode_res, missing) = precode_on->intermediate (D,
                                            mask_safe, repair_esi, ops,
                                            keep_working, thread_keep_working);
            missing = precode_on->get_missing (std::move(missing), mask_safe);
        }

    } else {
        std::tie (precode_res, missing) = precode_off->intermediate (D,
                                            mask_safe, repair_esi, ops,
                                            keep_working, thread_keep_working);
        missing = precode_off->get_missing (std::move(missing), mask_safe);
    }
    if (precode_res == Precode_Result::STOPPED) {
        if (mask.get_holes() == 0)
            return Decoder_Result::DECODED;
        return Decoder_Result::STOPPED;
    }

    D = DenseMtx(); // free some memory;
    if (type == Save_Computation::ON && !DO_NOT_SAVE &&
                                        precode_res == Precode_Result::DONE) {
        DenseMtx res;
        if (missing.rows() != 0) {
            const int32_t mt_size = static_cast<int32_t>(L_rows + overhead);
            res.setIdentity (mt_size, mt_size);
            for (const auto &op : ops)
                op.build_mtx (res);
            // TODO: lots of wasted ram? how to compress things directly?
            auto raw_mtx = Mtx_to_raw (res);
            auto compressed = compress (raw_mtx);
            DLF<std::vector<uint8_t>, Cache_Key>::get()->add (compressed.first,
                                                        compressed.second, key);
        }
    }

    std::lock_guard<std::mutex> dec_lock (lock);
    RQ_UNUSED(dec_lock);

    if (precode_res == Precode_Result::FAILED) {
        if (can_retry)
            return Decoder_Result::CAN_RETRY;
        return Decoder_Result::NEED_DATA;
    }
    if (mask.get_holes() == 0)
        return Decoder_Result::DECODED;

    // put missing symbols into "source_symbols".
    // remember: we might have received other symbols while decoding.
    uint16_t miss_row = 0;
    for (uint16_t row = 0; row < mask_safe._max_nonrepair &&
                                            miss_row < missing.rows(); ++row) {
        if (mask_safe.exists (row))
            continue;
        ++miss_row;
        if (mask.exists (row))
            continue;
        source_symbols.row (row) = missing.row (miss_row - 1);
        mask.add (row);
    }

    keep_working = false;   // tell eventual threads to stop crunching,
    // free some memory, we don't need recover symbols anymore
    received_repair = std::vector<std::pair<uint32_t, Vect>>();
    mask.free();


    return Decoder_Result::DECODED;
}

}   // namespace Impl
}   // namespace RFC6330__v1
