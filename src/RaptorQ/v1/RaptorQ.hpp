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
#include "RaptorQ/v1/block_sizes.hpp"
#ifdef RQ_HEADER_ONLY
    #include "RaptorQ/v1/RaptorQ_Iterators.hpp"
#endif
#include "RaptorQ/v1/Encoder.hpp"
#include "RaptorQ/v1/Decoder.hpp"
#include "RaptorQ/v1/Parameters.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>
#include <utility>



namespace RaptorQ__v1 {

namespace Impl {
template <typename Rnd_It, typename Fwd_It = Rnd_It>
class RAPTORQ_LOCAL Encoder;
template <typename In_It, typename Fwd_It = In_It>
class RAPTORQ_LOCAL Decoder;
} // namespace Impl

// expose classes, but only if header-only
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
    ~Encoder();
    // used for precomputation
    Encoder (const Block_Size symbols, const size_t symbol_size);
    Encoder() = delete;
    Encoder (const Encoder&) = delete;
    Encoder& operator= (const Encoder&) = delete;
    Encoder (Encoder &&) = delete;
    Encoder& operator= (Encoder &&) = delete;

    explicit operator bool() const;

    uint16_t symbols() const;
    size_t symbol_size() const;
    uint32_t max_repair() const;

#ifdef RQ_HEADER_ONLY
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin_repair();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end_repair
                                                        (const uint32_t repair);
#endif

    bool has_data() const;
    size_t set_data (const Rnd_It &from, const Rnd_It &to);
    void clear_data();
    bool ready() const;
    void stop();

    bool precompute_sync();
    bool compute_sync();
    std::shared_future<Error> precompute();
    std::shared_future<Error> compute();

    size_t encode (Fwd_It &output, const Fwd_It end, const uint32_t id);

private:
    enum class Enc_State : uint8_t {
        INIT_ERROR = 1,
        NEED_DATA = 2,
        FULL = 3
    };

    const size_t _symbol_size;
    const uint16_t _symbols;
    Enc_State _state;
    Raw_Encoder<Rnd_It, Fwd_It, without_interleaver> encoder;
    DenseMtx precomputed;
    Rnd_It _from, _to;
    // avoid launching multiple computations for the encoder.
    // it is guaranteed to succeed anyway.
    std::mutex _mtx;
    std::shared_future<Error> _single_wait;
    std::thread _waiting;

