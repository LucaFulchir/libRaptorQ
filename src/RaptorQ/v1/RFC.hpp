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

/////////////////////
//
//  These templates are just a wrapper around the
//  functionalities offered by the RaptorQ__v1::Impl namespace
//  So if you want to see what the algorithm looks like,
//  you are in the wrong place
//
/////////////////////

#include "RaptorQ/v1/block_sizes.hpp"
#include "RaptorQ/v1/Interleaver.hpp"
#include "RaptorQ/v1/De_Interleaver.hpp"
#include "RaptorQ/v1/Decoder.hpp"
#include "RaptorQ/v1/Encoder.hpp"
#include "RaptorQ/v1/RFC_Iterators.hpp"
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#include "RaptorQ/v1/Thread_Pool.hpp"
#include <algorithm>
#include <cassert>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <limits>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

namespace RFC6330__v1 {

constexpr uint64_t max_data = RFC6330_max_data;  // ~881 GB

namespace Impl {
template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_LOCAL Encoder;
template <typename In_It, typename Fwd_It>
class RAPTORQ_LOCAL Decoder;
} // namespace Impl
#ifdef RQ_HEADER_ONLY
    template <typename Rnd_It, typename Fwd_It>
    using Encoder = Impl::Encoder<Rnd_It, Fwd_It>;
    template <typename Rnd_It, typename Fwd_It>
    using Decoder = Impl::Decoder<Rnd_It, Fwd_It>;
#endif

namespace Impl {


template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_LOCAL Encoder
{
public:

    Encoder() = delete;
    Encoder (const Encoder&) = delete;
    Encoder& operator= (const Encoder&) = delete;
    Encoder (Encoder&&) = default;
    Encoder& operator= (Encoder&&) = default;
    ~Encoder();
    Encoder (const Rnd_It data_from, const Rnd_It data_to,
                                            const uint16_t min_subsymbol_size,
                                            const uint16_t symbol_size,
                                            const size_t max_sub_block)
        : _max_sub_blk (max_sub_block), _data_from (data_from),
                                            _data_to (data_to),
                                            _symbol_size (symbol_size),
                                            _min_subsymbol (min_subsymbol_size),
                                            interleave (_data_from,
                                                        _data_to,
                                                        _min_subsymbol,
                                                        _max_sub_blk,
                                                        _symbol_size)
    {
        IS_RANDOM(Rnd_It, "RFC6330__v1::Encoder");
        IS_FORWARD(Fwd_It, "RFC6330__v1::Encoder");
        auto _alignment = sizeof(typename
                                    std::iterator_traits<Rnd_It>::value_type);
        RQ_UNUSED(_alignment);  // used only for asserts
        assert(_symbol_size >= _alignment &&
                        "RaptorQ: symbol_size must be >= alignment");
        assert((_symbol_size % _alignment) == 0 &&
                        "RaptorQ: symbol_size must be multiple of alignment");
        assert(min_subsymbol_size >= _alignment &&
                        "RaptorQ: minimum subsymbol must be at least aligment");
        assert(min_subsymbol_size <= _symbol_size &&
                    "RaptorQ: minimum subsymbol must be at most symbol_size");
        assert((min_subsymbol_size % _alignment) == 0 &&
                    "RaptorQ: minimum subsymbol must be multiple of alignment");
        assert((_symbol_size % min_subsymbol_size == 0) &&
                    "RaptorQ: symbol size must be multiple of subsymbol size");
        // max size: ~881 GB
        if (static_cast<uint64_t> (data_to - data_from) *
                    sizeof(typename std::iterator_traits<Rnd_It>::value_type)
                                                                > max_data) {
            return;
        }

        _pool_notify = std::make_shared<std::condition_variable>();
        _pool_mtx = std::make_shared<std::mutex>();
        pool_last_reported = -1;
        use_pool = true;
        exiting = false;
    }

    It::Encoder::Block_Iterator<Rnd_It, Fwd_It> begin ()
        { return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (this, 0); }
    const It::Encoder::Block_Iterator<Rnd_It, Fwd_It> end ()
        { return It::Encoder::Block_Iterator<Rnd_It, Fwd_It> (this, blocks()); }

    operator bool() const { return interleave; }
    RFC6330_OTI_Common_Data OTI_Common() const;
    RFC6330_OTI_Scheme_Specific_Data OTI_Scheme_Specific() const;

    std::future<std::pair<Error, uint8_t>> compute (const Compute flags);

    size_t precompute_max_memory ();
    size_t encode (Fwd_It &output, const Fwd_It end, const uint32_t esi,
                                                            const uint8_t sbn);
    // id: 8-bit sbn + 24 bit esi
    size_t encode (Fwd_It &output, const Fwd_It end, const uint32_t &id);
    void free (const uint8_t sbn);
    uint8_t blocks() const;
    uint32_t block_size (const uint8_t sbn) const;
    uint16_t symbol_size() const;
    uint16_t symbols (const uint8_t sbn) const;
    uint32_t max_repair (const uint8_t sbn) const;
private:

    static void wait_threads (Encoder<Rnd_It, Fwd_It> *obj, const Compute flags,
                                    std::promise<std::pair<Error, uint8_t>> p);

    class Block_Work final : public Impl::Pool_Work {
    public:
        std::weak_ptr<RaptorQ__v1::Impl::Raw_Encoder<Rnd_It, Fwd_It,
                                  RaptorQ__v1::Impl::with_interleaver>> work;
        std::weak_ptr<std::condition_variable> notify;
        std::weak_ptr<std::mutex> lock;

        Work_Exit_Status do_work (RaptorQ__v1::Work_State *state) override;
        ~Block_Work() override;
    };

    // TODO: tagged pointer
    class Enc {
    public:
        Enc (Impl::Interleaver<Rnd_It> *interleaver, const uint8_t sbn)
        {
            enc = std::make_shared<RaptorQ__v1::Impl::Raw_Encoder<Rnd_It,
                                Fwd_It, RaptorQ__v1::Impl::with_interleaver>> (
                                                            interleaver, sbn);
            reported = false;
        }
        std::shared_ptr<RaptorQ__v1::Impl::Raw_Encoder<Rnd_It, Fwd_It,
                                    RaptorQ__v1::Impl::with_interleaver>> enc;
        bool reported;
    };

    std::pair<Error, uint8_t> get_report (const Compute flags);
    std::shared_ptr<std::condition_variable> _pool_notify;
    std::shared_ptr<std::mutex> _pool_mtx;
    std::deque<std::thread> pool_wait;

    std::map<uint8_t, Enc> encoders;
    std::mutex _mtx;

    const size_t _max_sub_blk;
    const Rnd_It _data_from, _data_to;
    const uint16_t _symbol_size;
    const uint16_t _min_subsymbol;
    Impl::Interleaver<Rnd_It> interleave;
    bool use_pool, exiting;
    int16_t pool_last_reported;

};

template <typename In_It, typename Fwd_It>
class RAPTORQ_LOCAL Decoder
{
public:

