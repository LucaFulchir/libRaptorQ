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

#ifndef RAPTORQ_DECODER_HPP
#define RAPTORQ_DECODER_HPP

#include "common.hpp"
#include "Parameters.hpp"
#include "Precode_Matrix.hpp"
#include "Graph.hpp"
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>
#include <vector>
#include <Eigen/Dense>

namespace RaptorQ {
namespace Impl {

template <typename In_It>
class RAPTORQ_API Decoder
{
	using Vect = Eigen::Matrix<Octet, 1, Eigen::Dynamic, Eigen::RowMajor>;
	using T_in = typename std::iterator_traits<In_It>::value_type;
public:
	Decoder (const uint16_t symbols, const uint16_t symbol_size)
		:_symbols (symbols),
				precode (Parameters(symbols)), mask (_symbols)
	{

		IS_INPUT(In_It, "RaptorQ::Impl::Decoder");

		// symbol size is in octets, but we save it in "T" sizes.
		// so be aware that "symbol_size" != "_symbol_size" for now
		source_symbols = DenseMtx (_symbols, symbol_size);
	}

	bool add_symbol (In_It &start, const In_It end, const uint32_t esi);
	bool decode ();
	DenseMtx* get_symbols();
	//std::vector<T> get (const uint16_t symbol) const;

private:
	std::mutex lock;
	const uint16_t _symbols;
	Precode_Matrix precode;
	Bitmask mask;
	DenseMtx source_symbols;

	std::vector<std::pair<uint32_t, Vect>> received_repair;
};


///////////////////////////////////
//
// IMPLEMENTATION OF ABOVE TEMPLATE
//
///////////////////////////////////


template <typename In_It>
bool Decoder<In_It>::add_symbol (In_It &start, const In_It end,
															const uint32_t esi)
{
	// true if added succesfully

	// if we were lucky to get a random access iterator, quickly check that
	// the we have enough data for the symbol.
	if (std::is_same<typename std::iterator_traits<In_It>::iterator_category,
									std::random_access_iterator_tag>::value) {
		if (static_cast<size_t>(end - start) * sizeof(T_in) <
								static_cast<size_t> (source_symbols.cols()))
			return false;
	}

	if (esi >= std::pow (2, 20))
		return false;

	std::lock_guard<std::mutex> guard (lock);
	UNUSED(guard);

	if (mask.get_holes() == 0)
		return false;	// not even needed;
	if (mask.exists(esi))
		return false;	// already present.

	uint16_t col = 0;
	if (esi < _symbols) {
		for (; start != end && col != source_symbols.cols(); ++start) {
			T_in al = *start;
			for (uint8_t *p = reinterpret_cast<uint8_t *> (&al);
						p != reinterpret_cast<uint8_t *> (&al) + sizeof(T_in)
										&& col != source_symbols.cols(); ++p) {
				source_symbols (static_cast<int32_t> (esi), col++) = *p;
			}
		}
		// input iterator might reach end before we get enough data
		// for the symbol.
		if (col != source_symbols.cols())
			return false;
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
			return false;
		received_repair.emplace_back (esi, std::move(v));
	}
	mask.add (esi);

	return true;
}

template <typename In_It>
bool Decoder<In_It>::decode ()
{
	// rfc 6330: can decode when received >= K_padded
	// actually: (K_padded - K) are padding and thus constant and NOT
	// transmitted. so really N >= K.
	if (mask.get_holes() == 0)
		return true;

	if (received_repair.size() < mask.get_holes())
		return false;

	precode.gen (static_cast<uint32_t> (received_repair.size() -
															mask.get_holes()));


	DenseMtx D = DenseMtx (precode._params.S + precode._params.H +
								precode._params.K_padded +
								(received_repair.size() - mask.get_holes()),
														source_symbols.cols());

	// initialize D
	for (uint16_t row = 0; row < precode._params.S + precode._params.H; ++row) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// put non-repair symbols (source symbols) in place
	lock.lock();
	if (mask.get_holes() == 0)	// other thread completed its work before us?
		return true;
	D.block (precode._params.S + precode._params.H, 0,
							source_symbols.rows(), D.cols()) = source_symbols;

	// mask must be copied to avoid threading problems, same with tracking
	// the repair esi.
	const Bitmask mask_safe = mask;
	std::vector<uint32_t> repair_esi;
	repair_esi.reserve (received_repair.size());
	for (auto rep : received_repair)
		repair_esi.push_back (rep.first);

	// fill holes with the first repair symbols available
	auto symbol = received_repair.begin();
	for (uint16_t hole = 0; hole < _symbols && symbol != received_repair.end();
																	++hole) {
		if (mask_safe.exists (hole))
			continue;
		const uint16_t row = precode._params.S + precode._params.H + hole;
		D.row (row) = symbol->second;
		++symbol;
	}
	// fill the padding symbols (always zero)
	for (uint16_t row = precode._params.S + precode._params.H + _symbols;
								row < precode._params.S + precode._params.H +
											precode._params.K_padded; ++row) {
		for (uint16_t col = 0; col < D.cols(); ++col)
			D (row, col) = 0;
	}
	// fill the remaining (redundant) repair symbols
	for (uint16_t row =
			precode._params.S + precode._params.H + precode._params.K_padded;
							symbol != received_repair.end(); ++symbol, ++row) {
		D.row (row) = symbol->second;
	}
	lock.unlock();

	// do not lock this part, as it's the expensive part
	DenseMtx missing = precode.intermediate (D, mask_safe, repair_esi);
	D = DenseMtx();	// free some memory;

	if (missing.rows() == 0)
		return mask.get_holes() == 0; // did other threads do something?

	std::lock_guard<std::mutex> guard (lock);
	UNUSED(guard);

	if (mask.get_holes() == 0)
		return true;			// other thread did something

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

	// free some memory, we don't need recover symbols anymore
	received_repair = std::vector<std::pair<uint32_t, Vect>>();

	return true;
}

template <typename In_It>
DenseMtx* Decoder<In_It>::get_symbols()
{
	return &source_symbols;
}

}	//namespace Impl
}	// namespace RaptorQ

#endif
