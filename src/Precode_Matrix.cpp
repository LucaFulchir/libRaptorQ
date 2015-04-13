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

#include "Graph.hpp"
#include "multiplication.hpp"
#include "Precode_Matrix.hpp"
#include "Rand.hpp"


// force promotion to double in division
namespace {
float RAPTORQ_LOCAL div_floor (const float a, const float b);
float RAPTORQ_LOCAL div_ceil (const float a, const float b);

float div_floor (const float a, const float b)
{
	return std::floor (a / b);
}
float div_ceil (const float a, const float b)
{
	return std::ceil (a / b);
}
}

///////////////////
//
// Bitmask
//
///////////////////

namespace RaptorQ {
namespace Impl {

Bitmask::Bitmask (const uint16_t symbols)
	: _max_nonrepair (symbols)
{
	holes = _max_nonrepair;
	size_t max_element = static_cast<size_t> (div_ceil (_max_nonrepair,
															sizeof(size_t)));
	mask.reserve (max_element + 1);
	for (size_t i = 0; i <= max_element; ++i)
		mask.push_back (0);
}

void Bitmask::add (const size_t id)
{
	size_t element = static_cast<size_t> (div_floor (id, sizeof(size_t)));
	while (element >= mask.size())
		mask.push_back(0);
	if (exists(id))
		return;

	size_t add_mask = 1 << (id - (element * sizeof(size_t)));
	mask[element] |= add_mask;
	if (id < _max_nonrepair)
		--holes;
}

bool Bitmask::exists (const size_t id ) const
{
	size_t element = static_cast<size_t> (div_floor (id, sizeof(size_t)));
	if (element >= mask.size())
		return false;

	size_t check_mask = 1 << (id - (element * sizeof(size_t)));
	return (mask[element] & check_mask) != 0;
}

uint16_t Bitmask::get_holes () const
{
	return holes;
}

///////////////////
//
// Precode_Matrix
//
///////////////////

///
/// These methods are used to generate the precode matrix.
///

void Precode_Matrix::gen (const uint32_t repair_overhead)
{
	_repair_overhead = repair_overhead;
	A = DenseMtx (_params.L + repair_overhead, _params.L);

	init_LDPC1 (_params.S, _params.B);
	add_identity (_params.S, 0, _params.B);
	init_LDPC2 (_params.W, _params.S, _params.P);
	init_HDPC ();
	add_identity (_params.H, _params.S, _params.L - _params.H);
	add_G_ENC ();
	// G_ENC only fills up to L rows, but we might have overhead.
	// initialize it.
	for (uint16_t row = _params.L; row < A.rows(); ++row) {
		for (uint16_t col = 0; col < A.cols(); ++col)
			A(row, col) = 0;
	}
}

void Precode_Matrix::init_LDPC1 (const uint16_t S, const uint16_t B)
{
	// The first LDPC1 submatrix is a SxB matrix of SxS submatrixes
	// (the last submatrix can have less than S columns)
	// each submatrix is full zero, with some exceptions:
	// in the first column positions "0", "i + 1" and "2 * (i+1)" are set
	// to 1. All next columns are all downshifts of the first.
	// which makes each submatrix a circulant matrix)

	// You won't find this directly on the rfc, but you can find it in the book:
	//  Raptor Codes Foundations and Trends in Communications
	//		and Information Theory

	uint16_t row, col;
	for (row = 0; row < S;++row) {
		for (col = 0; col < B; ++col) {
			bool zero = true;
			uint16_t submtx = col / S;
			if ((row == (col % S)) ||					// column 0 & downshifts
					(row == (col + submtx + 1) % S) ||	// i + 1 & downshifts
					(row == (col + 2 * (submtx + 1)) % S)) {// 2* (i+1) & dshift
				zero = false ;
			}
			A (row, col) = (zero ? 0 : 1);
		}
	}
}

void Precode_Matrix::add_identity (const uint16_t size, const uint16_t skip_row,
														const uint16_t skip_col)
{
	auto sub_mtx = A.block (skip_row, skip_col, size, size);
	for (uint16_t row = 0; row < sub_mtx.rows(); ++row) {
		for (uint16_t col = 0; col < sub_mtx.cols(); ++col)
			sub_mtx (row, col) = (row == col ? 1 : 0);
	}
}

void Precode_Matrix::init_LDPC2 (const uint16_t skip, const uint16_t rows,
															const uint16_t cols)
{
	// this submatrix has two consecutive "1" in the first row, first two
	// colums, and then every other row is the previous right shifted.

	// You won't find this easily on the rfc, but you can see this in the book:
	//  Raptor Codes Foundations and Trends in Communications
	//	and Information Theory
	auto sub_mtx = A.block (0, skip, rows, cols);
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

DenseMtx Precode_Matrix::make_MT() const
{
	// rfc 6330, pg 24
	Rand rnd;

	DenseMtx MT = DenseMtx (_params.H, _params.K_padded + _params.S);

	for (uint16_t row = 0; row < MT.rows(); ++row) {
		uint16_t col;
		for (col = 0; col < MT.cols() - 1; ++col) {
			auto tmp = rnd.get (col + 1, 6, _params.H);
			if ((row == tmp) || (row ==
								(tmp + rnd.get (col + 1, 7, _params.H - 1) + 1)
																% _params.H)) {
				MT (row, col) = 1;
			} else {
				MT (row, col) = 0;
			}
		}
		// last column: alpha ^^ i, as in rfc6330
		MT (row, col) = RaptorQ::Impl::oct_exp[row];
	}
	return MT;
}

DenseMtx Precode_Matrix::make_GAMMA() const
{
	// rfc 6330, pg 24
	DenseMtx GAMMA = DenseMtx (_params.K_padded + _params.S,
												_params.K_padded + _params.S);

	for (uint16_t row = 0; row < GAMMA.rows(); ++row) {
		uint16_t col;
		for (col = 0; col <= row; ++col)
			// alpha ^^ (i-j), as in rfc6330, pg24
			// rfc only says "i-j", but we could go well over oct_exp size.
			// we hope they just missed a modulus :/
			GAMMA (row, col) = RaptorQ::Impl::oct_exp[(row - col) %
												RaptorQ::Impl::oct_exp.size()];
		for (; col < GAMMA.cols(); ++col) {
			GAMMA (row, col) = 0;
		}
	}
	return GAMMA;
}

void Precode_Matrix::init_HDPC ()
{
	// rfc 6330, pg 25
	DenseMtx MT = make_MT();
	DenseMtx GAMMA = make_GAMMA();

	A.block(_params.S, 0, _params.H, GAMMA.rows()) = MT * GAMMA;
}

void Precode_Matrix::add_G_ENC ()
{
	// rfc 6330, pg 26
	for (uint16_t row = _params.S + _params.H; row < _params.L; ++row) {
		// all to zero
		for (uint16_t col = 0; col < _params.L; ++col)
			A (row, col) = 0;
		// only overwrite with ones the columns that need it
		auto idxs = _params.get_idxs ((row - _params.S) - _params.H);
		for (auto idx : idxs)
			A(row, idx) = 1;
	}
}

}	// namespace RaptorQ
}	// namespace Impl
