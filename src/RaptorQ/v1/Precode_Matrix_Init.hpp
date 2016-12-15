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

#include "RaptorQ/v1/Precode_Matrix.hpp"
#include "RaptorQ/v1/Rand.hpp"
#include <limits>

///////////////////
//
// Precode_Matrix
//
///////////////////

namespace RaptorQ__v1 {
namespace Impl {

///
/// These methods are used to generate the precode matrix.
///

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::gen (const uint32_t repair_overhead)
{
    _repair_overhead = repair_overhead;
    DenseMtx _A = DenseMtx (_params.L + repair_overhead, _params.L);

    init_LDPC1 (_A, _params.S, _params.B);
    add_identity (_A, _params.S, 0, _params.B);
    init_LDPC2 (_A, _params.W, _params.S, _params.P);
    init_HDPC (_A);
    add_identity (_A, _params.H, _params.S, _params.L - _params.H);
    add_G_ENC (_A);
    // G_ENC only fills up to L rows, but we might have overhead.
    // initialize it.
    for (uint16_t row = _params.L; row < A.rows(); ++row) {
        for (uint16_t col = 0; col < A.cols(); ++col)
            _A (row, col) = 0;
    }
    A = std::move (_A);
}

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::init_LDPC1 (DenseMtx &_A, const uint16_t S,
                                                        const uint16_t B) const
{
    // The first LDPC1 submatrix is a SxB matrix of SxS submatrixes
    // (the last submatrix can have less than S columns)
    // each submatrix is full zero, with some exceptions:
    // in the first column positions "0", "i + 1" and "2 * (i+1)" are set
    // to 1. All next columns are all downshifts of the first.
    // which makes each submatrix a circulant matrix)

    // You won't find this directly on the rfc, but you can find it in the book:
    //  Raptor Codes Foundations and Trends in Communications
    //      and Information Theory

    uint16_t row, col;
    for (row = 0; row < S;++row) {
        for (col = 0; col < B; ++col) {
            bool zero = true;
            uint16_t submtx = col / S;
            if ((row == (col % S)) ||                   // column 0 & downshifts
                    (row == (col + submtx + 1) % S) ||  // i + 1 & downshifts
                    (row == (col + 2 * (submtx + 1)) % S)) {// 2* (i+1) & dshift
                zero = false ;
            }
            _A (row, col) = (zero ? 0 : 1);
        }
    }
}

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::add_identity (DenseMtx &_A,
                                                const uint16_t size,
                                                const uint16_t skip_row,
                                                const uint16_t skip_col) const
{
    auto sub_mtx = _A.block (skip_row, skip_col, size, size);
    for (uint16_t row = 0; row < sub_mtx.rows(); ++row) {
        for (uint16_t col = 0; col < sub_mtx.cols(); ++col)
            sub_mtx (row, col) = (row == col ? 1 : 0);
    }
}

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::init_LDPC2 (DenseMtx &_A, const uint16_t skip,
                                                    const uint16_t rows,
                                                    const uint16_t cols) const
{
    // this submatrix has two consecutive "1" in the first row, first two
    // colums, and then every other row is the previous right shifted.

    // You won't find this easily on the rfc, but you can see this in the book:
    //  Raptor Codes Foundations and Trends in Communications
    //  and Information Theory
    auto sub_mtx = _A.block (0, skip, rows, cols);
    for (uint16_t row = 0; row < sub_mtx.rows(); ++row) {
        uint16_t start = row % cols;
        for (uint16_t col = 0; col < sub_mtx.cols(); ++col) {
            if (col == start || col == (start + 1) % cols) {
                sub_mtx (row, col) = 1;
            } else {
                sub_mtx (row, col) = 0;
            }
        }
    }
}

template<Save_Computation IS_OFFLINE>
DenseMtx Precode_Matrix<IS_OFFLINE>::make_MT() const
{
    // rfc 6330, pg 24

    DenseMtx MT = DenseMtx (_params.H, _params.K_padded + _params.S);

    for (uint16_t row = 0; row < MT.rows(); ++row) {
        uint16_t col;
        for (col = 0; col < MT.cols() - 1; ++col) {
            auto tmp = rnd_get (col + 1, 6, _params.H);
            if ((row == tmp) || (row ==
                                (tmp + rnd_get (col + 1, 7, _params.H - 1) + 1)
                                                                % _params.H)) {
                MT (row, col) = 1;
            } else {
                MT (row, col) = 0;
            }
        }
        // last column: alpha ^^ i, as in rfc6330
        MT (row, col) = RaptorQ__v1::Impl::oct_exp[row];
    }
    return MT;
}

template<Save_Computation IS_OFFLINE>
DenseMtx Precode_Matrix<IS_OFFLINE>::make_GAMMA() const
{
    // rfc 6330, pg 24
    DenseMtx GAMMA = DenseMtx (_params.K_padded + _params.S,
                                                _params.K_padded + _params.S);

    for (uint16_t row = 0; row < GAMMA.rows(); ++row) {
        uint16_t col;
        for (col = 0; col <= row; ++col)
            // alpha ^^ (i-j), as in rfc6330, pg24
            // end of Section 5.7.2: alpha^^i == oct_exp(i)
            //
            // rfc only says "i-j", while the ^^ op. is defined only if
            // the exponent is < 255.
            // we could actually use oct_exp.size() (=>510) since oct_exp
            // actually contains the same values twice. We still use 255
            // so that our implementation is more similar to the other.
            GAMMA (row, col) = RaptorQ__v1::Impl::oct_exp[(row - col) % 255];
        for (; col < GAMMA.cols(); ++col) {
            GAMMA (row, col) = 0;
        }
    }
    return GAMMA;
}

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::init_HDPC (DenseMtx &_A) const
{
    // rfc 6330, pg 25
    DenseMtx MT = make_MT();
    DenseMtx GAMMA = make_GAMMA();

    _A.block(_params.S, 0, _params.H, GAMMA.rows()) = MT * GAMMA;
}

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::add_G_ENC (DenseMtx &_A) const
{
    // rfc 6330, pg 26
    for (uint16_t row = _params.S + _params.H; row < _params.L; ++row) {
        // all to zero
        for (uint16_t col = 0; col < _params.L; ++col)
            _A (row, col) = 0;
        // only overwrite with ones the columns that need it
        auto idxs = _params.get_idxs ((row - _params.S) - _params.H);
        for (auto idx : idxs)
            _A (row, idx) = 1;
    }
}

}   // namespace RaptorQ
}   // namespace Impl
