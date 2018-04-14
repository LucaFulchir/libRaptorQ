/*
 * Copyright (c) 2015-2018, Luca Fulchir<luker@fulchir.it>, All rights reserved.
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

#if defined (TEST_HDR_ONLY)
    #include "../src/RaptorQ/RFC6330_v1_hdr.hpp"
#else
    #include "../src/RaptorQ/RFC6330_v1.hpp"
#endif
#include <fstream>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <vector>

// Demonstration of how to use the C++ interface
// it's pretty simple, we generate some input,
// then encode, drop some packets (source and repair)
// and finally decode everything.

namespace RFC6330 = RFC6330__v1;
namespace RaptorQ = RaptorQ__v1;

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
    std::uniform_int_distribution<uint8_t> distr(0, 0xFF);
    myvec.reserve (static_cast<size_t> (
                std::ceil(static_cast<float> (mysize) / sizeof(in_enc_align))));
    in_enc_align tmp = 0;
    uint8_t shift = 0;
    uint8_t data_counter;
    data_counter = 0;
    for (uint32_t i = 0; i < mysize; ++i) {
        // insert predicatable data: for debugging
        //tmp += static_cast<in_enc_align> (insert++) << shift * 8;
        // insert random data
        tmp += static_cast<in_enc_align> (distr (rnd)) << shift * 8;
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



    // std::pair<symbol id (esi), symbol>
    std::vector<std::pair<uint32_t, std::vector<out_enc_align>>> encoded;

    // symbol and sub-symbol sizes
    // sub symbol must be multiple of alignment,
    // symbol must be multiple of subsymbol
    std::uniform_int_distribution<uint16_t> sub_sym_distr (1, 16);
    const uint16_t subsymbol = sizeof(in_enc_align) * sub_sym_distr(rnd);
    std::uniform_int_distribution<uint16_t> sym_distr (1, 100);
    const uint16_t symbol_size = subsymbol * sym_distr (rnd);
    std::cout << "Subsymbol: " << subsymbol << " Symbol: " << symbol_size<<"\n";
    size_t aligned_symbol_size = static_cast<size_t> (
        std::ceil(static_cast<float> (symbol_size) / sizeof(out_enc_align)));
    auto enc_it = myvec.begin().base();

    std::uniform_int_distribution<uint32_t> mem_distr (10, 20000);
    RFC6330::Encoder<in_enc_align*, out_enc_align*> enc (
            enc_it, myvec.end().base(), subsymbol, symbol_size, mem_distr(rnd));
    std::cout << "Size: " << mysize << " Blocks: " <<
                                    static_cast<int32_t>(enc.blocks()) << "\n";
    if (!enc) {
        std::cout << "Coud not initialize encoder.\n";
        return false;
    }

    enc.compute (RFC6330::Compute::COMPLETE | RFC6330::Compute::NO_BACKGROUND);

    if (drop_prob > static_cast<float> (90.0))
        drop_prob = 90.0;   // this is still too high probably.
    std::uniform_real_distribution<float> drop (0.0, 100.0);

    int32_t repair;

    // start encoding
    int32_t blockid = -1;
    for (auto block : enc) {
        repair = overhead;
        ++blockid;
        std::cout << "Block " << blockid << " with " << block.symbols() <<
                                            " symbols. Real block size:" <<
                    static_cast<uint16_t> (block.extended_symbols()) << "\n";
        // Now get the source and repair symbols.
        // make sure that at the end we end with "block.symbols() + overhead"
        // symbols, so that decoding is possible
        for (auto sym_it = block.begin_source(); sym_it != block.end_source();
                                                                    ++sym_it) {
            float dropped = drop (rnd);
            if (dropped <= drop_prob) {
                // we dropped one source symbol, we need one more repair.
                ++repair;
                continue;
            }
            // create a place where to save our source symbol
            std::vector<out_enc_align> source_sym;
            source_sym.reserve (aligned_symbol_size);
            source_sym.insert (source_sym.begin(), aligned_symbol_size, 0);
            auto it = source_sym.begin().base();
            // save the symbol
            auto written = (*sym_it) (it, source_sym.end().base());
            if (written != aligned_symbol_size) {
                std::cout << written << "-vs-" << aligned_symbol_size <<
                                    " Could not get the whole source symbol!\n";
                return false;
            }
            // finally add it to the encoded vector
            encoded.emplace_back ((*sym_it).id(), std::move(source_sym));
        }
        // now get (overhead + source_symbol_lost) repair symbols.
        std::cout << "Source Packet lost: " << repair - overhead << "\n";
        auto sym_it = block.begin_repair();
        for (; repair >= 0 && sym_it != block.end_repair (block.max_repair());
                                                                    ++sym_it) {
            // repair symbols can be lost, too!
            float dropped = drop (rnd);
            if (dropped <= drop_prob) {
                continue;
            }
            --repair;
            // create a place where to save our source symbol
            std::vector<out_enc_align> repair_sym;
            repair_sym.reserve (aligned_symbol_size);
            repair_sym.insert (repair_sym.begin(), aligned_symbol_size, 0);
            auto it = repair_sym.begin().base();
            // save the repair symbol
            auto written = (*sym_it) (it, repair_sym.end().base());
            if (written != aligned_symbol_size) {
                std::cout << written << "-vs-" << aligned_symbol_size <<
                                    " Could not get the whole repair symbol!\n";
                return false;
            }
            // finally add it to the encoded vector
            encoded.emplace_back ((*sym_it).id(), std::move(repair_sym));
        }
        if (sym_it == block.end_repair (block.max_repair())) {
            // we dropped waaaay too many symbols! how much are you planning to
            // lose, again???
            std::cout << "Maybe losing " << drop_prob << "% is too much?\n";
            return false;
        }
    }
    auto oti_scheme = enc.OTI_Scheme_Specific();
    auto oti_common = enc.OTI_Common();

    // encoding done. now "encoded" is the vector with the trnasmitted data.
    // let's decode it

    RFC6330::Decoder<in_dec_align*, out_dec_align*>
                                                dec (oti_common, oti_scheme);
    if (!dec) {
        std::cout << "Could not initialize decoder\n";
        abort();
    }

    auto async_dec = dec.compute (RFC6330::Compute::COMPLETE);

    std::vector<out_dec_align> received;
    size_t out_size = mysize / sizeof(out_dec_align);
    // the input of the encoder might have a completely different alignment
    // then the output. keep that into consideration when confronting
    // "decoded" and "mysize"
    size_t aligned_missing = mysize % sizeof(out_dec_align);
    if (aligned_missing != 0)
        ++out_size;
    // make sure that there's enough place in "received" to get the
    // whole decoded data.
    received.resize (out_size, 0);

    for (size_t i = 0; i < encoded.size(); ++i) {
        auto it = encoded[i].second.begin().base();
        auto err = dec.add_symbol (it, encoded[i].second.end().base(),
                                                            encoded[i].first);
        if (err != RFC6330::Error::NONE && err != RFC6330::Error::NOT_NEEDED) {
            std::cout << "error adding?\n";
            abort();
        }
    }

    dec.end_of_input (RFC6330::Fill_With_Zeros::NO);

    async_dec.wait();

    auto re_it = received.begin().base();
    // decode all blocks
    // you can actually call ".decode(...)" as many times
    // as you want. It will only start decoding once
    // it has enough data.
    auto decoded = dec.decode_bytes (re_it, received.end().base(), 0);

    assert (mysize == dec.bytes());
    // "decoded" can be more than mysize if the alignments are different
    if (decoded < mysize) {
        if (decoded == 0) {
            std::cout << "Couldn't decode, RaptorQ Algorithm failure. "
                                                            "Can't Retry.\n";
            return true;
        } else {
            std::cout << "Partial Decoding? This should not have happened: " <<
                                        decoded  << " vs " << mysize <<
                                        " missing " << aligned_missing << "\n";
        }
        return false;
    } else {
        std::cout << "Decoded: " << mysize << "\n";
    }
    // byte-wise check: did we actually decode everything the right way?
    uint8_t *in, *out;
    in = reinterpret_cast<uint8_t *> (myvec.data());
    out = reinterpret_cast<uint8_t *> (received.data());
    bool error = false;
    for (uint64_t i = 0; i < mysize; ++i) {
        if (in[i] != out[i]) {
//            std::cout << "FAILED, but we thought otherwise! " << mysize << " - "
//                                            << drop_prob << " at " << i << "\n";
            std::cout << i << " -> " << std::hex << static_cast<int>(in[i]) << " = " << static_cast<int> (out[i]) << std::dec << "\n";
            error = true;
        }
    }

    return !error;
}

uint32_t rnd_size (std::mt19937_64 &rnd, uint8_t size);
uint32_t rnd_size (std::mt19937_64 &rnd, uint8_t size)
{
    std::uniform_int_distribution<uint32_t> distr(1, 10000);
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

    RFC6330__v1::set_thread_pool (1, 1, RaptorQ__v1::Work_State::KEEP_WORKING);

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
