/*
 * Copyright (c) 2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#pragma clang diagnostic ignored "-Wweak-vtables"
#include "../external/optionparser-1.4/optionparser.h"
#pragma clang diagnostic pop
#include "RaptorQ/RaptorQ_v1.hpp"
#include <cassert>
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
	if (option.arg != 0)
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

enum  optionIndex { UNKNOWN, HELP, SYMBOLS, SYMBOL_SIZE, REPAIR, BYTES };
const option::Descriptor usage[] =
{
 {UNKNOWN, 0, "", "", Arg::Unknown,
						"USAGE: encode|decode|benchmark OPTIONS INPUT OUTPUT\n"
											"  use '-' for stdin/stdout\n\n"
														"Standalone Options:"},
 {HELP,    0, "h", "help", Arg::None, "  -h --help\tThis help."},
 {UNKNOWN, 0, "", "", Arg::Unknown, "ENCODE/DECODE options:"},
 {SYMBOLS, 0, "s", "symbols", Arg::Numeric, "  -s --symbols\t"
												"number of symbols per block"},
 {SYMBOL_SIZE, 0, "w", "symbol-size", Arg::Numeric, "  -w --symbol-size\t"
														" bytes per symbol"},
 {UNKNOWN, 0, "", "", Arg::Unknown, "ENCODE only options:"},
 {REPAIR, 0, "r", "repair", Arg::Numeric, "  -r --repair\t"
										"number of repair symbols per block"},
 {UNKNOWN, 0, "", "", Arg::None, "DECODE only options:"},
 {BYTES, 0, "b", "bytes", Arg::Numeric, "  -b --bytes\t"
									"data size for each {en,de}coder block"},
 {UNKNOWN, 0, "", "", Arg::Unknown,
							"Encoder output format/Decoder input format:\n"
							"\t(uint32_t) block  number\n"
							"\t(uint32_t) symbol number\n"
							"\tsymbol\n"},
 {0,0,0,0,0,0}
};


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
	std::map<size_t, std::unique_ptr<Dec>> *decoders;
	std::mutex *mtx;
	std::condition_variable *cond;
	std::ostream *output;
	Out_Status *status;
};



bool encode (int64_t symbol_size, uint16_t symbols, size_t repair,
									std::istream *input, std::ostream *output);
bool decode (int64_t symbol_size, uint64_t bytes,
									std::istream *input, std::ostream *output);
static void print_output (struct write_out_args args);

// thread function to wait for the decoders to finish decoding and
// print output. Only used when decoding
static void print_output (struct write_out_args args)
{
	size_t current_block = 0;
	uint16_t last_symbol = 0;
	std::unique_lock<std::mutex> lock (*args.mtx, std::defer_lock);
	while (*args.status == Out_Status::WORKING ||
									*args.status == Out_Status::GRACEFUL_STOP) {
		lock.lock();
		auto dec_it = args.decoders->find (current_block);
		if (dec_it == args.decoders->end()) {
			args.cond->wait (lock);
			lock.unlock();
			continue;
		}
		auto dec = dec_it->second.get();
		if (!dec->can_decode()) {
			args.cond->wait (lock);
			lock.unlock();
			continue;
		}
		lock.unlock();
		auto pair = dec->wait_sync();
		if (pair.first == RaptorQ__v1::Error::NEED_DATA) {
			if (*args.status == Out_Status::GRACEFUL_STOP) {
				lock.lock();
				*args.status = Out_Status::ERROR;
				return;
			}
			continue;
		}
		if (pair.first != RaptorQ__v1::Error::NONE) {
			// internal error or interrupted computation
			lock.lock();
			*args.status = Out_Status::ERROR;
			return;
		}
		size_t sym_size = dec->symbol_size();
		std::vector<uint8_t> buffer (sym_size);;
		for (; last_symbol < pair.second; ++last_symbol) {
			buffer.clear();
			auto buf_start = buffer.begin();
			auto to_write = dec->decode_symbol (buf_start, buffer.end(),
																last_symbol);
			if (to_write != 1) {
				std::cerr << "ERR: partial or empty symbol from decoder\n";
				abort();
			}
			args.output->write (reinterpret_cast<char *> (buffer.data()),
												static_cast<int64_t>(sym_size));
		}
		if (last_symbol == dec->symbols()) {
			lock.lock();
			dec_it = args.decoders->find (current_block);
			dec_it->second = nullptr;
			lock.unlock();
			++current_block;
			last_symbol = 0;
		}
	}
	lock.lock();
	*args.status = Out_Status::EXITED;
}

bool decode (int64_t symbol_size, uint64_t bytes,
									std::istream *input, std::ostream *output)
{
	// decode
	std::vector<uint8_t> buf (static_cast<size_t> (symbol_size));
	std::map<size_t, std::unique_ptr<Dec>> decoders;
	uint32_t block_number;
	uint32_t symbol_number;
	std::mutex mtx;
	std::condition_variable cond;
	Out_Status thread_status = Out_Status::WORKING;
	struct write_out_args args;
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
			return 1;
		} else if (read == 0 || input->eof()) {
			lock.lock();
			if (thread_status == Out_Status::WORKING)
				thread_status = Out_Status::GRACEFUL_STOP;
			lock.unlock();
			// wait for all blocks to be decoded.
			// if one can not be decoded exit with error
			write_out.join();
			if (thread_status == Out_Status::EXITED)
				return 0;
			std::cerr << "ERR: not all blocks could be decoded\n";
			return 1;
		}
		input->read (reinterpret_cast<char *> (&symbol_number),
													sizeof(symbol_number));
		read = input->gcount();
		if (read != sizeof(symbol_number)) {
			std::cerr << "ERR: not enough data to fill symbol number\n";
			thread_status = Out_Status::ERROR;
			write_out.join();
			return 1;
		}

		lock.lock();
		auto dec_it = decoders.find (block_number);
		if (dec_it == decoders.end()) {
			// add new decoder
			bool success;
			std::tie (dec_it, success) = decoders.emplace (std::make_pair (
						block_number,
						new Dec (bytes, static_cast<size_t> (symbol_size),
									Dec::Report::PARTIAL_FROM_BEGINNING)));
			if (!success) {
				std::cerr << "ERR: Can not add decoder\n";
				thread_status = Out_Status::ERROR;
				write_out.join();
				return 1;
			}
		}
		auto dec = dec_it->second.get();
		lock.unlock();
		buf.clear();
		buf.insert (buf.begin(), static_cast<size_t> (symbol_size), 0);
		input->read (reinterpret_cast<char *> (buf.data()), symbol_size);
		read = input->gcount();
		if (read <= 0) {
			std::cerr << "ERR: unexpected end";
			thread_status = Out_Status::ERROR;
			write_out.join();
			return 1;
		}
		if (dec == nullptr) // received additional symbol for an
			continue;		// already decoded (and freed) block.
		auto symbol_start = buf.begin();
		auto err = dec->add_symbol (symbol_start, buf.end(), symbol_number);
		if (err != RaptorQ__v1::Error::NONE) {
			std::cerr << "ERR: error adding symbol\n";
			thread_status = Out_Status::ERROR;
			write_out.join();
			return 1;
		}
		cond.notify_one();
	}
}

// encoding function. manages both input and output
bool encode (int64_t symbol_size, uint16_t symbols, size_t repair,
									std::istream *input, std::ostream *output)
{
	std::vector<uint8_t> buf (static_cast<size_t> (symbol_size));
	// Since we do not change the number of symbols for each block,
	// we can reuse the encoder, so that less works will be done.
	// just call clear_data() before feeding it the next block.
	RaptorQ__v1::Encoder<iter_8, iter_8> encoder (symbols,
										static_cast<size_t> (symbol_size));
	auto future = encoder.compute();
	uint32_t sym_num = 0;
	uint32_t block_num = 0;
	while (true) {
		buf.clear();
		buf.insert (buf.begin(), static_cast<size_t> (symbol_size), 0);
		input->read (reinterpret_cast<char *> (buf.data()), symbol_size);
		int64_t read = input->gcount();
		if (read <= 0) {
			std::cerr << "ERR: unexpected end";
			return 1;
		}
		auto added = encoder.add_data (buf.begin(), buf.end());
		if (added != buf.size()) {
			std::cerr << "ERR: error adding?\n";
			return 1;
		}
		output->write (reinterpret_cast<char *> (&block_num),
														sizeof(block_num));
		output->write (reinterpret_cast<char *> (&sym_num),sizeof(sym_num));
		output->write (reinterpret_cast<char *> (buf.data()), symbol_size);
		++sym_num;
		size_t bytes_left = encoder.needed_bytes();
		if (input->eof() || read != static_cast<int64_t> (added)) {
			// we got EOF. Add padding data & symbols to fill the
			// encoder.
			std::vector<uint8_t> padding (bytes_left, 0);
			auto it = padding.begin();
			auto pad_added = encoder.add_data (it, padding.end());
			assert (it == padding.end() && pad_added == padding.size());
			bytes_left = encoder.needed_bytes();
			assert (bytes_left == 0);
		}
		if (bytes_left == 0) {
			future.wait();
			if (future.get() != RaptorQ__v1::Error::NONE) {
				std::cerr << "ERR: encoder should never fail!\n";
				return 1;
			}
			std::vector<uint8_t> rep (static_cast<size_t> (symbol_size), 0);
			for (uint32_t rep_id = sym_num; rep_id < (symbols + repair);
																++rep_id) {
				auto rep_start = rep.begin();
				auto rep_length = encoder.encode(rep_start, rep.end(),
																	rep_id);
				if (rep_length != static_cast<size_t> (symbol_size)) {
					std::cerr << "ERR: wrong repair symbol size\n";
					return 1;
				}
				output->write (reinterpret_cast<char *> (&block_num),
														sizeof(block_num));
				output->write (reinterpret_cast<char *> (&rep_id),
															sizeof(rep_id));
				output->write (reinterpret_cast<char *> (buf.data()),
															symbol_size);
			}
			encoder.clear_data();
			++block_num;
			sym_num = 0;
		}
	}
}

int main (int argc, char **argv)
{
	// manually parse first argument as command.
	// then use optionparser
	if (argc == 1 || (strncmp("encode", argv[1], 7) &&
											strncmp("decode", argv[1], 7) &&
											strncmp("banchmark", argv[1], 10))){
		std::cerr << "ERR: need a command as first argument: "
													"encode/decode/benchmark\n";
		option::printUsage (std::cout, usage);
		return 1;
	}

	// skip both program name and first command
	// apparently "optionparser" treats every option as "unknown" once one
	// unknown option has been found. :/
	auto arg_num   = (argc <= 2 ? 0 : argc - 2);
	auto arguments = (argc <= 2 ? nullptr : argv + 2);

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

	size_t repair = 0;
	size_t bytes = 0;
	const std::string command = std::string (argv[1]);
	if (parse.nonOptionsCount() == 1) {
		if (command.compare ("benchmark") != 0 || options[SYMBOLS].count() != 0
											|| options[SYMBOL_SIZE].count() != 0
											|| options[REPAIR].count() != 0
											|| options[BYTES].count() != 0) {
			std::cerr << "ERR: asdf\n";
			option::printUsage (std::cout, usage);
			return 1;
		}
		// TODO: launch benchmark
		std::cerr << "Benchmarks not implemented yet\n";
		return 0;
	}
	if (command.compare ("encode") == 0) {
		if (options[BYTES].count() != 0) {
			std::cerr << "ERR: encoder does not need \"--bytes\" parameter\n";
			return 1;
		}
		if (options[REPAIR].count() != 1) {
			std::cerr << "ERR: encoder requires one \"--repair\" parameter\n";
			return 1;
		}
		repair =  static_cast<size_t> (strtol(options[REPAIR].arg, nullptr,10));
		if (repair <= 0) {
			std::cerr << "ERR: Symbol_size must be positive\n";
			return 1;
		}
		if (parse.nonOptionsCount() != 2) {
			std::cerr << "ERR: Need to specify exactly one input and output\n";
			option::printUsage (std::cout, usage);
			return 1;
		}
	} else if (command.compare ("decode") == 0) {
		if (options[BYTES].count() != 1) {
			std::cerr << "ERR: encoder requires one \"--bytes\" parameter\n";
			return 1;
		}
		bytes =  static_cast<size_t> (strtol(options[BYTES].arg, nullptr, 10));
		if (bytes <= 0) {
			std::cerr << "ERR: bytes must be positive\n";
			return 1;
		}
		if (options[REPAIR].count() != 1) {
			std::cerr << "ERR: encoder does not need \"--repair\" parameter\n";
			return 1;
		}
		if (parse.nonOptionsCount() != 2) {
			std::cerr << "ERR: Need to specify exactly one input and output\n";
			option::printUsage (std::cout, usage);
			return 1;
		}
	} else {
		std::cerr << "ERR: command \"" << command << "\" not understood\n";
		return 1;
	}

	if (options[SYMBOLS].count() != 1 || options[SYMBOL_SIZE].count() != 1) {
		std::cerr << "ERR: number of symbols and symbols size are required\n";
		return 1;
	}

	const uint16_t symbols = static_cast<uint16_t> (strtol(options[SYMBOLS].arg,
																nullptr, 10));
	const int64_t symbol_size =  static_cast<int64_t> (
								strtol(options[SYMBOL_SIZE].arg, nullptr, 10));


	if (symbols < 1 || symbols > 56403) {
		std::cerr << "ERR: Symbols must be between 1 and 56403\n";
		return 1;
	}
	if (symbol_size <= 0) {
		std::cerr << "ERR: Symbol_size must be positive\n";
		return 1;
	}

	const std::string input_file = parse.nonOption (0);
	const std::string output_file = parse.nonOption (1);


	// try to open input/output files
	std::istream *input;
	std::ostream *output;
	std::ifstream in_file;
	std::ofstream out_file;
	if (input_file.compare("-")) {
		input = &std::cin;
	} else {
		in_file.open (input_file, std::ifstream::binary | std::ifstream::in);
		if (!in_file.is_open()) {
			std::cerr << "ERR: can't open input file\n";
			return 1;
		}
		input = &in_file;
	}
	if (output_file.compare("-")) {
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
		if (decode(symbol_size, bytes, input, output))
			return 0;
		return 1;
	}
}
