/*
 * Copyright (c) 2015-2016, Luca Fulchir<luker@fenrirproject.org>,
 * All rights reserved.
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

// we can switch easily between header-only and linked version of the library
#if defined (TEST_HDR_ONLY)
    #include "../src/RaptorQ/RaptorQ_v1_hdr.hpp"
#else
    #include "../src/RaptorQ/RaptorQ_v1.hpp"
#endif
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdlib.h>
#include <vector>

// Demonstration of how to use the C++ interface
// it's pretty simple, we generate some input,
// then encode, drop some packets (source and repair)
// and finally decode everything.

namespace RaptorQ = RaptorQ__v1;


bool test_enc_output (uint8_t **p_in, const uint8_t *in_end, uint8_t *p_out,
                                                            size_t symbol_size);
bool test_enc_output (uint8_t **p_in, const uint8_t *in_end, uint8_t *p_out,
                                                            size_t symbol_size)
{
    for (size_t idx = 0; idx < symbol_size && *p_in < in_end; ++idx) {
        if (*((*p_in)++) != *(p_out++)) {
            (*p_in) += (symbol_size - idx);
            return false;
        }
    }
    return true;
}

// mysize is bytes.
template <typename in_enc_align, typename out_enc_align, typename out_dec_align>
bool decode (const uint32_t mysize, std::mt19937_64 &rnd, float drop_prob,
                                                        const uint8_t overhead);
template <typename in_enc_align, typename out_enc_align, typename out_dec_align>
bool decode (const uint32_t mysize, std::mt19937_64 &rnd, float drop_prob,
                                                        const uint8_t overhead)
{
    // define the alignment of the input and output data, for
    // decoder and encoder.
    // note that this is independent from the "mysize" argument,
    // which is always in bytes.
    // used as template arguments
    //typedef uint8_t         in_enc_align;
    //typedef uint16_t        out_enc_align;
    typedef out_enc_align   in_dec_align;
    //typedef uint32_t        out_dec_align;
    // NOTE:  out_enc_align is the same as in_dec_align so that we
    // can simulate data trnsmision just by passing along a vector, but
    // they do not need to be the same.

    // make sure the size is a multiple of its iterator. not much sense
    // otherwise.
    assert ((mysize % sizeof(in_enc_align)) == 0);

    std::vector<in_enc_align> myvec;

    // initialize vector with random data
    // fill remaining data (more than "mysize" bytes) with zeros
    // distr should be "uint8_t". But visual studio.
    std::uniform_int_distribution<int16_t> distr (0,
                                          std::numeric_limits<uint8_t>::max());
    myvec.reserve (static_cast<size_t> (
                std::ceil(static_cast<float> (mysize) / sizeof(in_enc_align))));
    in_enc_align tmp = 0;
    uint8_t shift = 0;
    for (uint32_t i = 0; i < mysize; ++i) {
        tmp += static_cast<in_enc_align> (distr(rnd)) << shift * 8;
        //tmp += static_cast<in_enc_align> (i) << shift * 8;
        ++shift;
        if (shift >= sizeof(in_enc_align)) {
            myvec.push_back (tmp);
            shift = 0;
            tmp = 0;
        }
    }
    if (shift != 0)
        myvec.push_back (tmp);
    // done initializing random data.
    uint8_t *test_encoder = reinterpret_cast<uint8_t*>(&myvec[0]);
    const uint8_t *test_encoder_end = test_encoder + mysize;



    // std::pair<symbol id (esi), symbol>
    std::vector<std::pair<uint32_t, std::vector<out_enc_align>>> encoded;

    // symbol size must be multiple of iterator.
    // symbol size max = uint16_t
    uint16_t max_symsize = static_cast<uint16_t>(
            std::min (static_cast<size_t> (myvec.size()),
                static_cast<size_t> (std::numeric_limits<uint16_t>::max()
                                                    / sizeof(in_enc_align))));
    std::uniform_int_distribution<uint16_t> sym_distr (1, max_symsize);
    const uint16_t in_aligned_symbol_size = sym_distr (rnd);
    uint16_t symbol_size = in_aligned_symbol_size * sizeof(in_enc_align);
    const uint16_t out_aligned_symbol_size = static_cast<uint16_t> (std::ceil (
                    static_cast<float> (symbol_size) / sizeof (out_enc_align)));

    // find the right enum value for the block size:
    auto symbols = (myvec.size() * sizeof(in_enc_align)) / symbol_size;
    if ((myvec.size() * sizeof(in_enc_align)) % symbol_size != 0)
        ++symbols;
    RaptorQ::Block_Size block = RaptorQ::Block_Size::Block_10;
    for (auto blk : *RaptorQ::blocks) {
        if (static_cast<uint16_t> (blk) >= symbols) {
            block = blk;
            break;
        }
    }

    RaptorQ::Encoder<in_enc_align*, out_enc_align*> enc (block, symbol_size);
    if (enc.set_data (myvec.begin().base(), myvec.end().base()) != mysize) {
        std::cout << "Could not give data to the encoder :(\n";
        return false;
    }
    uint16_t _symbols = enc.symbols();
    std::cout << "Size: " << mysize << " symbols: " <<
                                static_cast<uint32_t> (_symbols) <<
                                " symbol size: " <<
                                static_cast<int32_t>(enc.symbol_size()) << "\n";
    if (!enc.compute_sync()) {
        std::cout << "Enc-RaptorQ failure! really bad!\n";
        return false;
    }

    if (drop_prob > static_cast<float> (90.0))
        drop_prob = 90.0;   // this is still too high probably.


    {
        std::uniform_real_distribution<float> drop (0.0, 100.0);
        // start encoding
        int32_t repair = overhead;
        // Now get the source and repair symbols.
        // make sure that at the end we end with "block.symbols() + overhead"
        // symbols, so that decoding is possible
        for (auto sym_it = enc.begin_source(); sym_it != enc.end_source();
                                                                    ++sym_it) {
            float dropped = drop (rnd);
            if (dropped <= drop_prob) {
                // we dropped one source symbol, we need one more repair.
                ++repair;
                test_encoder += symbol_size;
                continue;
            }
            // create a place where to save our source symbol
            std::vector<out_enc_align> source_sym;
            source_sym.reserve (out_aligned_symbol_size);
            source_sym.insert (source_sym.begin(), out_aligned_symbol_size, 0);
            out_enc_align* from = source_sym.begin().base();
            // save the symbol
            auto written = (*sym_it) (from, source_sym.end().base());
            if (written != out_aligned_symbol_size) {
                std::cout << written << "-vs-" << out_aligned_symbol_size <<
                                    " Could not get the whole source symbol!\n";
                return false;
            }
            if (!test_enc_output (&test_encoder, test_encoder_end,
                            reinterpret_cast<uint8_t*> (&*source_sym.begin()),
                                                                symbol_size)) {
                std::cout << "Encoder produced unexpected result\n";
                return false;
            }
            // finally add it to the encoded vector
            encoded.emplace_back ((*sym_it).id(), std::move(source_sym));
        }
        // now get (overhead + source_symbol_lost) repair symbols.
        std::cout << "Source Packet lost: " << repair - overhead << "\n";
        auto sym_it = enc.begin_repair();
        for (; repair >= 0 && sym_it != enc.end_repair (enc.max_repair());
                                                                    ++sym_it) {
            // repair symbols can be lost, too!
            float dropped = drop (rnd);
            if (dropped <= drop_prob) {
                continue;
            }
            --repair;
            // create a place where to save our source symbol
            std::vector<out_enc_align> repair_sym;
            repair_sym.reserve (out_aligned_symbol_size);
            repair_sym.insert (repair_sym.begin(), out_aligned_symbol_size, 0);
            out_enc_align* from = repair_sym.begin().base();
            // save the repair symbol
            auto written = (*sym_it) (from, repair_sym.end().base());
            if (written != out_aligned_symbol_size) {
                std::cout << written << "-vs-" << out_aligned_symbol_size <<
                                    " Could not get the whole repair symbol!\n";
                return false;
            }
            // finally add it to the encoded vector
            encoded.emplace_back ((*sym_it).id(), std::move(repair_sym));
        }
        if (sym_it == enc.end_repair (enc.max_repair())) {
            // we dropped waaaay too many symbols! how much are you planning to
            // lose, again???
            std::cout << "Maybe losing " << drop_prob << "% is too much?\n";
            return false;
        }
    }
    // encoding done. now "encoded" is the vector with the trnasmitted data.
    // let's decode it

    using Decoder_type = RaptorQ::Decoder<in_dec_align*, out_dec_align*>;
    Decoder_type dec (block, symbol_size, Decoder_type::Report::COMPLETE);


    std::vector<out_dec_align> received;
    // mysize and the out_alignment might be different. be sure to have
    // enough elements in "received".
    // NOTE: this means that the last element might have additional data.
    size_t out_size = static_cast<size_t> (
                std::ceil(static_cast<float>(mysize) / sizeof(out_dec_align)));
    received.reserve (out_size);
    // make sure that there's enough place in "received" to get the
    // whole decoded data.
    for (uint32_t i = 0; i < out_size; ++i)
        received.push_back (static_cast<out_dec_align> (0));

    for (size_t i = 0; i < encoded.size(); ++i) {
        out_enc_align *from = encoded[i].second.begin().base();
        auto err = dec.add_symbol (from, encoded[i].second.end().base(),
                                                            encoded[i].first);
        if (err != RaptorQ::Error::NONE && err != RaptorQ::Error::NOT_NEEDED) {
            std::cout << "error adding?\n";
            abort();
        }
    }

    dec.end_of_input (RaptorQ::Fill_With_Zeros::NO);
    // optional: fill with zeros to get partial data
    // std::vector<bool> symbol_bitmask = dec.fill_with_zeros (
    //                                          RaptorQ::Fill_With_Zeros::YES);
    auto res = dec.wait_sync();
    if (res.error != RaptorQ::Error::NONE) {
        std::cout << "Couldn't decode.\n";
        return false;
    }

    out_dec_align *from = received.begin().base();
    // decode all blocks
    // you can actually call ".decode(...)" as many times
    // as you want. It will only start decoding once
    // it has enough data.
    auto decoded = dec.decode_bytes (from, received.end().base(), 0, 0);

    // NOTE: decoded.first might be > mysize.
    // This can happen since "misize" might not fit the whole
    // symbols * symbol_size space.
    // Also, the decoder has to fill the iterators: if "mysize" is not aligned
    // with the "received" alignment, the last element in "received"
    // will have additional data.
    if (decoded.written < mysize) {
        if (decoded.written == 0) {
            std::cout << "Couldn't decode, RaptorQ Algorithm failure. "
                                                            "Can't Retry.\n";
            return true;
        } else {
            std::cout << "Partial Decoding? This should not have happened: " <<
                                    decoded.written  << " vs " << mysize << "\n";
        }
        return false;
    } else {
        std::cout << "Decoded: " << mysize << "\n";
    }
    // byte-wise check: did we actually decode everything the right way?
    uint8_t *in, *out;
    in = reinterpret_cast<uint8_t *> (myvec.data());
    out = reinterpret_cast<uint8_t *> (received.data());
    for (uint64_t i = 0; i < mysize; ++i) {
        if (in[i] != out[i]) {
            std::cout << "FAILED, but we thought otherwise! " << mysize << " - "
                                            << drop_prob << " at " << i << "\n";
            return false;
        }
    }

    return true;
}

uint32_t rnd_size (std::mt19937_64 &rnd, uint8_t size);
uint32_t rnd_size (std::mt19937_64 &rnd, uint8_t size)
{
    std::uniform_int_distribution<uint32_t> distr(1, 100000);
    uint32_t ret;
    do {
        ret = distr (rnd);
    } while ((ret % size) != 0);
    return ret;
}

int main (void)
{
    // get a random number generator
    std::mt19937_64 rnd;
    std::ifstream rand("/dev/urandom");
    uint64_t seed = 0;
    rand.read (reinterpret_cast<char *> (&seed), sizeof(seed));
    rand.close ();
    rnd.seed (seed);

    RaptorQ::local_cache_size (5000000);

    // encode and decoder
    for (size_t i = 0; i < 1000; ++i) {
        std::cout << "08-08-08\n";
        bool ret = decode<uint8_t, uint8_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
#if defined (TEST_HDR_ONLY)
        std::cout << "08-08-16\n";
        ret = decode<uint8_t, uint8_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "08-08-32\n";
        ret = decode<uint8_t, uint8_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "08-16-08\n";
        ret = decode<uint8_t, uint16_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "08-16-16\n";
        ret = decode<uint8_t, uint16_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "08-16-32\n";
        ret = decode<uint8_t, uint16_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "08-32-08\n";
        ret = decode<uint8_t, uint32_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "08-32-16\n";
        ret = decode<uint8_t, uint32_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "08-32-32\n";
        ret = decode<uint8_t, uint32_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint8_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "16-08-08\n";
        ret = decode<uint16_t, uint8_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "16-08-16\n";
        ret = decode<uint16_t, uint8_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "16-08-32\n";
        ret = decode<uint16_t, uint8_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "16-16-08\n";
        ret = decode<uint16_t, uint16_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
#endif
        std::cout << "16-16-16\n";
        ret = decode<uint16_t, uint16_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
#if defined (TEST_HDR_ONLY)
        std::cout << "16-16-32\n";
        ret = decode<uint16_t, uint16_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "16-32-08\n";
        ret = decode<uint16_t, uint32_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "16-32-16\n";
        ret = decode<uint16_t, uint32_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "16-32-32\n";
        ret = decode<uint16_t, uint32_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint16_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "32-08-08\n";
        ret = decode<uint32_t, uint8_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "32-08-16\n";
        ret = decode<uint32_t, uint8_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "32-08-32\n";
        ret = decode<uint32_t, uint8_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "32-16-08\n";
        ret = decode<uint32_t, uint16_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "32-16-16\n";
        ret = decode<uint32_t, uint16_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "32-16-32\n";
        ret = decode<uint32_t, uint16_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "32-32-08\n";
        ret = decode<uint32_t, uint32_t, uint8_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
        std::cout << "32-32-16\n";
        ret = decode<uint32_t, uint32_t, uint16_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;
#endif
        std::cout << "32-32-32\n";
        ret = decode<uint32_t, uint32_t, uint32_t> (
                                rnd_size (rnd, sizeof(uint32_t)), rnd, 20.0, 4);
        if (!ret)
            return -1;

    }
    std::cout << "All tests succesfull!\n";
    return 0;
}