    static void compute_thread (Encoder<Rnd_It, Fwd_It> *obj,
                                                    bool forced_precomputation,
                                                    std::promise<Error> p);
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_LOCAL Decoder
{
public:
    using Report = Dec_Report;

    ~Decoder();
    Decoder (const Block_Size symbols, const size_t symbol_size,
                                                        const Dec_Report type);
    Decoder (const Decoder&) = delete;
    Decoder& operator= (const Decoder&) = delete;
    Decoder (Decoder &&) = delete;
    Decoder& operator= (Decoder &&) = delete;

    explicit operator bool() const;

    uint16_t symbols() const;
    size_t symbol_size() const;

#ifdef RQ_HEADER_ONLY
    RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> begin();
    RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> end();
#endif

    Error add_symbol (In_It &from, const In_It to, const uint32_t esi);
    std::vector<bool> end_of_input (const Fill_With_Zeros fill);

    bool can_decode() const;
    bool ready() const;
    void stop();
    void clear_data();
    uint16_t needed_symbols() const;

    void set_max_concurrency (const uint16_t max_threads);
    Decoder_Result decode_once();

    struct Decoder_wait_res poll();
    struct Decoder_wait_res wait_sync();
    std::future<struct Decoder_wait_res> wait();


    Error decode_symbol (Fwd_It &start, const Fwd_It end, const uint16_t esi);
    // return number of bytes written
    struct Decoder_written decode_bytes (Fwd_It &start, const Fwd_It end,
                                    const size_t from_byte, const size_t skip);
private:
    uint16_t _max_threads;
    const uint16_t _symbols;
    const size_t _symbol_size;
    std::atomic<uint32_t> last_reported;
    const Dec_Report _type;
    RaptorQ__v1::Work_State work;
    Raw_Decoder<In_It> dec;
    // 2* symbols. Actually tracks available and reported symbols.
    // each symbol gets 2 bool: 1= available, 2=reported
    std::deque<std::atomic<bool>> symbols_tracker;
    std::mutex _mtx;
    std::condition_variable _cond;
    std::vector<std::thread> waiting;

    static void waiting_thread (Decoder<In_It, Fwd_It> *obj,
                                    std::promise<struct Decoder_wait_res> p);
};


///////////////////
//// Encoder
///////////////////

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::~Encoder()
{
    encoder.stop();
    if (_waiting.joinable())
        _waiting.join();
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::Encoder (const Block_Size symbols,
                                                    const size_t symbol_size)
    : _symbol_size (symbol_size), _symbols (static_cast<uint16_t> (symbols)),
                                                encoder (symbols, _symbol_size)
{
    IS_RANDOM(Rnd_It, "RaptorQ__v1::Encoder");
    IS_FORWARD(Fwd_It, "RaptorQ__v1::Encoder");
    // check for proper initialization
    uint16_t idx;
    for (idx = 0; idx < (*blocks).size(); ++idx) {
        if ((*blocks)[idx] == symbols)
            break;
    }
    // check that the user did not try some cast trickery,
    // and maximum size is ssize_t::max. But ssize_t is not standard,
    // so we search the maximum ourselves.
    if (idx == (*blocks).size() || symbol_size >= std::pow (2,
                                    (sizeof(size_t) == 4 ? 31 : 63))) {
        _state = Enc_State::INIT_ERROR;
    }
    _state = Enc_State::NEED_DATA;
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::operator bool() const
    { return _state != Enc_State::INIT_ERROR; }

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols() const
{
    if (_state == Enc_State::INIT_ERROR)
        return 0;
    return _symbols;
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
{
    if (_state == Enc_State::INIT_ERROR)
        return 0;
    return _symbol_size;
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair() const
{
    // you can have up to 56403 symbols in a block
    // rfc6330 limits you to 992173 repair symbols
    // but limits are meant to be broken!
    // the limit sould be up to 4294967279 repair symbols 2^32-(_param.S + H)
    // but people might misuse the API, and call end_repair(max_repair),
    // which would overflow.
    // We are sorry for taking away from you that 0.0014% of repair symbols.
    if (_state == Enc_State::INIT_ERROR)
        return 0;
    auto _param = Parameters (_symbols);
    return static_cast<uint32_t> (std::numeric_limits<uint32_t>::max() -
                                                                    _param.L);
}

#ifdef RQ_HEADER_ONLY
template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
                                        Encoder<Rnd_It, Fwd_It>::begin_source()
{
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (this, 0);
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
                                       Encoder<Rnd_It, Fwd_It>::end_source()
{
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (this,
                                                                    _symbols);
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
                                        Encoder<Rnd_It, Fwd_It>::begin_repair()
    { return end_source(); }

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
                    Encoder<Rnd_It, Fwd_It>::end_repair (const uint32_t repair)
{
    return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (nullptr,
                                                            _symbols + repair);
}
#endif

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::has_data() const
{
    if (_state == Enc_State::INIT_ERROR)
        return false;
    return _state == Enc_State::FULL;
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::set_data (const Rnd_It &from, const Rnd_It &to)
{
    if (_state == Enc_State::INIT_ERROR)
        return 0;
    _from = from;
    _to = to;
    encoder.set_data (&_from, &_to);
    _state = Enc_State::FULL;
    return static_cast<size_t>(_to - _from) *
                    sizeof(typename std::iterator_traits<Rnd_It>::value_type);
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::clear_data()
{
    if (_state == Enc_State::INIT_ERROR)
        return;
    std::lock_guard<std::mutex> lock (_mtx);
    RQ_UNUSED (lock);
    if (_waiting.joinable()) {
        encoder.stop();
        _waiting.join();
    }
    _single_wait = std::shared_future<Error>();
    _state = Enc_State::NEED_DATA;
    encoder.clear_data();
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::ready() const
{
    if (_state == Enc_State::INIT_ERROR)
        return false;
    return encoder.ready();
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::stop()
    { encoder.stop(); }

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::precompute_sync()
{
    if (_state == Enc_State::INIT_ERROR)
        return false;
    if (_single_wait.valid()) {
        _single_wait.wait();
        return true;
    }
    std::unique_lock<std::mutex> lock (_mtx);
    if (_single_wait.valid() &&  _single_wait.get() == Error::NONE)
        return true;
    stop();
    if (_waiting.joinable())
        _waiting.join();
    std::promise<Error> p;
    _single_wait = p.get_future().share();
    lock.unlock();
    compute_thread (this, true, std::move(p));
    return true;
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::compute_sync()
{
    if (_state == Enc_State::INIT_ERROR)
        return false;
    if (_single_wait.valid())
        _single_wait.wait();
    std::unique_lock<std::mutex> lock (_mtx);
    if (_single_wait.valid() && _single_wait.get() == Error::NONE)
        return true;
    stop();
    if (_waiting.joinable())
        _waiting.join();
    std::promise<Error> p;
    _single_wait = p.get_future().share();
    lock.unlock();
    compute_thread (this, false, std::move(p));
    return true;
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::compute_thread (
                        Encoder<Rnd_It, Fwd_It> *obj, bool force_precomputation,
                                                        std::promise<Error> p)
{
    static RaptorQ__v1::Work_State work = RaptorQ__v1::Work_State::KEEP_WORKING;

    if (force_precomputation) {
        if (obj->precomputed.rows() == 0)
            obj->precomputed = obj->encoder.get_precomputed (&work);
        if (obj->precomputed.rows() == 0) {
            // encoder always works. only possible reason:
            p.set_value (Error::EXITING);
            return;
        }
        // if we finished getting data by the time the computation
        // finished, update it all.
        if (obj->_state == Enc_State::FULL && !obj->encoder.ready())
            obj->encoder.generate_symbols (obj->precomputed,
                                                    &obj->_from, &obj->_to);
        p.set_value (Error::NONE);
    } else {
        if (obj->encoder.ready()) {
            p.set_value (Error::NONE);
            return;
        }
        if (obj->_state == Enc_State::FULL) {
            if (obj->encoder.generate_symbols (&work, &obj->_from, &obj->_to)) {
                p.set_value (Error::NONE);
                return;
            } else {
                // only possible reason:
                p.set_value (Error::EXITING);
                return;
            }
        } else {
            if (obj->precomputed.rows() == 0) {
                obj->precomputed = obj->encoder.get_precomputed (&work);
                if (obj->precomputed.rows() == 0) {
                    // only possible reason:
                    p.set_value (Error::EXITING);
                    return;
                }
            }
            if (obj->_state == Enc_State::FULL) {
                // if we finished getting data by the time the computation
                // finished, update it all.
                obj->encoder.generate_symbols (obj->precomputed,
                                                    &obj->_from, &obj->_to);
            }
            p.set_value (Error::NONE);
            return;
        }
    }
}

template <typename Rnd_It, typename Fwd_It>
std::shared_future<Error> Encoder<Rnd_It, Fwd_It>::precompute()
{
    if (_state == Enc_State::INIT_ERROR) {
        std::promise<Error> p;
        p.set_value (Error::INITIALIZATION);
        return p.get_future().share();
    }
    std::unique_lock<std::mutex> lock (_mtx);
    if (_single_wait.valid())
        return _single_wait;
    stop();
    if (_waiting.joinable())
        _waiting.join();
    std::promise<Error> p;
    _single_wait = p.get_future().share();
    _waiting = std::thread (compute_thread, this, true, std::move(p));
    lock.unlock();
    return _single_wait;
}

template <typename Rnd_It, typename Fwd_It>
std::shared_future<Error> Encoder<Rnd_It, Fwd_It>::compute()
{
    std::promise<Error> p;
    if (_state == Enc_State::INIT_ERROR) {
        p.set_value (Error::INITIALIZATION);
        return p.get_future().share();
    }
    std::unique_lock<std::mutex> lock (_mtx);
    if (_single_wait.valid())
        return _single_wait;
    stop();
    if (_waiting.joinable())
        _waiting.join();
    _single_wait = p.get_future().share();
    _waiting = std::thread (compute_thread, this, false, std::move(p));
    lock.unlock();
    return _single_wait;
}

template <typename Rnd_It, typename Fwd_It>
size_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
                                                            const uint32_t id)
{
    if (_state == Enc_State::INIT_ERROR)
        return 0;
    // returns number of iterators written
    if (_state == Enc_State::FULL) {
        if (id >= _symbols) { // repair symbol
            if (!encoder.ready()) {
                if (!_single_wait.valid())
                    _single_wait = compute();
                _single_wait.wait();
            }
        }
        return encoder.Enc (id, output, end);
    }
    return 0;
}

///////////////////
//// Decoder
///////////////////

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::~Decoder ()
{
    work = RaptorQ__v1::Work_State::ABORT_COMPUTATION;
    std::unique_lock<std::mutex> lock (_mtx);
    _cond.notify_all();
    lock.unlock();
    // wait threads to exit
    do {
        lock.lock();
        if (waiting.size() == 0) {
            lock.unlock();
            break;
        }
        _cond.wait (lock);
        lock.unlock();
    } while (waiting.size() != 0);
}

template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::Decoder (const Block_Size symbols,
                                const size_t symbol_size, const Dec_Report type)
    :_symbols (static_cast<uint16_t> (symbols)), _symbol_size (symbol_size),
                                    _type (type), dec (symbols, symbol_size)
{
    IS_INPUT(In_It, "RaptorQ__v1::Decoder");
    IS_FORWARD(Fwd_It, "RaptorQ__v1::Decoder");
    // check for proper initialization
    uint16_t idx;
    for (idx = 0; idx < (*blocks).size(); ++idx) {
        if ((*blocks)[idx] == symbols)
            break;
    }
    // check that the user did not try some cast trickery,
    // and maximum size is ssize_t::max.
    if (idx == (*blocks).size() || symbol_size >=
                                        std::numeric_limits<ssize_t>::max()) {
        return;
    }
    if (type != Dec_Report::PARTIAL_FROM_BEGINNING &&
            type != Dec_Report::PARTIAL_ANY &&
            type != Dec_Report::COMPLETE) {
        return; // no cast trickey plz
    }

    last_reported.store (0);
    symbols_tracker = std::deque<std::atomic<bool>> (2 * _symbols);
    for (idx = 0; idx < 2 * _symbols; ++idx)
        symbols_tracker[idx] = false;
    work = RaptorQ__v1::Work_State::KEEP_WORKING;
    _max_threads = 1;
}

template <typename Rnd_It, typename Fwd_It>
Decoder<Rnd_It, Fwd_It>::operator bool() const
    { return symbols_tracker.size() > 0; }

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbols() const
{
    if (symbols_tracker.size() == 0)
        return 0;
    return _symbols;
}

template <typename In_It, typename Fwd_It>
size_t Decoder<In_It, Fwd_It>::symbol_size() const
{
    if (symbols_tracker.size() == 0)
        return 0;
    return _symbol_size;
}

#ifdef RQ_HEADER_ONLY
template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It>
                                                Decoder<In_It, Fwd_It>::begin()
{
    return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (this, 0);
}

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It>
                                                Decoder<In_It, Fwd_It>::end()
{
    return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (nullptr,
                                                                _symbols);
}
#endif

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::needed_symbols() const
{
    if (symbols_tracker.size() == 0)
        return 0;
    return dec.needed_symbols();
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &from, const In_It to,
                                                            const uint32_t esi)
{
    if (symbols_tracker.size() == 0)
        return Error::INITIALIZATION;
    auto ret = dec.add_symbol (from, to, esi, false);
    if (ret == Error::NONE) {
        if (esi < _symbols)
            symbols_tracker [2 * esi].store (true);
        std::unique_lock<std::mutex> lock (_mtx);
        _cond.notify_all();
    }
    return ret;
}

template <typename In_It, typename Fwd_It>
Decoder_wait_res Decoder<In_It, Fwd_It>::poll ()
{
    if (symbols_tracker.size() == 0)
        return {Error::INITIALIZATION, 0};
    uint32_t idx;
    uint32_t last;
    bool expected = false;
    switch (_type) {
    case Dec_Report::PARTIAL_FROM_BEGINNING:
        // report the number of symbols that are known, starting from
        // the beginning.
        last = last_reported.load();
        idx = last;
        for (; idx < symbols_tracker.size(); idx += 2) {
            if (symbols_tracker[idx].load() == true) {
                ++idx;
                if (symbols_tracker[idx].load() == false)
                    symbols_tracker[idx].store (true);
                --idx;
            } else {
                break;
            }
        }
        idx /= 2;
        if (idx > last) {
            while (!last_reported.compare_exchange_weak (last, idx)) {
                // expected is now "last_reported.load()"
                if (last >= idx) {
                    // other thread already reported more than us.
                    // do not report things twice.
                    if (dec.ready()) {
                        last_reported.store (_symbols);
                        return {Error::NONE, _symbols};
                    }
                    if (dec.threads() > 0)
                        return {Error::WORKING, 0};
                    return {Error::NEED_DATA, 0};
                }
                // else we can report the new stuff
            }
            return {Error::NONE, static_cast<uint16_t>(idx)};
        }
        // nothing to report
        if (dec.ready()) {
            last_reported.store (_symbols);
            return {Error::NONE, _symbols};
        }
        if (dec.threads() > 0)
            return {Error::WORKING, 0};
        return {Error::NEED_DATA, 0};
    case Dec_Report::PARTIAL_ANY:
        // report the first available, not yet reported.
        // or return {NONE, _symbols} if all have been reported
        if (dec.ready())
            return {Error::NONE, _symbols};
        for (idx = 0; idx < static_cast<uint32_t> (symbols_tracker.size());
                                                                    idx += 2) {
            if (symbols_tracker[idx].load() == true) {
                ++idx;
                if (symbols_tracker[idx].load() == false) {
                    expected = false;
                    if (symbols_tracker[idx].
                                    compare_exchange_strong (expected, true)) {
                        return {Error::NONE, static_cast<uint16_t> (idx / 2)};
                    }   // else some other thread raced us, keep trying other
                        // symbols
                }
            }
        }
        if (dec.ready())
            return {Error::NONE, _symbols};
        if (dec.threads() > 0)
            return {Error::WORKING, 0};
        return {Error::NEED_DATA, 0};
    case Dec_Report::COMPLETE:
        auto init = last_reported.load();
        idx = init * 2;
        for (; idx < symbols_tracker.size(); idx += 2) {
            if (symbols_tracker[idx].load() == false) {
                idx /= 2;
                while (!last_reported.compare_exchange_weak(init, idx))
                    idx = std::max(init, idx);
                if (dec.threads() > 0)
                    return {Error::WORKING, 0};
                return {Error::NEED_DATA, 0};
            }
        }
        last_reported.store (_symbols);
        return {Error::NONE, 0};
    }
    return {Error::WORKING, 0};
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::waiting_thread (Decoder<In_It, Fwd_It> *obj,
                                        std::promise<struct Decoder_wait_res> p)
{
    bool promise_set = false;
    while (obj->work == RaptorQ__v1::Work_State::KEEP_WORKING) {
        bool compute = obj->dec.add_concurrent (obj->_max_threads);
        if (compute) {
            obj->decode_once();
            std::unique_lock<std::mutex> lock (obj->_mtx);
            obj->dec.drop_concurrent();
            obj->_cond.notify_all(); // notify other waiting threads
        }
        std::unique_lock<std::mutex> lock (obj->_mtx);
        // poll() does not actually need to be locked, but we use the
        // lock-wait mechanism to signal the arrival of new symbols,
        // so that we retry only when we get new data.
        auto res = obj->poll();
        if (res.error == Error::NONE || (obj->dec.end_of_input == true &&
                                            !obj->dec.can_decode()  &&
                                            obj->dec.threads() == 0 &&
                                                res.error == Error::NEED_DATA)){
            p.set_value (res);
            promise_set = true;
            break;
        }
        obj->_cond.wait (lock);
        lock.unlock();
    }

    if (obj->work != RaptorQ__v1::Work_State::KEEP_WORKING && !promise_set)
        p.set_value ({Error::EXITING, 0});

    std::unique_lock<std::mutex> lock (obj->_mtx);
    RQ_UNUSED (lock);
    for (auto th = obj->waiting.begin(); th != obj->waiting.end(); ++th) {
        if (std::this_thread::get_id() == th->get_id()) {
            th->detach();
            obj->waiting.erase (th);
            break;
        }
    }
    obj->_cond.notify_all(); // notify exit to destructor
}

template <typename In_It, typename Fwd_It>
Decoder_wait_res Decoder<In_It, Fwd_It>::wait_sync ()
{
    // FIXME: if used, then poll() can not return ERROR::WAITING
    if (symbols_tracker.size() == 0)
        return {Error::INITIALIZATION, 0};
    std::promise<struct Decoder_wait_res> p;
    auto fut = p.get_future();
    waiting_thread (this, std::move(p));
    fut.wait();
    return fut.get();
}

template <typename In_It, typename Fwd_It>
std::future<struct Decoder_wait_res> Decoder<In_It, Fwd_It>::wait ()
{
    std::promise<struct Decoder_wait_res> p;
    if (symbols_tracker.size() == 0) {
        p.set_value ({Error::INITIALIZATION, 0});
        return p.get_future();
    }
    auto f = p.get_future();
    std::unique_lock<std::mutex> lock (_mtx);
    RQ_UNUSED (lock);
    waiting.emplace_back (waiting_thread, this, std::move(p));
    return f;
}

template <typename In_It, typename Fwd_It>
std::vector<bool> Decoder<In_It, Fwd_It>::end_of_input (
                                                    const Fill_With_Zeros fill)
{
    if (symbols_tracker.size() != 0) {
        if (fill == Fill_With_Zeros::YES)
            return dec.fill_with_zeros();
        dec.end_of_input = true;
    }
    return std::vector<bool>();
}

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::can_decode() const
{
    if (symbols_tracker.size() == 0)
        return false;
    return dec.can_decode();
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::set_max_concurrency (const uint16_t max_threads)
{
    if (symbols_tracker.size() != 0)
        _max_threads = max_threads;
}

template <typename In_It, typename Fwd_It>
Decoder_Result Decoder<In_It, Fwd_It>::decode_once()
{
    if (symbols_tracker.size() == 0)
        return Decoder_Result::STOPPED;
    auto res = dec.decode (&work);
    if (res == Decoder_Result::DECODED) {
        std::unique_lock<std::mutex> lock (_mtx);
        RQ_UNUSED (lock);
        if (_type != Dec_Report::COMPLETE) {
            uint32_t id = last_reported.load();
            for (; id < symbols_tracker.size(); id += 2)
                symbols_tracker[id].store (true);
        }
        last_reported.store(_symbols);
        lock.unlock();
    }
    return res;
}

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::ready() const
{
    if (symbols_tracker.size() == 0)
        return false;
    return dec.ready();
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::stop()
{
    if (symbols_tracker.size() == 0)
        return;
    work = RaptorQ__v1::Work_State::ABORT_COMPUTATION;
    std::unique_lock<std::mutex> lock (_mtx);
    RQ_UNUSED (lock);
    _cond.notify_all();
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::clear_data()
{
    if (symbols_tracker.size() == 0)
        return;
    std::unique_lock<std::mutex> lock (_mtx);
    RQ_UNUSED (lock);
    dec.clear_data();
    last_reported.store(0);
    for (auto it = symbols_tracker.begin(); it != symbols_tracker.end(); ++it)
        *it = false;
}

template <typename In_It, typename Fwd_It>
Decoder_written Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start,
                                                        const Fwd_It end,
                                                        const size_t from_byte,
                                                        const size_t skip)
{
    using T = typename std::iterator_traits<Fwd_It>::value_type;

    if (symbols_tracker.size() == 0 || skip >= sizeof(T) || from_byte >=
                            static_cast<size_t> (_symbols * _symbol_size)) {
        return {0, 0};
    }

    auto decoded = dec.get_symbols();

    uint16_t esi = static_cast<uint16_t> (from_byte /
                                            static_cast<size_t> (_symbol_size));
    uint16_t byte = static_cast<uint16_t> (from_byte %
                                            static_cast<size_t> (_symbol_size));

    size_t offset_al = skip;
    T element = static_cast<T> (0);
    if (skip != 0) {
        uint8_t *p = reinterpret_cast<uint8_t *> (&*start);
        for (size_t keep = 0; keep < skip; ++keep) {
            element += static_cast<T> (*(p++)) << keep * 8;
        }
    }
    size_t written = 0;
    while (start != end && esi < _symbols && dec.has_symbol (esi)) {
        element += static_cast<T> (static_cast<uint8_t> ((*decoded)(esi, byte)))
                                                            << offset_al * 8;
        ++offset_al;
        if (offset_al == sizeof(T)) {
            *start = element;
            ++start;
            written += offset_al;
            offset_al = 0;
            element = static_cast<T> (0);
        }
        ++byte;
        if (byte == decoded->cols()) {
            byte = 0;
            ++esi;
        }
    }
    if (start != end && offset_al != 0) {
        // we have more stuff in "element", but not enough to fill
        // the iterator. Do not overwrite additional data of the iterator.
        uint8_t *out = reinterpret_cast<uint8_t *> (&*start);
        uint8_t *in = reinterpret_cast<uint8_t *> (&element);
        for (size_t idx = 0; idx < offset_al; ++idx, ++out, ++in)
            *out = *in;
        written += offset_al;
    }
    return {written, offset_al};
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::decode_symbol (Fwd_It &start, const Fwd_It end,
                                                            const uint16_t esi)
{
    if (symbols_tracker.size() == 0)
        return Error::INITIALIZATION;
    auto start_copy = start;
    size_t esi_byte = esi * _symbol_size;

    auto out = decode_bytes (start_copy, end, esi_byte, 0);
    if (out.written == _symbol_size) {
        start = start_copy;
        return Error::NONE;
    }
    return Error::NEED_DATA;
}

}   // namespace Impl
}   // namespace RaptorQ__v1