    // rfc 6330, pg 6
    // easy explanation for OTI_* comes next.
    // we do NOT use bitfields as compilators are not actually forced to put
    // them in any particular order. meaning tey're useless.
    //
    //union OTI_Common_Data {
    //  uint64_t raw;
    //  struct {
    //      uint64_t size:40;
    //      uint8_t reserved:8;
    //      uint16_t symbol_size:16;
    //  };
    //};

    //union OTI_Scheme_Specific_Data {
    //  uint32_t raw;
    //  struct {
    //      uint8_t source_blocks;
    //      uint16_t sub_blocks;
    //      uint8_t alignment;
    //  };
    //};
    Decoder (const Decoder&) = delete;
    Decoder& operator= (const Decoder&) = delete;
    Decoder (Decoder&&) = default;
    Decoder& operator= (Decoder&&) = default;
    ~Decoder();
    Decoder (const RFC6330_OTI_Common_Data common,
                            const RFC6330_OTI_Scheme_Specific_Data scheme)
    {
        // _size > max_data means improper initialization.
        IS_INPUT(In_It, "RaptorQ__v1::Decoder");
        IS_FORWARD(Fwd_It, "RaptorQ__v1::Decoder");

        // see the above commented bitfields for quick reference
        _symbol_size = static_cast<uint16_t> (common);
        _size = common >> 24;
        uint16_t tot_sub_blocks = static_cast<uint16_t> (scheme >> 8);
        _alignment = static_cast<uint8_t> (scheme);
        _blocks = static_cast<uint8_t> (scheme >> 24);
        if (_size > max_data || _size % _alignment != 0 ||
                                            _symbol_size % _alignment != 0) {
            _size = std::numeric_limits<uint64_t>::max();
            return;
        }
        _sub_blocks = Impl::Partition (_symbol_size /
                                                static_cast<uint8_t> (scheme),
                                                                tot_sub_blocks);

        const uint64_t total_symbols = static_cast<uint64_t> (ceil (
                                _size / static_cast<double> (_symbol_size)));

        part = Impl::Partition (total_symbols, static_cast<uint8_t> (_blocks));
        pool_last_reported = -1;
        _pool_notify = std::make_shared<std::condition_variable>();
        _pool_mtx = std::make_shared<std::mutex>();
        use_pool = true;
        exiting = false;
    }

    Decoder (const uint64_t size, const uint16_t symbol_size,
                                                    const uint16_t sub_blocks,
                                                    const uint8_t blocks,
                                                    const uint8_t alignment)
        :_size (size), _symbol_size (symbol_size), _blocks (blocks),
                                                        _alignment(alignment)
    {
        // _size > max_data means improper initialization.
        if (_size > max_data || _size % _alignment != 0 ||
                                            _symbol_size % _alignment != 0) {
            // really, not all the possible tests are here.
            // but the RFC sucks really bad... input validation is a pain...
            // please use the RAW API...
            _size = std::numeric_limits<uint64_t>::max();
            return;
        }

        const uint64_t total_symbols = static_cast<uint64_t> (ceil (
                                _size / static_cast<double> (_symbol_size)));
        _sub_blocks = Impl::Partition (_symbol_size / _alignment, sub_blocks);

        part = Impl::Partition (total_symbols, static_cast<uint8_t> (_blocks));
        _pool_notify = std::make_shared<std::condition_variable>();
        _pool_mtx = std::make_shared<std::mutex>();
        pool_last_reported = -1;
        use_pool = true;
        exiting = false;
    }
    It::Decoder::Block_Iterator<In_It, Fwd_It> begin ()
        { return It::Decoder::Block_Iterator<In_It, Fwd_It> (this, 0); }
    const It::Decoder::Block_Iterator<In_It, Fwd_It> end ()
        { return It::Decoder::Block_Iterator<In_It, Fwd_It> (this, blocks()); }
    operator bool() const
        { return _size <= max_data; }

    std::future<std::pair<Error, uint8_t>> compute (const Compute flags);
    // if you can tell there is no more input, we can avoid locking
    // forever and return an error.
    void end_of_input (const uint8_t block);
    void end_of_input();

    // result in BYTES
    uint64_t decode_symbol (Fwd_It &start, const Fwd_It end, const uint16_t esi,
                                                            const uint8_t sbn);
    uint64_t decode_bytes (Fwd_It &start, const Fwd_It end, const uint8_t skip);
    size_t decode_block_bytes (Fwd_It &start, const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn);
    // result in ITERATORS
    // last *might* be half written depending on data alignments
    // NOTE: skip = uint8_t to avoid problems with _alignment
    Decoder_aligned_res decode_aligned (Fwd_It &start,const Fwd_It end,
                                                            const uint8_t skip);
    Decoder_aligned_res decode_block_aligned (Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn);
    // id: 8-bit sbn + 24 bit esi
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
    // using shared pointers to avoid locking too much or
    // worrying about deleting used stuff.
    class RAPTORQ_LOCAL Block_Work final : public Impl::Pool_Work {
    public:
        std::weak_ptr<RaptorQ__v1::Impl::Raw_Decoder<In_It>> work;
        std::weak_ptr<std::condition_variable> notify;
        std::weak_ptr<std::mutex> lock;

