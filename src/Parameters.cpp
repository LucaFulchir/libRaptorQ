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

#include "degree.hpp"
#include "Parameters.hpp"
#include "Rand.hpp"

RaptorQ::Impl::Parameters::Parameters(const uint16_t symbols)
{
	uint16_t idx;
	for (idx = 0; idx < RaptorQ::Impl::K_padded.size(); ++idx) {
		if (RaptorQ::Impl::K_padded[idx] >= symbols) {
			K_padded = RaptorQ::Impl::K_padded[idx];
			break;
		}
	}

	J = RaptorQ::Impl::J_K_padded[idx];
	std::tie (S, H, W) = RaptorQ::Impl::S_H_W [idx];

	L = K_padded + S + H;
	P = L - W;
	U = P - H;
	B = W - S;
	P1 = P + 1;			// first prime number bigger than P. turns out its
						// always between 1 and 14 more numbers.
	while (!is_prime (P1))	// so this while will be really quick anyway
		++P1;
}

bool RaptorQ::Impl::Parameters::is_prime (const uint16_t n)
{
	// 1 as prime, don't care. Not in our scope anyway.
	// thank you stackexchange for the code
	if (n <= 3)
		return true;
	if (n % 2 == 0 || n % 3 == 0)
		return false;

	uint32_t i = 5;
	uint32_t w = 2;
	while (i * i <= n) {
		if (n % i == 0)
			return false;
		i += w;
		w = 6 - w;
	}
	return true;
}


uint16_t RaptorQ::Impl::Parameters::Deg (const uint32_t v) const
{
	// rfc 6330, pg 27

	for (uint16_t d = 0; d < RaptorQ::Impl::degree_distribution.size(); ++d) {
		if (v < RaptorQ::Impl::degree_distribution[d])
			return (d < (W - 2)) ? d : (W - 2);
	}
	return 0;	// never get here, but don't make the compiler complain
}

RaptorQ::Impl::Tuple RaptorQ::Impl::Parameters::tuple (const uint32_t ISI) const
{
	RaptorQ::Impl::Rand rnd;
	RaptorQ::Impl::Tuple ret;

	// taken straight from RFC6330, pg 30
	// so thank them for the *beautiful* names
	// also, don't get confused with "B": this one is different,
	// and thus named "B1"

	size_t A = 53591 + J * 997;

	if (A % 2 == 0)
		++A;
	size_t B1 = 10267 * (J + 1);
	uint32_t y = static_cast<uint32_t> (B1 + ISI * A);
	uint32_t v = rnd.get (y, 0, static_cast<uint32_t> (std::pow(2, 20)));
	ret.d = Deg (v);
	ret.a = 1 + static_cast<uint16_t> (rnd.get (y, 1, W - 1));
	ret.b = static_cast<uint16_t> (rnd.get (y, 2, W));
	if (ret.d < 4) {
		ret.d1 = 2 + static_cast<uint16_t> (rnd.get (ISI, 3, 2));
	} else {
		ret.d1 = 2;
	}
	ret.a1 = 1 + static_cast<uint16_t> (rnd.get (ISI, 4, P1 - 1));
	ret.b1 = static_cast<uint16_t> (rnd.get (ISI, 5, P1));

	return ret;
}

std::vector<uint16_t> RaptorQ::Impl::Parameters::get_idxs (const uint32_t ISI)
																		const
{
	// Needed to generate G_ENC: We need the ids of the symbols we would
	// use on a "Enc" call. So this is the "enc algorithm, but returns the
	// indexes instead of computing the result.
	// rfc6330, pg29

	std::vector<uint16_t> ret;
	Tuple t = tuple (ISI);

	ret.reserve (t.d + t.d1);
	ret.push_back (t.b);

	for (uint16_t j = 1; j < t.d; ++j) {
		t.b = (t.b + t.a) % W;
		ret.push_back (t.b);
	}
	while (t.b1 >= P)
		t.b1 = (t.b1 + t.a1) % P1;

	ret.push_back (W + t.b1);
	for (uint16_t j = 1; j < t.d1; ++j) {
		t.b1 = (t.b1 + t.a1) % P1;
		while (t.b1 >= P)
			t.b1 = (t.b1 + t.a1) % P1;
		ret.push_back (W + t.b1);
	}
	return ret;
}
