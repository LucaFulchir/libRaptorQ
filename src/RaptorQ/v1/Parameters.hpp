/*
 * Copyright (c) 2015-2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include "RaptorQ/v1/degree.hpp"
#include "RaptorQ/v1/Rand.hpp"
#include "RaptorQ/v1/table2.hpp"
#include <cmath>
#include <Eigen/Core>
#include <vector>

namespace RaptorQ__v1 {
namespace Impl {

class RAPTORQ_LOCAL Tuple
{
    // d  1-30  LT-degree of encoded symbol
    // a  0-(W-1)
    // b  0-(W-1)
    // d1 PI-degree of encoded symbol (2 or 3)
    // a1 0-(P1-1)
    // b1 0-(P1-1)
public:
    Tuple() = default;
    Tuple (const Tuple&) = default;
    Tuple& operator= (const Tuple&) = default;
    Tuple (Tuple&&) = default;
    Tuple& operator= (Tuple&&) = default;
    ~Tuple() = default;

    uint16_t d, a, b, d1, a1, b1;   // great names. thanks rfc6330!
};

class RAPTORQ_API Parameters
{
public:
    explicit Parameters (const uint16_t symbols);
    Parameters() = delete;
    Parameters (const Parameters&) = default;
    Parameters& operator= (const Parameters&) = default;
    Parameters (Parameters&&) = default;
    Parameters& operator= (Parameters&&) = default;
    ~Parameters() {}

    uint16_t Deg (const uint32_t v) const;
    Tuple tuple (const uint32_t ISI) const;
    std::vector<uint16_t> get_idxs (const uint32_t ISI) const;

    uint16_t K_padded, S, H, W, L, P, P1, U, B; // RFC 6330, pg 22
    uint16_t J;
private:
    static bool is_prime (const uint16_t n);
};


inline Parameters::Parameters (const uint16_t symbols)
{
    uint16_t idx;
    for (idx = 0; idx < RaptorQ__v1::Impl::K_padded.size(); ++idx) {
        if (RaptorQ__v1::Impl::K_padded[idx] >= symbols) {
            K_padded = RaptorQ__v1::Impl::K_padded[idx];
            break;
        }
    }

    J = RaptorQ__v1::Impl::J_K_padded[idx];
    std::tie (S, H, W) = RaptorQ__v1::Impl::S_H_W [idx];

    L = K_padded + S + H;
    P = L - W;
    U = P - H;
    B = W - S;
    P1 = P + 1;         // first prime number bigger than P. turns out its
                        // always between 1 and 14 more numbers.
    while (!is_prime (P1))  // so this while will be really quick anyway
        ++P1;
}

inline bool Parameters::is_prime (const uint16_t n)
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


inline uint16_t Parameters::Deg (const uint32_t v) const
{
    // rfc 6330, pg 27

    for (uint16_t d = 0; d < RaptorQ__v1::Impl::degree_distribution.size();++d){
        if (v < RaptorQ__v1::Impl::degree_distribution[d])
            return (d < (W - 2)) ? d : (W - 2);
    }
    return 0;   // never get here, but don't make the compiler complain
}

inline Tuple RaptorQ__v1::Impl::Parameters::tuple (const uint32_t ISI) const
{
    RaptorQ__v1::Impl::Tuple ret;

    // taken straight from RFC6330, pg 30
    // so thank them for the *beautiful* names
    // also, don't get confused with "B": this one is different,
    // and thus named "B1"

    size_t A = 53591 + J * 997;

    if (A % 2 == 0)
        ++A;
    size_t B1 = 10267 * (J + 1);
    uint32_t y = static_cast<uint32_t> (B1 + ISI * A);
    uint32_t v = rnd_get (y, 0, static_cast<uint32_t> (std::pow(2, 20)));
    ret.d = Deg (v);
    ret.a = 1 + static_cast<uint16_t> (rnd_get (y, 1, W - 1));
    ret.b = static_cast<uint16_t> (rnd_get (y, 2, W));
    if (ret.d < 4) {
        ret.d1 = 2 + static_cast<uint16_t> (rnd_get (ISI, 3, 2));
    } else {
        ret.d1 = 2;
    }
    ret.a1 = 1 + static_cast<uint16_t> (rnd_get (ISI, 4, P1 - 1));
    ret.b1 = static_cast<uint16_t> (rnd_get (ISI, 5, P1));

    return ret;
}

inline std::vector<uint16_t> Parameters::get_idxs (const uint32_t ISI) const
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

}   // namespace Impl
}   // namespace RaptorQ