        Work_Exit_Status do_work (RaptorQ__v1::Work_State *state) override;
        ~Block_Work() override;
    };
    // TODO: tagged pointer
    class RAPTORQ_LOCAL Dec {
    public:
        Dec (const RaptorQ__v1::Block_Size symbols, const uint16_t symbol_size)
        {
            dec = std::make_shared<RaptorQ__v1::Impl::Raw_Decoder<In_It>> (
                                                          symbols, symbol_size);
            reported = false;
        }
        std::shared_ptr<RaptorQ__v1::Impl::Raw_Decoder<In_It>> dec;
        bool reported;
    };

    static void wait_threads (Decoder<In_It, Fwd_It> *obj, const Compute flags,
                                    std::promise<std::pair<Error, uint8_t>> p);
    std::pair<Error, uint8_t> get_report (const Compute flags);
    std::shared_ptr<std::condition_variable> _pool_notify;
    std::shared_ptr<std::mutex> _pool_mtx;
    std::deque<std::thread> pool_wait;

    uint64_t _size;
    Impl::Partition part, _sub_blocks;
    std::map<uint8_t, Dec> decoders;
    std::mutex _mtx;
    uint16_t _symbol_size;
    int16_t pool_last_reported;
    uint8_t _blocks, _alignment;
    bool use_pool, exiting;

