/*
 * Copyright (c) 2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#include <array>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include "../src/RaptorQ.hpp"
#include <string>
#include <thread>
#include <tuple>
#include <vector>


uint64_t decode (uint32_t mysize, std::mt19937_64 &rnd, float drop_prob,
															uint8_t overhead);

#ifdef USING_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif	//using_clang
static std::mutex global_mtx;
#ifdef USING_CLANG
#pragma clang diagnostic pop
#endif	//using_clang


// for each matrix size, test it multiple times (encode + decode),
// with different overheads (0, 1, 2 symbols).
void conform_test (uint16_t *K_idx, uint32_t *test_num,
			std::array<std::tuple<uint8_t, uint16_t, uint32_t>, 477> *failures);
void conform_test (uint16_t *K_idx, uint32_t *test_num,
			std::array<std::tuple<uint8_t, uint16_t, uint32_t>, 477> *failures)
{
	std::mt19937_64 rnd;
	std::ifstream rand("/dev/random");
	uint64_t seed = 0;
	rand.read (reinterpret_cast<char *> (&seed), sizeof(seed));
	rand.close ();
	rnd.seed (seed);

	while (true) {
		global_mtx.lock();
		auto idx = *K_idx;
		auto test = *test_num;
		++(*test_num);
		if (test >= 1010100) {
			*test_num = 0;
			idx = ++(*K_idx);
		}
		global_mtx.unlock();

		if (idx >= 477)
			return;

		uint8_t overhead = 0;
		// each test gets a little more dropped packets, so that
		// more repair packets are used.
		// Also, 20% max drop pro 0 overhead, 30% for 1 overhead,
		// and 40% for 3 overhead.
		float max_drop = (20 / 100) * test;
		if (test >= 100) {
			++overhead;
			max_drop = (30 / 10000) * test;
		}
		if (test >= 10000) {
			++overhead;
			max_drop = (40 / 1000000) * test;
		}
		auto size = RaptorQ::Impl::K_padded[idx];
		auto time = decode (size, rnd, max_drop, overhead);
		if (time == 0) {
			global_mtx.lock();
			uint32_t third;
			uint16_t second;
			uint8_t first;
			std::tie (first, second, third) = (*failures)[idx];
			if (test < 100) {
				++first;
			} else if (test < 10000) {
				++second;
			} else {
				++third;
			}
			(*failures)[idx] = std::make_tuple (first, second, third);
			global_mtx.unlock();
		}
	}
}

class Timer {
public:

	Timer();
	void start();
	uint64_t stop ();	// microseconds
private:
	struct clock
	{
	    typedef unsigned long long                 rep;
		// My machine is 2.4 GHz
	    typedef std::ratio<1, 2400000000>          period;
	    typedef std::chrono::duration<rep, period> duration;
	    typedef std::chrono::time_point<clock>     time_point;
	    static const bool is_steady =              true;

	    static time_point now() noexcept
	    {
	        unsigned lo, hi;
	        asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
	        return time_point(duration(static_cast<rep>(hi) << 32 | lo));
	    }
	};
	// typedef std::chrono::microseconds microseconds;
    // typedef std::chrono::duration<double,
	//				typename std::chrono::high_resolution_clock::period> Cycle;
	// std::chrono::time_point<std::chrono::high_resolution_clock> t0;
    typedef std::chrono::duration<uint64_t, std::micro> microseconds;
    typedef std::chrono::duration<double, typename clock::period> Cycle;
	std::chrono::time_point<clock> t0;
};

Timer::Timer() {}
void Timer::start()
{
	//t0 = std::chrono::high_resolution_clock::now();
	t0 = clock::now();
}

uint64_t Timer::stop()
{
	//auto t1 = std::chrono::high_resolution_clock::now();
	auto t1 = clock::now();
	auto ticks_per_iter = Cycle (t1 - t0);

    return static_cast<uint64_t> (std::chrono::duration_cast<microseconds>(
													ticks_per_iter).count());
}

// for each matrix size, test it once (encode + decode), get the average time.
void bench (uint16_t *K_idx, std::array<uint64_t, 477> *times);
void bench (uint16_t *K_idx, std::array<uint64_t, 477> *times)
{
	std::mt19937_64 rnd;
	std::ifstream rand("/dev/random");
	uint64_t seed = 0;
	rand.read (reinterpret_cast<char *> (&seed), sizeof(seed));
	rand.close ();
	rnd.seed (seed);

	while (true) {
		global_mtx.lock();
		auto idx = *K_idx;
		++(*K_idx);
		global_mtx.unlock();
		if (idx >= 477)
			return;
		auto size = RaptorQ::Impl::K_padded[idx];
		std::uniform_real_distribution<float> drop (0.0, 20.0);
		uint64_t time = decode (size, rnd, drop(rnd), 4);
		(*times)[idx] = time;
		std::cout << "K: " << size << " time: " << time << "\n";
	}
}

uint64_t decode (uint32_t mysize, std::mt19937_64 &rnd, float drop_prob,
															uint8_t overhead)
{
	// returns average number of microseconds for encoding and decoding
	Timer t;
	std::vector<uint32_t> myvec;

	//initialize vector
	std::uniform_int_distribution<uint32_t> distr(0, ~static_cast<uint32_t>(0));
	myvec.reserve (mysize);
	for (uint32_t i = 0; i < mysize; ++i)
		myvec.push_back (distr(rnd));

	std::vector<std::pair<uint32_t, std::vector<uint32_t>>> encoded;

	const uint16_t subsymbol = 8;
	const uint16_t symbol_size = 8;
	auto enc_it = myvec.begin();
	RaptorQ::Encoder<std::vector<uint32_t>::iterator,
									std::vector<uint32_t>::iterator> enc (
					enc_it, myvec.end(), subsymbol, symbol_size, 1073741824);

	t.start();
	enc.precompute(1, false);
	uint64_t micro1 = t.stop();

	if (micro1 == 0)
		return 0;

	if (drop_prob > 100.0)
		drop_prob = 90.0;	// this is still too high probably.
	std::uniform_real_distribution<float> drop (0.0, 100.0);

	int32_t repair = overhead;

	for (auto block : enc) {
		for (auto sym_it = block.begin_source(); sym_it != block.end_source();
																	++sym_it) {
			float dropped = drop (rnd);
			if (dropped <= drop_prob) {
				++repair;
				continue;
			}
			std::vector<uint32_t> source_sym;
			source_sym.reserve (symbol_size / 4);
			source_sym.insert (source_sym.begin(), symbol_size / 4, 0);
			auto it = source_sym.begin();
			(*sym_it) (it, source_sym.end());
			encoded.emplace_back ((*sym_it).id(), std::move(source_sym));
		}
		auto sym_it = block.begin_repair();
		for (; repair >= 0 && sym_it != block.end_repair (block.max_repair());
																	++sym_it) {
			// repair symbols can be lost, too
			float dropped = drop (rnd);
			if (dropped <= drop_prob) {
				continue;
			}
			--repair;
			std::vector<uint32_t> repair_sym;
			repair_sym.reserve (symbol_size / 4);
			repair_sym.insert (repair_sym.begin(), symbol_size / 4, 0);
			auto it = repair_sym.begin();
			(*sym_it) (it, repair_sym.end());
			encoded.emplace_back ((*sym_it).id(), std::move(repair_sym));
		}
		// we dropped waaaay too many symbols! how much are you planning to
		// lose, again???
		if (sym_it == block.end_repair (block.max_repair())) {
			std::cout << "Maybe losing " << drop_prob << "% is too much?\n";
			return 0;
		}
	}
	auto oti_scheme = enc.OTI_Scheme_Specific();
	auto oti_common = enc.OTI_Common();

	RaptorQ::Decoder<std::vector<uint32_t>::iterator, std::vector<uint32_t>::
										iterator> dec (oti_common, oti_scheme);

	std::vector<uint32_t> received;
	received.reserve (mysize);
	for (uint32_t i = 0; i < mysize; ++i)
		received.push_back (0);

	for (size_t i = 0; i < encoded.size(); ++i) {
		auto it = encoded[i].second.begin();
		dec.add_symbol (it, encoded[i].second.end(), encoded[i].first);
	}

	auto re_it = received.begin();
	t.start();
	auto decoded = dec.decode(re_it, received.end(), 0);
	uint64_t micro2 = t.stop();

	if (decoded != mysize) {
		std::cout << "NOPE: " << decoded << " - " << mysize << "\n";
		return 0;
	} else {
		std::cout << "OK: " << mysize << "-" << micro1 << "\n";
	}
	for (uint16_t i = 0; i < mysize; ++i) {
		if (myvec[i] != received[i]) {
			std::cout << "FAILED, but we though otherwise! " << mysize << " - "
											<< drop_prob << " - " <<
											static_cast<int> (overhead) << "\n";
			return 0;
		}
	}

	return (micro1 + micro2) / 2;
}

int main (int argc, char **argv)
{
	// get the amount of threads to use
	uint32_t threads;
	bool conformity = false;

	switch (argc) {
	case 2:
		// one argument: the number of threads.
		// or the string "conformity" to test the
		// rfc6330 conformity specifics
		if (!strncmp("conformity", argv[1], strlen(argv[1]))) {
			threads = std::thread::hardware_concurrency();
			conformity = true;
		} else {
			char *end_ptr = nullptr;
			threads = static_cast<uint32_t> (strtol(argv[1], &end_ptr, 10));
			if ((end_ptr != nullptr && end_ptr != argv[1] + strlen(argv[1]))) {
				// some problem. print help and exit
				std::cout << "Usage:\t\t" << argv[0] << " [threads]\n";
				std::cout << "rfc test:\t" << argv[0] << " conformity\n";
				return 1;
			}
			if (threads == 0) {
				threads = std::thread::hardware_concurrency();
			}
		}
		break;
	default:
		std::cout << "libRaptorQ tests\n";
		std::cout << "\tuse this to verify the library performance\n";
		std::cout << "\tUsage:\t\t" << argv[0] << " [threads]\n";
		std::cout << "\trfc test:\t" << argv[0] << " conformity\n";
		return 1;
	}


	uint16_t K_index = 0;

	std::vector<std::thread> t;
	t.reserve (threads);
	if (!conformity) {
		std::array<uint64_t, 477> times;
		for (uint32_t i = 0; i < 477; ++i)
			times[i] = 0;
		for (uint8_t i = 0; i < threads; ++i)
			t.emplace_back (bench, &K_index, &times);
		while (K_index != 477) {
			std::this_thread::sleep_for (std::chrono::seconds(10));
			std::cout << "Done: " << K_index << "==" <<
									RaptorQ::Impl::K_padded[K_index] << "\n";
		}
		for (uint8_t i = 0; i < threads; ++i)
			t[i].join();
		for (uint16_t i = 0; i < 477; ++i)
			std::cout << RaptorQ::Impl::K_padded[i] << "\t-\t" << times[i]
																		<< "\n";
	} else {
		std::array<std::tuple<uint8_t, uint16_t, uint32_t>, 477> failures;
		for (uint16_t i = 0; i < 477; ++i)
			failures[i] = std::make_tuple (static_cast<uint8_t> (0),
						static_cast<uint16_t> (0), static_cast<uint32_t> (0));
		uint32_t test_num = 0;
		for (uint8_t i = 0; i < threads; ++i)
			t.emplace_back (conform_test, &K_index, &test_num, &failures);
		while (K_index != 477 && test_num < 1000000) {
			std::this_thread::sleep_for (std::chrono::seconds(10));
			std::cout << "Done: " << K_index << "==" <<
								RaptorQ::Impl::K_padded[K_index] << ". Test:"
															<< test_num << "\n";
		}
		for (uint8_t i = 0; i < threads; ++i)
			t[i].join();
	}

	return 0;
}
