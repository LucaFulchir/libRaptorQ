/*
 * Copyright (c) 2015-2018, Luca Fulchir<luker@fenrirproject.org>,
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

#include "../src/RaptorQ/RaptorQ_v1_hdr.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdlib.h>
#include <vector>

// Demonstration of how to use the C++ RAW interface
// it's pretty simple, we generate some input,
// then encode, drop some packets (source and repair)
// and finally decode everything.


// rename the main namespace for ease of use
namespace RaptorQ = RaptorQ__v1;


// mysize is bytes.
bool test_rq (const uint32_t mysize, std::mt19937_64 &rnd,
                                                        float drop_probability,
                                                        const uint8_t overhead);
// the "overhead" variable tells us how many symbols more than the
// minimum we will generate. RaptorQ can not always decode a block,
// but there is a small probability that it will fail.
// More overhead => less probability of failure
//  overhead 0 => 1% failures
//  overhead 1 => 0.01% failures
//  overhead 2 => 0.0001% failures
// etc... as you can see, it make little sense to work with more than 3-4
// overhead symbols, but at least one should be considered
bool test_rq (const uint32_t mysize, std::mt19937_64 &rnd,
                                                        float drop_probability,
                                                        const uint8_t overhead)
{
    // the actual input.
    std::vector<uint8_t> input;
    input.reserve (mysize);

    // initialize vector with random data
    // distr should be "uint8_t". But visual studio does not like it, so
    // we use uint16_t
    // generate a random distribution between all values of uint8_t
    std::uniform_int_distribution<int16_t> distr (0,
                                          std::numeric_limits<uint8_t>::max());

    // fill our input with random data
    for (size_t idx = 0; idx < mysize; ++idx) {
        input.push_back (static_cast<uint8_t> (distr(rnd)));
    }


    // the input will be divided in blocks of 4 bytes.
    // it's a bit low, but this is just an example.
    // NOTE: the symbol size must be a multiple of the container size.
    //  since sizeof(uint8_t) == 1 and 4 is a multiple of 1, we are safe.
    const uint16_t symbol_size = 4; // bytes


    // how many symbols do we need to encode all our input in a single block?
    auto min_symbols = (input.size() * sizeof(uint8_t)) / symbol_size;
    if ((input.size() * sizeof(uint8_t)) % symbol_size != 0)
        ++min_symbols;
    // convert "symbols" to a typesafe equivalent, RaptorQ::Block_Size
    // This is needed becouse not all numbers are valid block sizes, and this
    // helps you choose the right block size
    RaptorQ::Block_Size block = RaptorQ::Block_Size::Block_10;
    for (auto blk : *RaptorQ::blocks) {
        // RaptorQ::blocks is a pointer to an array, just scan it to find your
        // block.
        if (static_cast<uint16_t> (blk) >= min_symbols) {
            block = blk;
            break;
        }
    }


    // now initialize the encoder.
    // the input for the encoder is std::vector<uint8_t>
    // the output for the encoder is std::vector<uint8_t>
    // yes, you can have different types, but most of the time you will
    // want to work with uint8_t
    RaptorQ::Encoder<typename std::vector<uint8_t>::iterator,
                            typename std::vector<uint8_t>::iterator> enc (
                                                            block, symbol_size);

    // give the input to the encoder. the encoder answers with the size of what
    // it can use
    if (enc.set_data (input.begin(), input.end()) != mysize) {
        std::cout << "Could not give data to the encoder :(\n";
        return false;
    }
    // actual symbols. you could just use static_cast<uint16_t> (blok)
    // but this way you can actually query the encoder.
    uint16_t _symbols = enc.symbols();
    // print some stuff in output
    std::cout << "Size: " << mysize << " symbols: " <<
                                static_cast<uint32_t> (_symbols) <<
                                " symbol size: " <<
                                static_cast<int32_t>(enc.symbol_size()) << "\n";

    // RQ need to do its magic on the input before you can ask the symbols.
    // multiple ways to do this are available.
    // The simplest is to run the computation and block everything until
    // the work has been done. Not a problem for small sizes (<200),
    // but big sizes will take **a lot** of time, consider running this with the
    // asynchronous calls
    if (!enc.compute_sync()) {
        // if this happens it's a bug in the library.
        // the **Decoder** can fail, but the **Encoder** can never fail.
        std::cout << "Enc-RaptorQ failure! really bad!\n";
        return false;
    }


    // the probability that a symbol will be dropped.
    if (drop_probability > static_cast<float> (90.0))
        drop_probability = 90.0;   // this is still too high probably.


    // we will store here all encoded and transmitted symbols
    // std::pair<symbol id (esi), symbol data>
    using symbol_id = uint32_t; // just a better name
    std::vector<std::pair<symbol_id, std::vector<uint8_t>>> received;
    {
        // in this block we will generate the symbols that will be sent to
        // the decoder.
        // a block of size X will need at least X symbols to be decoded.
        // we will randomly drop some symbols, but we will keep generating
        // repari symbols until we have the required number of symbols.

        std::uniform_real_distribution<float> drop_rnd (0.0, 100.0);
        uint32_t received_tot = 0;

        // Now get the source symbols.
        // source symbols are specials because they contain the input data
        // as-is, so if you get all of these, you don't need repair symbols
        // to make sure that we are using the decoder, drop the first
        // source symbol.
        auto source_sym_it = enc.begin_source();
        ++source_sym_it; // ignore the first soure symbol (=> drop it)
        source_sym_it++;
        for (; source_sym_it != enc.end_source(); ++source_sym_it) {
            // we save the symbol here:
            // make sure the vector has enough space for the symbol:
            // fill it with zeros for the size of the symbol
            std::vector<uint8_t> source_sym_data (symbol_size, 0);

            // save the data of the symbol into our vector
            auto it = source_sym_data.begin();
            auto written = (*source_sym_it) (it, source_sym_data.end());
            if (written != symbol_size) {
                // this can only happen if "source_sym_data" did not have
                // enough space for a symbol (here: never)
                std::cout << written << "-vs-" << symbol_size <<
                                    " Could not get the whole source symbol!\n";
                return false;
            }

            // can we keep this symbol or do we randomly drop it?
            float dropped = drop_rnd (rnd);
            if (dropped <= drop_probability) {
                continue; // start the cycle again
            }

            // good, the symbol was received.
            ++received_tot;
            // add it to the vector of received symbols
            symbol_id tmp_id = (*source_sym_it).id();
            received.emplace_back (tmp_id, std::move(source_sym_data));
        }

        std::cout << "Source Packet lost: " << enc.symbols() - received.size()
                                                                        << "\n";


        //--------------------------------------------
        // we finished working with the source symbols.
        // now we need to transmit the repair symbols.
        auto repair_sym_it = enc.begin_repair();
        auto max_repair = enc.max_repair(); // RaptorQ can theoretically handle
                                            // infinite repair symbols
                                            // but computers are not so infinite

        // we need to have at least enc.symbols() + overhead symbols.
        for (; received.size() < (enc.symbols() + overhead) &&
                                repair_sym_it != enc.end_repair (max_repair);
                                                            ++repair_sym_it) {
            // we save the symbol here:
            // make sure the vector has enough space for the symbol:
            // fill it with zeros for the size of the symbol
            std::vector<uint8_t> repair_sym_data (symbol_size, 0);

            // save the data of the symbol into our vector
            auto it = repair_sym_data.begin();
            auto written = (*repair_sym_it) (it, repair_sym_data.end());
            if (written != symbol_size) {
                // this can only happen if "repair_sym_data" did not have
                // enough space for a symbol (here: never)
                std::cout << written << "-vs-" << symbol_size <<
                                    " Could not get the whole repair symbol!\n";
                return false;
            }

            // can we keep this symbol or do we randomly drop it?
            float dropped = drop_rnd (rnd);
            if (dropped <= drop_probability) {
                continue; // start the cycle again
            }

            // good, the symbol was received.
            ++received_tot;
            // add it to the vector of received symbols
            symbol_id tmp_id = (*repair_sym_it).id();
            received.emplace_back (tmp_id, std::move(repair_sym_data));

        }
        if (repair_sym_it == enc.end_repair (enc.max_repair())) {
            // we dropped waaaay too many symbols!
            // should never happen in real life. it means that we do not
            // have enough repair symbols.
            // at this point you can actually start to retransmit the
            // repair symbols from enc.begin_repair(), but we don't care in
            // this example
            std::cout << "Maybe losing " << drop_probability << "% is too much?\n";
            return false;
        }
    }

    // Now we all the source and repair symbols are in "received".
    // we will use those to start decoding:


    // define "Decoder_type" to write less afterwards
    using Decoder_type = RaptorQ::Decoder<
                                    typename std::vector<uint8_t>::iterator,
                                    typename std::vector<uint8_t>::iterator>;
    Decoder_type dec (block, symbol_size, Decoder_type::Report::COMPLETE);
    // "Decoder_type::Report::COMPLETE" means that the decoder will not
    // give us any output until we have decoded all the data.
    // there are modes to extract the data symbol by symbol in an ordered
    // an unordered fashion, but let's keep this simple.


    // we will store the output of the decoder here:
    // note: the output need to have at least "mysize" bytes, and
    // we fill it with zeros
    std::vector<uint8_t> output (mysize, 0);

    // now push every received symbol into the decoder
    for (auto &rec_sym : received) {
        // as a reminder:
        //  rec_sym.first = symbol_id (uint32_t)
        //  rec_sym.second = std::vector<uint8_t> symbol_data
        symbol_id tmp_id = rec_sym.first;
        auto it = rec_sym.second.begin();
        auto err = dec.add_symbol (it, rec_sym.second.end(), tmp_id);
        if (err != RaptorQ::Error::NONE && err != RaptorQ::Error::NOT_NEEDED) {
            // When you add a symbol, you can get:
            //   NONE: no error
            //   NOT_NEEDED: libRaptorQ ignored it because everything is
            //              already decoded
            //   INITIALIZATION: wrong parameters to the decoder contructor
            //   WRONG_INPUT: not enough data on the symbol?
            //   some_other_error: errors in the library
            std::cout << "error adding?\n";
            return false;
        }
    }

    // by now we now there will be no more input, so we tell this to the
    // decoder. You can skip this call, but if the decoder does not have
    // enough data it sill wait forever (or until you call .stop())
    dec.end_of_input (RaptorQ::Fill_With_Zeros::NO);
    // optional if you want partial decoding without using the repair
    // symbols
    // std::vector<bool> symbols_bitmask = dec.end_of_input (
    //                                          RaptorQ::Fill_With_Zeros::YES);

    // decode, and do not return until the computation is finished.
    auto res = dec.wait_sync();
    if (res.error != RaptorQ::Error::NONE) {
        std::cout << "Couldn't decode.\n";
        return false;
    }

    // now save the decoded data in our output
    size_t decode_from_byte = 0;
    size_t skip_bytes_at_begining_of_output = 0;
    auto out_it = output.begin();
    auto decoded = dec.decode_bytes (out_it, output.end(), decode_from_byte,
                                            skip_bytes_at_begining_of_output);
    // "decode_from_byte" can be used to have only a part of the output.
    // it can be used in advanced setups where you ask only a part
    // of the block at a time.
    // "skip_bytes_at_begining_of_output" is used when dealing with containers
    // which size does not align with the output. For really advanced usage only
    // Both should be zero for most setups.


    if (decoded.written != mysize) {
        if (decoded.written == 0) {
            // we were really unlucky and the RQ algorithm needed
            // more symbols!
            std::cout << "Couldn't decode, RaptorQ Algorithm failure. "
                                                            "Can't Retry.\n";
        } else {
            // probably a library error
            std::cout << "Partial Decoding? This should not have happened: " <<
                                    decoded.written  << " vs " << mysize << "\n";
        }
        return false;
    } else {
        std::cout << "Decoded: " << mysize << "\n";
    }

    // byte-wise check: did we actually decode everything the right way?
    for (uint64_t i = 0; i < mysize; ++i) {
        if (input[i] != output[i]) {
            // this is a bug in the library, please report
            std::cout << "The output does not correspond to the input!\n";
            return false;
        }
    }

    return true;
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

    // keep some computation in memory. If you use only one block size it
    // will make things faster on bigger block size.
    // allocate 5Mb
    RaptorQ__v1::local_cache_size (5000000);

    // for our test, we use an input of random size, between 100 and 10.000
    // bytes.
    std::uniform_int_distribution<uint32_t> distr(100, 10000);
    uint32_t input_size = distr (rnd);

    if (!test_rq (input_size, rnd, 20.0, 4))
        return -1;
    std::cout << "The example completed successfully\n";
    return 0;
}