    std::vector<bool> decoded_sbn;

};


/////////////////
//
// Encoder
//
/////////////////


template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::~Encoder()
{
    exiting = true; // stop notifying thread
    std::unique_lock<std::mutex> enc_lock (_mtx);
    for (auto &it : encoders) { // stop existing computations
        auto ptr = it.second.enc;
        if (ptr != nullptr)
            ptr->stop();
    }
    enc_lock.unlock();
    _pool_notify->notify_all();
    while (pool_wait.size() != 0) {
        std::unique_lock<std::mutex> lock (*_pool_mtx);
        if (pool_wait.size() != 0)
            _pool_notify->wait (lock);
    }
}

template <typename Rnd_It, typename Fwd_It>
RFC6330_OTI_Common_Data Encoder<Rnd_It, Fwd_It>::OTI_Common() const
{
    if (!interleave)
        return 0;
    RFC6330_OTI_Common_Data ret;
    // first 40 bits: data length.
    ret = (static_cast<uint64_t> (_data_to - _data_from) *
            sizeof(typename std::iterator_traits<Rnd_It>::value_type)) << 24;
    // 8 bits: reserved
    // last 16 bits: symbol size
    ret += _symbol_size;

    return ret;
}

template <typename Rnd_It, typename Fwd_It>
RFC6330_OTI_Scheme_Specific_Data Encoder<Rnd_It, Fwd_It>::OTI_Scheme_Specific() const
{
    if (!interleave)
        return 0;
    RFC6330_OTI_Scheme_Specific_Data ret;
    // 8 bit: source blocks
    ret = static_cast<uint32_t> (interleave.blocks()) << 24;
    // 16 bit: sub-blocks number (N)
    ret += static_cast<uint32_t> (interleave.sub_blocks()) << 8;
    // 8 bit: alignment
    ret += sizeof(typename std::iterator_traits<Rnd_It>::value_type);

    return ret;
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::precompute_max_memory ()
{
    // give a good estimate on the amount of memory neede for the precomputation
    // of one block;
    // this will help you understand how many concurrent precomputations
    // you want to do :)

    if (!interleave)
        return 0;

    uint16_t symbols = interleave.source_symbols (0);

    uint16_t K_idx;
    for (K_idx = 0; K_idx < RaptorQ__v1::Impl::K_padded.size(); ++K_idx) {
        if (symbols < RaptorQ__v1::Impl::K_padded[K_idx])
            break;
    }
    if (K_idx == RaptorQ__v1::Impl::K_padded.size())
        return 0;

    auto S_H_W = RaptorQ__v1::Impl::S_H_W[K_idx];
    enum Tup { S = 0, H = 1, W = 2 };
    uint16_t matrix_cols = RaptorQ__v1::Impl::K_padded[K_idx] +
                                                    std::get<Tup::S> (S_H_W) +
                                                    std::get<Tup::H> (S_H_W);

    // Rough memory estimate: Matrix A, matrix X (=> *2) and matrix D.
    return matrix_cols * matrix_cols * 2 + _symbol_size * matrix_cols;
}
template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::Block_Work::~Block_Work()
{
    // cleanup. have we benn called before the computation finished?
    auto locked_enc = work.lock();
    auto locked_notify = notify.lock();
    auto locked_mtx = lock.lock();
    if (locked_enc != nullptr && locked_notify != nullptr &&
                                                        locked_mtx != nullptr) {
        locked_enc->stop();
        std::unique_lock<std::mutex> p_lock (*locked_mtx);
        RQ_UNUSED(p_lock);
        locked_notify->notify_all();
    }
}

template <typename Rnd_It, typename Fwd_It>
Work_Exit_Status Encoder<Rnd_It, Fwd_It>::Block_Work::do_work (
                                                RaptorQ__v1::Work_State *state)
{
    auto locked_enc = work.lock();
    auto locked_notify = notify.lock();
    auto locked_mtx = lock.lock();
    if (locked_enc != nullptr && locked_notify != nullptr &&
                                                        locked_mtx != nullptr) {
        // encoding always works. It's one of the few constants of the universe.
        if (!locked_enc->generate_symbols (state))
            return Work_Exit_Status::STOPPED;   // or maybe not so constant
        work.reset();
        std::unique_lock<std::mutex> p_lock (*locked_mtx);
        RQ_UNUSED(p_lock);
        locked_notify->notify_all();
    }
    return Work_Exit_Status::DONE;
}

template <typename Rnd_It, typename Fwd_It>
std::future<std::pair<Error, uint8_t>> Encoder<Rnd_It, Fwd_It>::compute (
                                                            const Compute flags)
{
    using ret_t = std::pair<Error, uint8_t>;
    std::promise<ret_t> p;

    bool error = !interleave;
    // need some flags
    if (flags == Compute::NONE)
        error = true;

    // flag incompatibilities
    if (Compute::NONE != (flags & Compute::PARTIAL_FROM_BEGINNING) &&
                    (Compute::NONE != (flags & (Compute::PARTIAL_ANY |
                                                Compute::COMPLETE |
                                                Compute::NO_POOL)))) {
            error = true;
    } else if (Compute::NONE != (flags & Compute::PARTIAL_ANY) &&
                (Compute::NONE != (flags & (Compute::PARTIAL_FROM_BEGINNING |
                                            Compute::COMPLETE |
                                            Compute::NO_POOL)))) {
            error = true;
    } else if (Compute::NONE != (flags & Compute::COMPLETE) &&
                    Compute::NONE != (flags &(Compute::PARTIAL_FROM_BEGINNING |
                                                Compute::PARTIAL_ANY |
                                                Compute::NO_POOL))) {
            error = true;
    }

    if (Compute::NONE != (flags & Compute::NO_POOL)) {
        std::unique_lock<std::mutex> lock (_mtx);
        RQ_UNUSED(lock);
        if (encoders.size() != 0) {
            // You can only say you won't use the pool *before* you start
            // decoding something!
            error = true;
        } else {
            use_pool = false;
            p.set_value ({Error::NONE, 0});
            return p.get_future();
        }
    }

    if (error) {
        p.set_value ({Error::WRONG_INPUT, 0});
        return p.get_future();
    }

    // flags are fine, add work to pool
    std::unique_lock<std::mutex> lock (_mtx);
    for (uint8_t block = 0; block < blocks(); ++block) {
        auto enc = encoders.find (block);
        if (enc == encoders.end()) {
            bool success;
            std::tie (enc, success) = encoders.emplace (
                                    std::piecewise_construct,
                                    std::forward_as_tuple (block),
                                    std::forward_as_tuple (&interleave, block));
            assert (success == true);
            std::unique_ptr<Block_Work> work = std::unique_ptr<Block_Work>(
                                                            new Block_Work());
            work->work = enc->second.enc;
            work->notify = _pool_notify;
            work->lock = _pool_mtx;
            Thread_Pool::get().add_work (std::move(work));
        }
    }
    lock.unlock();

    // spawn thread waiting for other thread exit.
    // this way we can set_value to the future when needed.
    auto future = p.get_future();
    if (Compute::NONE != (flags & Compute::NO_BACKGROUND)) {
        wait_threads (this, flags, std::move(p));
    } else {
        std::unique_lock<std::mutex> pool_wait_lock (*_pool_mtx);
        RQ_UNUSED(pool_wait_lock);
        pool_wait.emplace_back(wait_threads, this, flags, std::move(p));
    }
    return future;
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::wait_threads (Encoder<Rnd_It, Fwd_It> *obj,
                                    const Compute flags,
                                    std::promise<std::pair<Error, uint8_t>> p)
{
    auto _notify = obj->_pool_notify;
    while (true) {
        std::unique_lock<std::mutex> lock (*obj->_pool_mtx);
        if (obj->exiting) {
            p.set_value ({Error::EXITING, 0});
            break;
        }
        auto status = obj->get_report (flags);
        if (Error::WORKING != status.first) {
            p.set_value (status);
            break;
        }

        _notify->wait (lock);
        lock.unlock();
    }

    // delete ourselves from the waiting thread vector.
    std::unique_lock<std::mutex> lock (*obj->_pool_mtx);
    for (auto it = obj->pool_wait.begin(); it != obj->pool_wait.end(); ++it) {
        if (it->get_id() == std::this_thread::get_id()) {
            it->detach();
            obj->pool_wait.erase (it);
            break;
        }
    }
    lock.unlock();
    _notify->notify_all();
}

template <typename Rnd_It, typename Fwd_It>
std::pair<Error, uint8_t> Encoder<Rnd_It, Fwd_It>::get_report (
                                                        const Compute flags)
{
    if (encoders.size() == 0)
        return {Error::WORKING, 0};
    if (Compute::NONE != (flags & Compute::COMPLETE) ||
                Compute::NONE != (flags & Compute::PARTIAL_FROM_BEGINNING)) {
        auto it = encoders.begin();
        for (; it != encoders.end(); ++it) {
            auto ptr = it->second.enc;
            if (ptr != nullptr) {
                if (!ptr->ready()) {
                    if (ptr->is_stopped())
                        return{Error::EXITING, 0};
                    break;
                }
            }
        }
        if (it == encoders.end()) {
            pool_last_reported = static_cast<int16_t> (encoders.size() - 1);
            return {Error::NONE, static_cast<uint8_t>(pool_last_reported)};
        }
        if (Compute::NONE != (flags & Compute::PARTIAL_FROM_BEGINNING) &&
                                    (pool_last_reported < (it->first - 1))) {
            pool_last_reported = it->first - 1;
            return {Error::NONE, static_cast<uint8_t>(pool_last_reported)};
        }
        return {Error::WORKING, 0};
    }
    if (Compute::NONE != (flags & Compute::PARTIAL_ANY)) {
        for (auto &it : encoders) {
            if (!it.second.reported) {
                auto ptr = it.second.enc;
                if (ptr != nullptr) {
                    if (ptr->ready())
                        return {Error::NONE, it.first};
                    if (ptr->is_stopped())
                        return{Error::EXITING, 0};
                }
            }
        }
    }
    return {Error::WORKING, 0}; // should never be reached
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t &id)
{
    constexpr uint32_t mask = ~(static_cast<uint32_t>(0xFF) << 24);

    return encode (output, end, id & mask, static_cast<uint8_t> (id >> 24));
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t esi,
                                                            const uint8_t sbn)
{
    if (sbn >= interleave.blocks())
        return 0;

    std::unique_lock<std::mutex> lock (_mtx);
    auto it = encoders.find (sbn);
    if (use_pool) {
        if (it == encoders.end())
            return 0;
        auto shared_enc = it->second.enc;
        if (!shared_enc->ready())
            return 0;
        lock.unlock();
        return shared_enc->Enc (esi, output, end);
    } else {
        if (it == encoders.end()) {
            bool success;
            std::tie (it, success) = encoders.emplace (std::make_pair (sbn,
                                                    Enc (&interleave, sbn)));
            auto shared_enc = it->second.enc;
            lock.unlock();
            RaptorQ__v1::Work_State state =
                                        RaptorQ__v1::Work_State::KEEP_WORKING;
            shared_enc->generate_symbols (&state);
            return shared_enc->Enc (esi, output, end);
        } else {
            auto shared_enc = it->second.enc;
            lock.unlock();
            if (!shared_enc->ready())
                return 0;
            return shared_enc->Enc (esi, output, end);
        }
    }
}


template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::free (const uint8_t sbn)
{
    std::unique_lock<std::mutex> lock (_mtx);
    RQ_UNUSED(lock);
    auto it = encoders.find (sbn);
    if (it != encoders.end())
        encoders.erase (it);
}

template <typename Rnd_It, typename Fwd_It>
uint8_t Encoder<Rnd_It, Fwd_It>::blocks() const
{
    if (!interleave)
        return 0;
    return interleave.blocks();
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::block_size (const uint8_t sbn) const
{
    if (!interleave)
        return 0;
    return interleave.source_symbols (sbn) * interleave.symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
{
    if (!interleave)
        return 0;
    return interleave.symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols (const uint8_t sbn) const
{
    if (!interleave)
        return 0;
    return interleave.source_symbols (sbn);
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair (const uint8_t sbn) const
{
    if (!interleave)
        return 0;
    return static_cast<uint32_t> (std::pow (2, 20)) -
                                            interleave.source_symbols (sbn);
}

/////////////////
//
// Decoder
//
/////////////////

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::~Decoder()
{
    exiting = true; // stop notifying thread
    _mtx.lock();
    for (auto &it : decoders) { // stop existing computations
        auto ptr = it.second.dec;
        if (ptr != nullptr)
            ptr->stop();
    }
    _mtx.unlock();
    _pool_notify->notify_all();
    while (pool_wait.size() != 0) {
        std::unique_lock<std::mutex> lock (*_pool_mtx);
        if (pool_wait.size() != 0)
            _pool_notify->wait (lock);
    }
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::free (const uint8_t sbn)
{
    _mtx.lock();
    auto it = decoders.find(sbn);
    if (it != decoders.end())
        decoders.erase(it);
    _mtx.unlock();
    _pool_notify->notify_all();
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
                                                            const uint32_t id)
{
    uint32_t esi = (id << 8 ) >> 8;
    uint8_t sbn = id >> 24;

    return add_symbol (start, end, esi, sbn);
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &start, const In_It end,
                                        const uint32_t esi, const uint8_t sbn)
{
    if (!operator bool())
        return Error::INITIALIZATION;
    if (sbn >= _blocks)
        return Error::WRONG_INPUT;
    std::unique_lock<std::mutex> lock (_mtx);
    auto it = decoders.find (sbn);
    if (it == decoders.end()) {
        const uint16_t symbols = sbn < part.num (0) ?
                                                    part.size(0) : part.size(1);
        bool success;
        std::tie (it, success) = decoders.emplace (std::make_pair(sbn,
            Dec (static_cast<RaptorQ__v1::Block_Size>(symbols), _symbol_size)));
        assert (success);
    }
    auto dec = it->second.dec;
    lock.unlock();

    auto err = dec->add_symbol (start, end, esi);
    if (err != Error::NONE)
        return err;
    // automatically add work to pool if we use it and have enough data
    std::unique_lock<std::mutex> pool_lock (*_pool_mtx);
    RQ_UNUSED(pool_lock);
    if (use_pool && dec->can_decode()) {
        bool add_work = dec->add_concurrent (max_block_decoder_concurrency);
        if (add_work) {
            std::unique_ptr<Block_Work> work = std::unique_ptr<Block_Work>(
                                                            new Block_Work());
            work->work = dec;
            work->notify = _pool_notify;
            work->lock = _pool_mtx;
            Impl::Thread_Pool::get().add_work (std::move(work));
        }
    }
    return Error::NONE;
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::end_of_input()
{
    if (!operator bool())
        return;
    std::unique_lock<std::mutex> pool_lock (*_pool_mtx);
    std::unique_lock<std::mutex> dec_lock (_mtx);
    for (auto &it : decoders)
        it.second.dec->end_of_input = true;
    dec_lock.unlock();
    pool_lock.unlock();
    _pool_notify->notify_all();
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::end_of_input (const uint8_t block)
{
    if (!operator bool())
        return;
    std::unique_lock<std::mutex> pool_lock (*_pool_mtx);
    std::unique_lock<std::mutex> dec_lock (_mtx);
    auto it = decoders.find(block);
    if (it != decoders.end()) {
        it->second.dec->end_of_input = true;
        dec_lock.unlock();
        pool_lock.unlock();
        _pool_notify->notify_all();
    }
}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::Block_Work::~Block_Work()
{
    // have we been called before the computation finished?
    auto locked_dec = work.lock();
    auto locked_notify = notify.lock();
    auto locked_mtx = lock.lock();
    if (locked_dec != nullptr && locked_notify != nullptr &&
                                                        locked_mtx != nullptr) {
        locked_dec->stop();
        std::unique_lock<std::mutex> p_lock (*locked_mtx);
        RQ_UNUSED(p_lock);
        locked_notify->notify_all();
    }
}

template <typename In_It, typename Fwd_It>
Work_Exit_Status Decoder<In_It, Fwd_It>::Block_Work::do_work (
                                                RaptorQ__v1::Work_State *state)
{
    auto locked_dec = work.lock();
    auto locked_notify = notify.lock();
    auto locked_mtx = lock.lock();
    if (locked_dec != nullptr && locked_notify != nullptr &&
                                                        locked_mtx != nullptr) {
        auto ret = locked_dec->decode (state);
        std::unique_lock<std::mutex> p_lock (*locked_mtx, std::defer_lock);
        switch (ret) {
        case RaptorQ__v1::Decoder_Result::DECODED:
            work.reset();
            p_lock.lock();
            locked_dec->drop_concurrent();
            locked_notify->notify_all();
            p_lock.unlock();
            return Work_Exit_Status::DONE;
        case RaptorQ__v1::Decoder_Result::NEED_DATA:
            p_lock.lock();
            if (locked_dec->can_decode()) {
                // check again to avoid race between threads
                return Work_Exit_Status::REQUEUE;
            } else {
                locked_dec->drop_concurrent();
                if (locked_dec->end_of_input && locked_dec->threads() == 0)
                    locked_notify->notify_all();
                p_lock.unlock();
                work.reset();
                return Work_Exit_Status::DONE;
            }
        case RaptorQ__v1::Decoder_Result::STOPPED:
            p_lock.lock();
            if (locked_dec->ready()) { // did an other thread stop us?
                locked_dec->drop_concurrent();
                work.reset();
                return Work_Exit_Status::DONE;
            }
            // requeued. Do not drop_concurrent
            return Work_Exit_Status::STOPPED;
        case RaptorQ__v1::Decoder_Result::CAN_RETRY:
            // requeued. Do not drop_concurrent
            return Work_Exit_Status::REQUEUE;
        }
    }
    return Work_Exit_Status::DONE;
}

template <typename In_It, typename Fwd_It>
std::future<std::pair<Error, uint8_t>> Decoder<In_It, Fwd_It>::compute (
                                                        const Compute flags)
{
    using ret_t = std::pair<Error, uint8_t>;
    std::promise<ret_t> p;

    bool error = !operator bool();    // test correct class initialization
    // need some flags
    if (flags == Compute::NONE)
        error = true;

    // flag incompatibilities
    if (Compute::NONE != (flags & Compute::PARTIAL_FROM_BEGINNING) &&
                            (Compute::NONE != (flags & (Compute::PARTIAL_ANY |
                                                        Compute::COMPLETE |
                                                        Compute::NO_POOL)))) {
        error = true;
    } else if (Compute::NONE != (flags & Compute::PARTIAL_ANY) &&
                (Compute::NONE != (flags & (Compute::PARTIAL_FROM_BEGINNING |
                                            Compute::COMPLETE |
                                            Compute::NO_POOL)))) {
        error = true;
    } else if (Compute::NONE != (flags & Compute::COMPLETE) &&
                    Compute::NONE != (flags &(Compute::PARTIAL_FROM_BEGINNING |
                                                Compute::PARTIAL_ANY |
                                                Compute::NO_POOL))) {
        error = true;
    }

    if (Compute::NONE != (flags & Compute::NO_POOL)) {
        std::unique_lock<std::mutex> lock (_mtx);
        RQ_UNUSED(lock);
        if (decoders.size() != 0) {
            // You can only say you won't use the pool *before* you start
            // decoding something!
            error = true;
        } else {
            use_pool = false;
            p.set_value ({Error::NONE, 0});
            return p.get_future();
        }
    }

    if (error) {
        p.set_value ({Error::WRONG_INPUT, 0});
        return p.get_future();
    }

    // do not add work to the pool to save up memory.
    // let "add_symbol craete the Decoders as needed.

    // spawn thread waiting for other thread exit.
    // this way we can set_value to the future when needed.
    auto future = p.get_future();
    if (Compute::NONE != (flags & Compute::NO_BACKGROUND)) {
        wait_threads (this, flags, std::move(p));
    } else {
        std::unique_lock<std::mutex> pool_wait_lock (*_pool_mtx);
        RQ_UNUSED(pool_wait_lock);
        pool_wait.emplace_back (wait_threads, this, flags, std::move(p));
    }
    return future;
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::wait_threads (Decoder<In_It, Fwd_It> *obj,
                                    const Compute flags,
                                    std::promise<std::pair<Error, uint8_t>> p)
{
    auto _notify = obj->_pool_notify;
    while (true) {
        std::unique_lock<std::mutex> lock (*obj->_pool_mtx);
        if (obj->exiting) { // make sure we can exit
            p.set_value ({Error::EXITING, 0});
            break;
        }
        auto status = obj->get_report (flags);
        if (Error::WORKING != status.first) {
            p.set_value (status);
            break;
        }

        _notify->wait (lock);
        lock.unlock();
    }

    // delete ourselves from the waiting thread vector.
    std::unique_lock<std::mutex> lock (*obj->_pool_mtx);
    for (auto it = obj->pool_wait.begin(); it != obj->pool_wait.end(); ++it) {
        if (it->get_id() == std::this_thread::get_id()) {
            it->detach();
            obj->pool_wait.erase (it);
            break;
        }
    }
    lock.unlock();
    _notify->notify_all();
}

template <typename In_It, typename Fwd_It>
std::pair<Error, uint8_t> Decoder<In_It, Fwd_It>::get_report (
                                                            const Compute flags)
{
    if (decoders.size() == 0)
        return {Error::WORKING, 0};
    if (Compute::COMPLETE == (flags & Compute::COMPLETE) ||
            Compute::PARTIAL_FROM_BEGINNING ==
                                    (flags & Compute::PARTIAL_FROM_BEGINNING)) {
        uint16_t reportable = 0;
        uint16_t next_expected = static_cast<uint16_t> (pool_last_reported + 1);
        std::unique_lock<std::mutex> dec_lock (_mtx);
        auto it = decoders.lower_bound (static_cast<uint8_t> (next_expected));

        // get last reportable block
        for (; it != decoders.end(); ++it) {
            auto id = it->first;
            if (id != next_expected)
                break; // not consecutive
            auto ptr = it->second.dec;
            if (ptr == nullptr) {
                assert(false && "RFC6330: decoder should never be nullptr.");
                break;
            }
            if (!ptr->ready()) {
                if (ptr->is_stopped())
                    return {Error::EXITING, 0};
                if (ptr->end_of_input && ptr->threads() == 0)
                    return {Error::NEED_DATA, 0};
                break; // still working
            }
            ++reportable;
            ++next_expected;
        }
        dec_lock.unlock();
        if (reportable > 0) {
            pool_last_reported += reportable;
            if (Compute::PARTIAL_FROM_BEGINNING ==
                                    (flags & Compute::PARTIAL_FROM_BEGINNING)) {
                return {Error::NONE, static_cast<uint8_t>(pool_last_reported)};
            } else {
                // complete
                if (pool_last_reported == _blocks - 1)
                    return {Error::NONE,
                                    static_cast<uint8_t>(pool_last_reported)};
            }
        }
    } else if (Compute::PARTIAL_ANY == (flags & Compute::PARTIAL_ANY)) {
        // invalidate other pointers.
        auto undecodable = decoders.end();
        std::unique_lock<std::mutex> dec_lock (_mtx);
        RQ_UNUSED(dec_lock);
        for (auto it = decoders.begin(); it != decoders.end(); ++it) {
            if (!it->second.reported) {
                auto ptr = it->second.dec;
                if (ptr == nullptr) {
                    assert(false && "RFC6330: decoder should never be nullptr");
                    break;
                }
                if (ptr->ready()) {
                    it->second.reported = true;
                    return {Error::NONE, it->first};
                }
                if (ptr->is_stopped())
                    return {Error::EXITING, 0};
                // first return all decodable blocks
                // then return the ones we can not decode.
                if (ptr->end_of_input && ptr->threads() == 0)
                    undecodable = it;
            }
        }
        if (undecodable != decoders.end()) {
            undecodable->second.reported = true;
            return {Error::NEED_DATA, undecodable->first};
        }
    }
    // can be reached if computing thread was stopped
    return {Error::WORKING, 0};
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::decode_symbol (Fwd_It &start, const Fwd_It end,
                                                            const uint16_t esi,
                                                            const uint8_t sbn)
{
    if (!operator bool() || sbn > blocks() || esi > symbols (sbn))
        return 0;


    std::shared_ptr<RaptorQ__v1::Impl::Raw_Decoder<In_It>> dec_ptr = nullptr;
    std::unique_lock<std::mutex> lock (_mtx);
    auto it = decoders.find (sbn);

    if (it == decoders.end())
        return 0;   // did not receiveany data yet.

    if (use_pool) {
        dec_ptr = it->second.dec;
        lock.unlock();
        if (!dec_ptr->ready())
            return 0;   // did not receive enough data, or could not decode yet.
    } else {
        dec_ptr = it->second.dec;
        lock.unlock();
        if (!dec_ptr->ready()) {
            if (!dec_ptr->can_decode())
                return 0;
            RaptorQ__v1::Work_State keep_working =
                                        RaptorQ__v1::Work_State::KEEP_WORKING;
            dec_ptr->decode (&keep_working);
            if (!dec_ptr->ready())
                return 0;
        }
    }
    // decoder has decoded the block

    Impl::De_Interleaver<Fwd_It> de_interleaving (dec_ptr->get_symbols(),
                                                    _sub_blocks, _alignment);
    size_t max_bytes = block_size (sbn);
    if (sbn == (blocks() - 1)) {
        // size of the data (_size) is different from the sum of the size of
        // all blocks. get the real size, so we do not write more.
        // we obviously need to consider this only for the last block.
        uint64_t all_blocks = 0;
        for (uint8_t id = 0; id < blocks(); ++id)
            all_blocks += block_size (sbn);
        const uint64_t diff = all_blocks - _size;
        max_bytes -= static_cast<size_t>(diff);
    }
    // find the end:
    auto real_end = start;
    size_t fwd_iter_for_symbol = symbol_size() /
                    sizeof(typename std::iterator_traits<Fwd_It>::value_type);
    // be sure that 'end' points AT MAX to the end of the symbol
    if (std::is_same<typename std::iterator_traits<Fwd_It>::iterator_category,
                                    std::random_access_iterator_tag>::value) {
        real_end += fwd_iter_for_symbol;
        if (real_end > end)
            real_end = end;
    } else {
        // sory, fwd_iterators do not have comparison operators :(
        while (real_end != end && fwd_iter_for_symbol != 0)
            ++real_end;
    }
    return de_interleaving (start, end, max_bytes, 0, esi);
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start, const Fwd_It end,
                                                        const uint8_t skip)
{
    if (!operator bool())
        return 0;
    // Decode from the beginning, up untill we can.
    // return number of BYTES written, starting at "start + skip" bytes
    //
    // in case the last iterator is only half written, "start" will
    // point to the half-written iterator.

    uint64_t written = 0;
    uint8_t new_skip = skip;
    for (uint8_t sbn = 0; sbn < blocks(); ++sbn) {
        std::unique_lock<std::mutex> block_lock (_mtx);
        auto it = decoders.find (sbn);
        if (it == decoders.end())
            return written;
        auto dec_ptr = it->second.dec;
        block_lock.unlock();

        if (!dec_ptr->ready()) {
            if (!use_pool && dec_ptr->can_decode()) {
                RaptorQ__v1::Work_State state =
                                        RaptorQ__v1::Work_State::KEEP_WORKING;
                auto ret = dec_ptr->decode (&state);
                if (RaptorQ__v1::Decoder_Result::DECODED != ret) {
                    return written;
                }
            } else {
                return written;
            }
        }

        Impl::De_Interleaver<Fwd_It> de_interleaving (dec_ptr->get_symbols(),
                                                    _sub_blocks, _alignment);

        size_t max_bytes = block_size (sbn);
        if (sbn == (blocks() - 1)) {
            // size of the data (_size) is different from the sum of the size of
            // all blocks. get the real size, so we do not write more.
            // we obviously need to consider this only for the last block.
            uint64_t all_blocks = 0;
            for (uint8_t id = 0; id < blocks(); ++id)
                all_blocks += block_size (sbn);
            const size_t diff = static_cast<size_t> (all_blocks - _size);
            max_bytes -= diff;
        }
        auto tmp_start = start;
        uint64_t bytes_written = de_interleaving (tmp_start, end, max_bytes,
                                                                    new_skip);
        written += bytes_written;
        uint64_t bytes_and_skip = new_skip + bytes_written;
        new_skip = bytes_and_skip %
                    sizeof(typename std::iterator_traits<Fwd_It>::value_type);
        if (bytes_written == 0)
            return written;
        //new_skip = block_size (sbn) %
        //          sizeof(typename std::iterator_traits<Fwd_It>::value_type);
        // if we ended decoding in the middle of a Fwd_It, do not advance
        // start too much, or we will end up having additional zeros.
        if (new_skip == 0) {
            start = tmp_start;
        } else {
            uint64_t it_written = bytes_and_skip /
                    sizeof(typename std::iterator_traits<Fwd_It>::value_type);
            // RaptorQ handles at most 881GB per rfc, so
            // casting uint64 to int64 is safe
            // we can not do "--start" since it's a forward iterator
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wshorten-64-to-32"
            #pragma clang diagnostic ignored "-Wsign-conversion"
            start += std::max (static_cast<uint64_t>(0), it_written - 1);
            #pragma clang diagnostic pop
        }
    }
    return written;
}

template <typename In_It, typename Fwd_It>
size_t Decoder<In_It, Fwd_It>::decode_block_bytes (Fwd_It &start,
                                                            const Fwd_It end,
                                                            const uint8_t skip,
                                                            const uint8_t sbn)
{
    if (!operator bool() || sbn >= _blocks)
        return 0;

    std::shared_ptr<RaptorQ__v1::Impl::Raw_Decoder<In_It>> dec_ptr = nullptr;
    std::unique_lock<std::mutex> lock (_mtx);
    auto it = decoders.find (sbn);

    if (it == decoders.end())
        return 0;   // did not receiveany data yet.

    if (use_pool) {
        dec_ptr = it->second.dec;
        lock.unlock();
        if (!dec_ptr->ready())
            return 0;   // did not receive enough data, or could not decode yet.
    } else {
        dec_ptr = it->second.dec;
        lock.unlock();
        if (!dec_ptr->ready()) {
            if (!dec_ptr->can_decode())
                return 0;
            RaptorQ__v1::Work_State keep_working =
                                        RaptorQ__v1::Work_State::KEEP_WORKING;
            dec_ptr->decode (&keep_working);
            if (!dec_ptr->ready())
                return 0;
        }
    }
    // decoder has decoded the block

    Impl::De_Interleaver<Fwd_It> de_interleaving (dec_ptr->get_symbols(),
                                                    _sub_blocks, _alignment);
    size_t max_bytes = block_size (sbn);
    if (sbn == (blocks() - 1)) {
        // size of the data (_size) is different from the sum of the size of
        // all blocks. get the real size, so we do not write more.
        // we obviously need to consider this only for the last block.
        uint64_t all_blocks = 0;
        for (uint8_t id = 0; id < blocks(); ++id)
            all_blocks += block_size (sbn);
        const uint64_t diff = all_blocks - _size;
        max_bytes -= static_cast<size_t>(diff);
    }
    return de_interleaving (start, end, max_bytes, skip);
}

template <typename In_It, typename Fwd_It>
Decoder_aligned_res Decoder<In_It, Fwd_It>::decode_aligned (
                                                        Fwd_It &start,
                                                        const Fwd_It end,
                                                        const uint8_t skip)
{
    const uint64_t bytes = decode_bytes (start, end, skip);
    const uint64_t skip_and_bytes = skip + bytes;
    const uint64_t iterators = skip_and_bytes /
                    sizeof(typename std::iterator_traits<Fwd_It>::value_type);
    const uint8_t new_skip = skip_and_bytes %
                    sizeof(typename std::iterator_traits<Fwd_It>::value_type);
    return {iterators, new_skip};
}

template <typename In_It, typename Fwd_It>
Decoder_aligned_res Decoder<In_It, Fwd_It>::decode_block_aligned (
                                                        Fwd_It &start,
                                                        const Fwd_It end,
                                                        const uint8_t skip,
                                                        const uint8_t sbn)
{
    const size_t bytes = decode_block_bytes (start, end, skip, sbn);
    const size_t skip_and_bytes = skip + bytes;
    const size_t iterators = skip_and_bytes /
                    sizeof(typename std::iterator_traits<Fwd_It>::value_type);
    const uint8_t new_skip = skip_and_bytes %
                    sizeof(typename std::iterator_traits<Fwd_It>::value_type);
    return {iterators, new_skip};
}

template <typename In_It, typename Fwd_It>
uint64_t Decoder<In_It, Fwd_It>::bytes() const
{
    if (!operator bool())
        return 0;
    return _size;
}

template <typename In_It, typename Fwd_It>
uint8_t Decoder<In_It, Fwd_It>::blocks() const
{
    if (!operator bool())
        return 0;
    return static_cast<uint8_t> (part.num (0) + part.num (1));
}

template <typename In_It, typename Fwd_It>
uint32_t Decoder<In_It, Fwd_It>::block_size (const uint8_t sbn) const
{
    if (!operator bool())
        return 0;
    if (sbn < part.num (0)) {
        return part.size (0) * _symbol_size;
    } else if (sbn - part.num (0) < part.num (1)) {
        return part.size (1) * _symbol_size;
    }
    return 0;
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbol_size() const
{
    if (!operator bool())
        return 0;
    return _symbol_size;
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbols (const uint8_t sbn) const
{
    if (!operator bool())
        return 0;
    if (sbn < part.num (0)) {
        return part.size (0);
    } else if (sbn - part.num (0) < part.num (1)) {
        return part.size (1);
    }
    return 0;
}

}   // namespace Impl
}   // namespace RFC6330__v1

