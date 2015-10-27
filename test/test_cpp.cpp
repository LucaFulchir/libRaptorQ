/*
 * Copyright (c) 2015, Luca Fulchir<luker@fenrirproject.org>, All rights reserved.
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

#include <fstream>
#include <iostream>
#include <random>
#include "../src/RaptorQ.hpp"
#include <vector>

// Demonstration of how to use the C++ interface
// it's pretty simple, we generate some input,
// then encode, drop some packets (source and repair)
// and finally decode everything.

bool decode (uint32_t mysize, std::mt19937_64 &rnd, float drop_prob,
														const uint8_t overhead);
bool decode (uint32_t mysize, std::mt19937_64 &rnd, float drop_prob,
														const uint8_t overhead)
{
	std::vector<uint32_t> myvec;

	//initialize vector with random data
	std::uniform_int_distribution<uint32_t> distr(0, ~static_cast<uint32_t>(0));
	myvec.reserve (mysize);
	for (uint32_t i = 0; i < mysize; ++i)
		myvec.push_back (distr(rnd));

	// std::pair<symbol id (esi), symbol>
	std::vector<std::pair<uint32_t, std::vector<uint32_t>>> encoded;

	// symbol and sub-symbol sizes
	const uint16_t subsymbol = 8;
	const uint16_t symbol_size = 16;
	auto enc_it = myvec.begin();
	// use multiple blocks
	RaptorQ::Encoder<std::vector<uint32_t>::iterator,
									std::vector<uint32_t>::iterator> enc (
					enc_it, myvec.end(), subsymbol, symbol_size, 200);
	std::cout << static_cast<int32_t>(enc.blocks()) << " blocks\n";
	if (!enc) {
		std::cout << "Coud not initialize encoder.\n";
		return false;
	}

	enc.precompute(1, false);

	if (drop_prob > 90.0)
		drop_prob = 90.0;	// this is still too high probably.
	std::uniform_real_distribution<float> drop (0.0, 100.0);

	int32_t repair;

	int32_t blockid = -1;
	for (auto block : enc) {
		repair = overhead;
		++blockid;
		std::cout << "Block " << blockid << " with " << block.symbols() <<
																" symbols\n";
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
			std::vector<uint32_t> source_sym;
			source_sym.reserve (symbol_size / sizeof(uint32_t));
			source_sym.insert (source_sym.begin(),
											symbol_size / sizeof(uint32_t), 0);
			auto it = source_sym.begin();
			// save the symbol
			auto written = (*sym_it) (it, source_sym.end());
			if (written != source_sym.size()) {
				std::cout << "Could not get the whole source symbol!\n";
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
			std::vector<uint32_t> repair_sym;
			repair_sym.reserve (symbol_size / sizeof(uint32_t));
			repair_sym.insert (repair_sym.begin(),
											symbol_size / sizeof(uint32_t), 0);
			auto it = repair_sym.begin();
			// save the repair symbol
			auto written = (*sym_it) (it, repair_sym.end());
			if (written != repair_sym.size()) {
				std::cout << "Could not get the whole repair symbol!\n";
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

	RaptorQ::Decoder<std::vector<uint32_t>::iterator, std::vector<uint32_t>::
										iterator> dec (oti_common, oti_scheme);

	std::vector<uint32_t> received;
	received.reserve (mysize);
	// make sure that there's enough place in "received" to get the
	// whole decoded data.
	for (uint32_t i = 0; i < mysize; ++i)
		received.push_back (0);

	for (size_t i = 0; i < encoded.size(); ++i) {
		auto it = encoded[i].second.begin();
		if (!dec.add_symbol (it, encoded[i].second.end(), encoded[i].first))
			std::cout << "error adding?\n";
	}

	auto re_it = received.begin();
	// decode all blocks
	// you can actually call ".decode(...)" as many times
	// as you want. It will only start decoding once
	// it has enough data.
	auto decoded = dec.decode(re_it, received.end());

	if (decoded != mysize) {
		std::cout << "Couldn't decode: " << mysize << "\n";
		return false;
	} else {
		std::cout << "Decoded: " << mysize << "\n";
	}
	for (uint16_t i = 0; i < mysize; ++i) {
		if (myvec[i] != received[i]) {
			std::cout << "FAILED, but we though otherwise! " << mysize << " - "
					<< drop_prob << " at " << i << ":" << received[i] << "\n";
			//return false;
		}
	}

	return true;
}

int main (void)
{
	// get a random number generator
	std::mt19937_64 rnd;
	std::ifstream rand("/dev/random");
	uint64_t seed = 0;
	rand.read (reinterpret_cast<char *> (&seed), sizeof(seed));
	rand.close ();
	rnd.seed (seed);

	// encode and decode
	bool ret = decode (501, rnd, 20.0, 4);

	return (ret == true ? 0 : -1);
}
