/*
 * Copyright (c) 2016-2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "../external/optionparser-1.4/optionparser.h"
#pragma clang diagnostic pop
#pragma GCC diagnostic pop
#include "RaptorQ/RaptorQ_v1_hdr.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <vector>


//////
/// Command line parsing stuff. see "optionparser" documentation
//////
struct Arg: public option::Arg {

static option::ArgStatus Unknown(const option::Option& option, bool msg)
{
    if (msg)
        std::cerr << "Unknown option '" << option.name << "'\n";
    return option::ARG_ILLEGAL;
}

static option::ArgStatus Numeric (const option::Option& option, bool msg)
{
    char* endptr = nullptr;
    int64_t res = -1;
    if (option.arg != nullptr)
        res = strtol(option.arg, &endptr, 10);
    // NOTE: numeric arguments must be >= 0
    if (endptr != option.arg && *endptr == 0 && res >= 0)
        return option::ARG_OK;

    if (msg) {
        std::cerr << "ERR: Option '" << option.name <<
                                            "' requires a numeric argument\n";
    }
    return option::ARG_ILLEGAL;
}
};

enum  optionIndex { UNKNOWN, HELP, FORMAT, SYMBOLS, SYMBOL_SIZE, REPAIR, BYTES};
const option::Descriptor usage[] =
{
 {UNKNOWN, 0, "", "", Arg::Unknown, "USAGE: benchmark|blocks"},
 {UNKNOWN, 0, "", "", Arg::Unknown,
                    "USAGE: encode|decode PARAMETERS INPUT OUTPUT\n"
                                            "  use '-' for stdin/stdout\n\n"
                                                    "Standalone parameters:"},
 {HELP,    0, "h", "help", Arg::None, "  -h --help\tThis help."},
 {FORMAT,    0, "f", "format", Arg::None, "  -f --format\taddidional "
                                                    "information/examples"},
 {UNKNOWN, 0, "", "", Arg::Unknown, "ENCODE/DECODE parameters:"},
 {SYMBOLS, 0, "s", "symbols", Arg::Numeric, "  -s --symbols\t"
                                                "number of symbols per block"},
 {SYMBOL_SIZE, 0, "w", "symbol-size", Arg::Numeric, "  -w --symbol-size\t"
                                                        " bytes per symbol"},
 {UNKNOWN, 0, "", "", Arg::Unknown, "ENCODE only parameters:"},
 {REPAIR, 0, "r", "repair", Arg::Numeric, "  -r --repair\t"
                                        "number of repair symbols per block"},
 {UNKNOWN, 0, "", "", Arg::None, "DECODE only parameters:"},
 {BYTES, 0, "b", "bytes", Arg::Numeric, "  -b --bytes\t"
                                    "data size for each {en,de}coder block"},
 {0,0,nullptr,nullptr,nullptr,nullptr}
};

void bench (uint32_t seconds);
static void info (const char *prog_name);
static bool encode (const int64_t symbol_size,
                                        const RaptorQ__v1::Block_Size symbols,
                                        const uint32_t repair,
                                        std::istream *input,
                                        std::ostream *output);

static bool decode (const size_t bytes, const RaptorQ__v1::Block_Size symbols,
                                                    const int64_t symbol_size,
                                                    std::istream *input,
                                                    std::ostream *output);

static void info (const char *prog_name)
{
    std::cout << "RaptorQ library version: " << RaptorQ_version << "\n";
    std::cout << "\tThis program uses the libRaptorQ library encoder and "
                                                                    "decoder\n";
    std::cout << "\tExample:\n";
    std::cout << "\t\t" << prog_name << " encode -s 10 -w 8 -r 10 input "
                                                                    "output\n";
    std::cout << "\t\t" << prog_name << " decode -s 10 -w 8 -b 1024 input "
                                                                    "output\n";
    std:: cout << "\tUse \"-\" for stdin and stdout\n";
    std::cout << "\n";
    std::cout << "Encoder output format/Decoder input format:\n";
    std::cout << "\tuint32_t: block  number\n";
    std::cout << "\tuint32_t: symbol number\n";
    std::cout << "\tsymbol:   data\n";
}

// argument passing and data between input7output thread for the
// decoder
using iter_8 = std::vector<uint8_t>::iterator;
using Dec = RaptorQ__v1::Decoder<iter_8, iter_8>;

enum class Out_Status : uint8_t {
    WORKING,
    GRACEFUL_STOP,
    ERROR,
    EXITED,
};

struct write_out_args
{
    size_t bytes;
    std::map<size_t, std::unique_ptr<Dec>> *decoders;
    std::mutex *mtx;
    std::condition_variable *cond;
    std::ostream *output;
    Out_Status *status;
};


static void print_output (struct write_out_args args);

// thread function to wait for the decoders to finish decoding and
// print output. Only used when decoding
static void print_output (struct write_out_args args)
{
    size_t bytes_left = args.bytes;
    size_t current_block = 0;
    uint16_t last_symbol = 0;
    std::unique_lock<std::mutex> lock (*args.mtx, std::defer_lock);
    while (*args.status == Out_Status::WORKING ||
                                    *args.status == Out_Status::GRACEFUL_STOP) {
        lock.lock();
        auto dec_it = args.decoders->find (current_block);
        if (dec_it == args.decoders->end()) {
            if (*args.status == Out_Status::GRACEFUL_STOP) {
                lock.unlock();
                break;
            }
            args.cond->wait (lock);
            lock.unlock();
            continue;
        }
        auto dec = dec_it->second.get();
        if (!dec->can_decode()) {
            if (*args.status == Out_Status::GRACEFUL_STOP) {
                *args.status = Out_Status::ERROR;
                return;
            }
            args.cond->wait (lock);
            lock.unlock();
            continue;
        }
        lock.unlock();
        auto pair = dec->wait_sync();
        if (pair.error == RaptorQ__v1::Error::NEED_DATA) {
            if (*args.status == Out_Status::GRACEFUL_STOP) {
                lock.lock();
                if (dec->poll().error == RaptorQ__v1::Error::NONE)
                    continue;
                *args.status = Out_Status::ERROR;
                return;
            }
            continue;
        }
        if (pair.error != RaptorQ__v1::Error::NONE) {
            // internal error or interrupted computation
            lock.lock();
            *args.status = Out_Status::ERROR;
            return;
        }
        size_t sym_size = dec->symbol_size();
        std::vector<uint8_t> buffer (sym_size);;
        for (; last_symbol < pair.symbol; ++last_symbol) {
            buffer.clear();
            buffer.insert (buffer.begin(), sym_size, 0);
            auto buf_start = buffer.begin();
            auto to_write = dec->decode_symbol (buf_start, buffer.end(),
                                                                last_symbol);
            if (to_write != RaptorQ__v1::Error::NONE) {
                std::cerr << "ERR: partial or empty symbol from decoder\n";
                lock.lock();
                *args.status = Out_Status::ERROR;
                return;
            }
            size_t writes_left = std::min (bytes_left,
                                            static_cast<size_t> (sym_size));
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wshorten-64-to-32"
            args.output->write (reinterpret_cast<char *> (buffer.data()),
                                            static_cast<int64_t> (writes_left));
            #pragma clang diagnostic pop
            bytes_left -= writes_left;
            if (bytes_left == 0) {
                // early exit. we wrote everything.
                last_symbol = dec->symbols();
                break;
            }
        }
        if (last_symbol == dec->symbols()) {
            lock.lock();
            dec_it = args.decoders->find (current_block);
            dec_it->second.reset (nullptr);
            lock.unlock();
            ++current_block;
            last_symbol = 0;
        }
    }
    lock.lock();
    *args.status = Out_Status::EXITED;
}

static bool decode (const size_t bytes, const RaptorQ__v1::Block_Size symbols,
                                                    const int64_t symbol_size,
                                                    std::istream *input,
                                                    std::ostream *output)
{
    // false on error
    std::vector<uint8_t> buf (static_cast<size_t> (symbol_size));
    std::map<size_t, std::unique_ptr<Dec>> decoders;
    size_t bytes_left = bytes;
    size_t last_block_bytes = 0;
    uint32_t block_number;
    uint32_t symbol_number;
    std::mutex mtx;
    std::condition_variable cond;
    Out_Status thread_status = Out_Status::WORKING;
    struct write_out_args args;
    args.bytes = bytes;
    args.decoders = &decoders;
    args.mtx = &mtx;
    args.cond = &cond;
    args.output = output;
    args.status = &thread_status;
    std::thread write_out (print_output, args);

    std::unique_lock<std::mutex> lock (mtx, std::defer_lock);
    while (true) {
        input->read (reinterpret_cast<char *> (&block_number),
                                                    sizeof(block_number));
        int64_t read = input->gcount();
        if (read > 0 && read != sizeof(block_number)) {
            std::cerr << "ERR: not enough data to fill block number\n";
            thread_status = Out_Status::ERROR;
            return false;
        } else if (read == 0 || input->eof()) {
            if (bytes_left == 0 && last_block_bytes !=
                                static_cast<uint64_t> (
                                static_cast<int64_t> (symbols) * symbol_size)) {
                // the last block was not filled to the end.
                // we need to pad it with additional symbols.
                lock.lock();
                auto dec_it = decoders.rbegin();
                if (dec_it == decoders.rend()) {
                    // no decoder found? we needed to pad it. err.
                    thread_status = Out_Status::ERROR;
                    lock.unlock();
                    cond.notify_one();
                    write_out.join();
                    std::cerr << "ERR: could not pad the last block\n";
                    return false;
                }
                auto dec = dec_it->second.get();
                if (dec != nullptr) {
                    buf.clear();
                    buf.insert (buf.begin(),
                                        static_cast<size_t> (symbol_size), 0);
                    uint16_t last_symbol = static_cast<uint16_t> (
                                        std::ceil (last_block_bytes /
                                            static_cast<float> (symbol_size)));
                    for (uint16_t idx = 0; idx < dec->needed_symbols(); ++idx) {
                        auto buf_start = buf.begin();
                        dec->add_symbol (buf_start, buf.end(),
                                                            last_symbol + idx);
                    }
                }
                lock.unlock();
            } // else let the other thread stop, and report error
            lock.lock();
            if (thread_status == Out_Status::WORKING)
                thread_status = Out_Status::GRACEFUL_STOP;
            lock.unlock();
            cond.notify_one();
            // wait for all blocks to be decoded.
            // if one can not be decoded exit with error
            write_out.join();
            if (thread_status == Out_Status::EXITED)
                return true;
            std::cerr << "ERR: not all blocks could be decoded\n";
            return false;
        }
        input->read (reinterpret_cast<char *> (&symbol_number),
                                                    sizeof(symbol_number));
        read = input->gcount();
        if (read != sizeof(symbol_number)) {
            std::cerr << "ERR: not enough data to fill symbol number\n";
            thread_status = Out_Status::ERROR;
            cond.notify_one();
            write_out.join();
            return false;
        }
        buf.clear();
        buf.insert (buf.begin(), static_cast<size_t> (symbol_size), 0);
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wshorten-64-to-32"
        input->read (reinterpret_cast<char *> (buf.data()), symbol_size);
        #pragma clang diagnostic pop
        read = input->gcount();
        if (read <= 0) {
            std::cerr << "ERR: unexpected end";
            thread_status = Out_Status::ERROR;
            cond.notify_one();
            write_out.join();
            return false;
        }
        lock.lock();
        auto dec_it = decoders.find (block_number);
        if (dec_it == decoders.end()) {
            // add new decoder
            bool success;
            if (bytes_left == 0) {
                std::cerr << "ERR: additional blocks found.\n";
                thread_status = Out_Status::ERROR;
                lock.unlock();
                cond.notify_one();
                write_out.join();
                return false;
            }
            last_block_bytes = std::min (bytes_left,
                                static_cast<size_t> (
                                static_cast<int64_t>(symbols) * symbol_size));
            bytes_left -= last_block_bytes;
            std::tie (dec_it, success) = decoders.emplace (std::make_pair (
                        block_number,
                        std::unique_ptr<Dec> (
                            new Dec (symbols, static_cast<size_t> (symbol_size),
                                        Dec::Report::PARTIAL_FROM_BEGINNING))));
            if (!success) {
                std::cerr << "ERR: Can not add decoder\n";
                thread_status = Out_Status::ERROR;
                lock.unlock();
                cond.notify_one();
                write_out.join();
                return false;
            }
        }
        auto dec = dec_it->second.get();
        if (dec == nullptr) { // received additional symbol for an
            lock.unlock();
            continue;       // already decoded (and freed) block.
        }
        auto symbol_start = buf.begin();
        auto err = dec->add_symbol (symbol_start, buf.end(), symbol_number);
        lock.unlock();
        if (err != RaptorQ__v1::Error::NONE &&
                                        err != RaptorQ__v1::Error::NOT_NEEDED) {
            std::cerr << "ERR: error adding symbol\n";
            thread_status = Out_Status::ERROR;
            cond.notify_one();
            write_out.join();
            return false;
        }
        cond.notify_one();
    }
}

// encoding function. manages both input and output
static bool encode (const int64_t symbol_size,
                                        const RaptorQ__v1::Block_Size symbols,
                                        const uint32_t repair,
                                        std::istream *input,
                                        std::ostream *output)
{
    // false on error
    std::vector<uint8_t> buf (static_cast<size_t> (symbol_size), 0);
    std::vector<uint8_t> block_buf;
    block_buf.reserve (static_cast<size_t> (symbols) *
                                            static_cast<size_t> (symbol_size));
    // Since we do not change the number of symbols for each block,
    // we can reuse the encoder, so that less works will be done.
    // just call clear_data() before feeding it the next block.
    RaptorQ__v1::Encoder<iter_8, iter_8> encoder (symbols,
                                        static_cast<size_t> (symbol_size));
    auto future = encoder.precompute();
    RaptorQ__v1::Error enc_status = RaptorQ__v1::Error::INITIALIZATION;
    uint32_t sym_num = 0;
    uint32_t block_num = 0;
    while (true) {
        buf.clear(); // with padding already set
        buf.insert (buf.begin(), static_cast<size_t> (symbol_size), 0);
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wshorten-64-to-32"
        input->read (reinterpret_cast<char *> (buf.data()), symbol_size);
        #pragma clang diagnostic pop
        int64_t read = input->gcount();
        if (read > 0) {
            output->write (reinterpret_cast<char *> (&block_num),
                                                            sizeof(block_num));
            output->write (reinterpret_cast<char *> (&sym_num),sizeof(sym_num));
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wshorten-64-to-32"
            output->write (reinterpret_cast<char *> (buf.data()), symbol_size);
            #pragma clang diagnostic pop
            block_buf.insert (block_buf.end(), buf.begin(), buf.end());
            ++sym_num;
        }
        if (input->eof() || read <= 0) {
            // end of input.
            sym_num = static_cast<uint16_t> (symbols);
        }

        if (sym_num == static_cast<uint16_t> (symbols)) {
            // give the data to the encoder. It will pad it automatically.
            size_t ret = encoder.set_data (block_buf.begin(), block_buf.end());
            if (ret != block_buf.size()) {
                std::cout << "ERR: can not add block data to the decoder\n";
                return false;
            }
            // we use the same future multiple times, but it has a shared state
            // only the first time. Do not wait() the other times.
            if (future.valid()) {
                future.wait();
                enc_status = future.get();
            }
            if (enc_status != RaptorQ__v1::Error::NONE) {
                std::cerr << "ERR: encoder should never fail!\n";
                return false;
            }
            std::vector<uint8_t> rep (static_cast<size_t> (symbol_size), 0);
            for (uint32_t rep_id = sym_num; rep_id <
                           (static_cast<size_t> (symbols) + repair); ++rep_id) {
                auto rep_start = rep.begin();
                auto rep_length = encoder.encode (rep_start, rep.end(), rep_id);
                // rep_length is actually the number of iterators written.
                // but our iteerators are over uint8_t, so
                // rep_length == bytes written
                if (rep_length != static_cast<size_t> (symbol_size)) {
                    std::cerr << "ERR: wrong repair symbol size\n";
                    return false;
                }
                output->write (reinterpret_cast<char *> (&block_num),
                                                            sizeof(block_num));
                output->write (reinterpret_cast<char *> (&rep_id),
                                                            sizeof(rep_id));
                #pragma clang diagnostic push
                #pragma clang diagnostic ignored "-Wshorten-64-to-32"
                output->write (reinterpret_cast<char *> (rep.data()),
                                                                symbol_size);
                #pragma clang diagnostic pop
            }
            if (input->eof())
                return true;
            encoder.clear_data();
            block_buf.clear();
            ++block_num;
            sym_num = 0;
        }
    }
}

int main (int argc, char **argv)
{
    // manually parse first argument as command.
    // then use optionparser
    bool helponly = false;
    if (argc == 1 || (strncmp("encode", argv[1], 7) &&
                                            strncmp("decode", argv[1], 7) &&
                                            strncmp("blocks", argv[1], 7) &&
                                            strncmp("benchmark", argv[1], 10))){
        helponly = true;
    }

    // skip both program name and first command
    // apparently "optionparser" treats every option as "unknown" once one
    // unknown option has been found. :/
    auto arg_num   = (argc <= 2 ? argc - 1 : argc - 2);
    char **arguments;
    if (argc <= 1) {
        // no arguments
        arguments = nullptr;
    } else if (argc == 2) {
        // maybe "help" or "format" or "benchmark"
        arguments = argv + 1;
    } else {
        // command. but if we do not skip it the command line parsing library
        // will interpret everything after it as a non-option argument.
        arguments = argv + 2;
    }

    option::Stats  stats (usage, arg_num, arguments);
    std::vector<option::Option> options (stats.options_max);
    std::vector<option::Option> buffer (stats.buffer_max);
    option::Parser parse (usage, arg_num, arguments,
                                const_cast<option::Option *> (options.data()),
                                const_cast<option::Option *> ( buffer.data()));


    if (parse.error() || options[HELP].count() != 0) {
        option::printUsage (std::cout, usage);
        if (parse.error())
            return 1;
        return 0;
    }
    if (options[FORMAT].count() != 0) {
        info (argv[0]);
        return 0;
    }

    if (helponly) {
        std::cerr << "ERR: need a command as first argument: "
                                            "encode/decode/benchmark/blocks\n";
        option::printUsage (std::cout, usage);
        return 1;
    }


    uint32_t repair = 0;
    size_t bytes = 0;
    const std::string command = std::string (argv[1]);
    if (command.compare ("benchmark") == 0) {
        if (options[SYMBOLS].count() != 0 || options[SYMBOL_SIZE].count() != 0
                                        || options[REPAIR].count() != 0
                                        || options[BYTES].count() != 0
                                        || parse.nonOptionsCount() != 1) {
            std::cerr << "ERR: \"benchmark\" does not use arguments\n";
            option::printUsage (std::cout, usage);
            return 1;
        }
        // TODO: launch benchmark
        // This should launch single-thread benchmark up to 1 second, then
        // estimate the amount needed for the next blocks.
        // Meaning we should do some interpolation?
        // is just taking the last 3 points and getting the x^3 curve enough?
        bench (1);
        std::cout << "Extrapolate by yourself. cubic curve :p\n";
        return 0;
    } else if (command.compare ("blocks") == 0) {
        std::cout << "Usable block sizes:\n";
        for (size_t idx = 0; idx < RaptorQ__v1::blocks->size(); ++idx) {
            std::cout << static_cast<uint16_t> ((*RaptorQ__v1::blocks)[idx])
                                                                    << "\n";
        }
        return 0;
    } else if (command.compare ("encode") == 0) {
        bool err = false;
        // parameters that should NOT be here:
        if (options[BYTES].count() != 0) {
            std::cerr << "ERR: encoder does not need the \"--bytes\" "
                                                                "parameter\n";
            err = true;
        }
        // parameter that are missing/multiple times:
        if (options[SYMBOLS].count() != 1) {
            std::cerr << "ERR: encoder requires exactly one \"--symbols\" "
                                                                "parameter\n";
            err = true;
        }
        if (options[SYMBOL_SIZE].count() != 1) {
            std::cerr << "ERR: encoder requires exactly one \"--symbol-size\" "
                                                                "parameter\n";
            err = true;
        }
        if (options[REPAIR].count() != 1) {
            std::cerr << "ERR: encoder requires exactly one \"--repair\" "
                                                                "parameter\n";
            err = true;
        }
        // input/output parameters:
        if (parse.nonOptionsCount() != 2) {
            std::cerr << "ERR: You need to specify exactly one input and output\n";
            if (parse.nonOptionsCount() > 2)
                std:: cerr << "\t(Did you pass unkown parameters?\n";
            err = true;
        }
        if (err) {
            option::printUsage (std::cout, usage);
            return 1;
        }

        repair =  static_cast<uint32_t> (strtol(options[REPAIR].arg, nullptr,
                                                                        10));
        if (repair == 0) {
            std::cerr << "ERR: number of repair symbols must be positive\n";
            return 1;
        }
    } else if (command.compare ("decode") == 0) {
        bool err = false;
        // parameters that should NOT be here:
        if (options[REPAIR].count() != 0) {
            std::cerr << "ERR: decoder does not need the \"--repair\" "
                                                                "parameter\n";
            err = true;
        }
        // parameter that are missing/multiple times:
        if (options[SYMBOLS].count() != 1) {
            std::cerr << "ERR: decoder requires exactly one \"--symbols\" "
                                                                "parameter\n";
            err = true;
        }
        if (options[SYMBOL_SIZE].count() != 1) {
            std::cerr << "ERR: decoder requires exactly one \"--symbol-size\" "
                                                                "parameter\n";
            err = true;
        }
        if (options[BYTES].count() != 1) {
            std::cerr << "ERR: decoder requires exactly one \"--bytes\" "
                                                                "parameter\n";
            err = true;
        }
        // input/output parameters:
        if (parse.nonOptionsCount() != 2) {
            std::cerr << "ERR: You need to specify exactly one input and output\n";
            if (parse.nonOptionsCount() > 2)
                std:: cerr << "\t(Did you pass unkown parameters?\n";
            err = true;
        }
        if (err) {
            option::printUsage (std::cout, usage);
            return 1;
        }

        bytes =  static_cast<size_t> (strtol(options[BYTES].arg, nullptr, 10));
        if (bytes == 0) {
            std::cerr << "ERR: \"--bytes\" must be positive\n";
            return 1;
        }
    } else {
        std::cerr << "ERR: command \"" << command << "\" not understood\n";
        return 1;
    }

    const uint16_t syms = static_cast<uint16_t> (strtol(options[SYMBOLS].arg,
                                                                nullptr, 10));
    const int64_t symbol_size =  static_cast<int64_t> (
                                strtol(options[SYMBOL_SIZE].arg, nullptr, 10));


    if (syms < 1 || syms > 56403) {
        std::cerr << "ERR: Symbols must be between 1 and 56403\n";
        return 1;
    }
    RaptorQ__v1::Block_Size symbols = RaptorQ__v1::Block_Size::Block_10;
    for (size_t idx = 0; idx < RaptorQ__v1::blocks->size(); ++idx) {
        if (static_cast<uint16_t> ((*RaptorQ__v1::blocks)[idx]) >= syms) {
            if (static_cast<uint16_t> ((*RaptorQ__v1::blocks)[idx]) > syms) {
                size_t pre_idx = (idx == 0 ? 0 : idx - 1);
                size_t post_idx = (idx == RaptorQ__v1::blocks->size() - 1 ?
                                                                idx : idx + 1);
                std::cerr << "ERR: wrong block size. Closest blocks: "
                    << static_cast<uint32_t> ((*RaptorQ__v1::blocks)[pre_idx])
                    << " - "
                    << static_cast<uint32_t> ((*RaptorQ__v1::blocks)[idx])
                    << " - "
                    << static_cast<uint32_t> ((*RaptorQ__v1::blocks)[post_idx])
                          << "\n";
                return 1;
            }
            symbols = (*RaptorQ__v1::blocks)[idx];
            break;
        }
    }

    const std::string input_file = parse.nonOption (0);
    const std::string output_file = parse.nonOption (1);


    // try to open input/output files
    std::istream *input;
    std::ostream *output;
    std::ifstream in_file;
    std::ofstream out_file;
    if (input_file.compare("-") == 0) {
        input = &std::cin;
    } else {
        in_file.open (input_file, std::ios_base::binary | std::ios_base::in);
        if (!in_file.is_open()) {
            std::cerr << "ERR: can't open input file\n";
            return 1;
        }
        input = &in_file;
    }
    if (output_file.compare("-") == 0) {
        output = &std::cout;
    } else {
        out_file.open (output_file, std::ios_base::binary | std::ios_base::out
                                                        | std::ios_base::trunc);
        if (!out_file.is_open()) {
            std::cerr << "ERR: can't open output file\n";
            return 1;
        }
        output = &out_file;
    }

    if (command.compare ("encode") == 0) {
        if (encode (symbol_size, symbols, repair, input, output))
            return 0;
        return 1;
    } else {
        if (decode(bytes, symbols, symbol_size, input, output))
            return 0;
        return 1;
    }
}


// Benchmark stuff:

class Timer {
public:
    Timer() {}
    Timer (const Timer&) = delete;
    Timer& operator= (const Timer&) = delete;
    Timer (Timer&&) = delete;
    Timer& operator= (Timer&&) = delete;
    void start()
        { t0 = std::chrono::high_resolution_clock::now(); }
    std::chrono::microseconds stop ()
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto diff = t1 - t0;
        return std::chrono::duration_cast<std::chrono::microseconds> (diff);
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> t0;
};


void bench (uint32_t seconds)
{
    // max 100MB of cache
    // up to block size 10120 (more or less)
    RaptorQ__v1::local_cache_size (1024*1024*100);
    std::cout << "S: block size.\n";
    std::cout << "A: average microseconds\n";
    std::cout << "EF: encoder microseconds without precomputation\n";
    std::cout << "EP: encoder microseconds with    precomputation\n";
    std::cout << "DF: decoder microseconds without precomputation\n";
    std::cout << "DP: decoder microseconds with    precomputation\n";
    // do benchmarks until we find a block that stays "seconds" secs to crunch.
    // then extrapolate the rest.
    for (auto blk : *RaptorQ__v1::blocks) {
        constexpr size_t symbol_size = 1280;    // min supported ipv6 payload
                                                // kinda arbitrary anyway.
        using T = typename std::vector<uint8_t>;
        T data (symbol_size * static_cast<uint16_t> (blk), 0);

        RaptorQ__v1::Encoder<T::iterator, T::iterator> encoder (
                                                            blk, symbol_size);
        encoder.set_data (data.begin(), data.end());
        Timer time;
        time.start();
        if (!encoder.compute_sync()) {
            std::cerr << "Could not encode??\n";
            return;
        }
        auto microsec_encode_full = time.stop();
        // now test after precomputation:
        encoder.clear_data();
        encoder.precompute_sync();
        encoder.set_data (data.begin(), data.end());
        time.start();
        if (!encoder.compute_sync()) {
            std::cerr << "Could not encode??\n";
            return;
        }
        auto microsec_encode_pre = time.stop();
        const uint32_t max_symbol = static_cast<uint16_t> (blk) + 5;
        std::vector<T> encoded (max_symbol);
        for (uint32_t id = 1; id <= max_symbol; ++id) {
            // skip the first so the decoder can do its work
            encoded[id - 1] = T (symbol_size, 0);
            auto b_it = encoded[id - 1].begin();
            encoder.encode (b_it, encoded[id - 1].end(), id);
        }

        using Dec_t = typename RaptorQ__v1::Decoder<T::iterator, T::iterator>;
        Dec_t decoder (blk, symbol_size, Dec_t::Report::COMPLETE);
        uint32_t id = 1;
        for (auto &sym : encoded) {
            auto b_it = sym.begin();
            decoder.add_symbol (b_it, sym.end(), id++);
        }
        time.start();
        auto dec_res = decoder.decode_once();
        auto microsec_decode_full = time.stop();
        if (dec_res != RaptorQ__v1::Decoder_Result::DECODED) {
            std::cerr << "Could not decode size " << static_cast<uint32_t> (blk)
                                                                        << "\n";
            microsec_decode_full = std::chrono::microseconds (0);
            // do not return. can happen :(
        }
        std::chrono::microseconds microsec_decode_pre (0);
        if (microsec_decode_full.count() > 0) {
            // try to decode again, so that precomputation is used.
            Dec_t dec_pre (blk, symbol_size, Dec_t::Report::COMPLETE);
            id = 1;
            for (auto &sym : encoded) {
                auto b_it = sym.begin();
                dec_pre.add_symbol (b_it, sym.end(), id++);
            }
            time.start();
            dec_res = dec_pre.decode_once();
            microsec_decode_pre = time.stop();
        }

        auto avg_microsec = (microsec_encode_full +
                             microsec_encode_pre +
                             microsec_decode_full +
                             microsec_decode_pre) / 4;

        std::cout << "  S: " << static_cast<uint32_t> (blk) <<
                                " A: "  << avg_microsec.count() <<
                                " EF: " << microsec_encode_full.count() <<
                                " EP: " << microsec_encode_pre.count() <<
                                " DF: "  << microsec_decode_full.count() <<
                                " DP: "  << microsec_decode_pre.count() << "\n";
        if (avg_microsec > std::chrono::seconds (seconds))
            break;
    }
}
