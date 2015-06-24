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

#include "Operation.hpp"

RaptorQ::Impl::Operation::~Operation () {}

/////////////////////
// Operation_Swap
/////////////////////
uint64_t RaptorQ::Impl::Operation_Swap::size () const
{
	return sizeof(uint8_t) + 2 * sizeof(uint16_t);
}

std::ostream &RaptorQ::Impl::Operation_Swap::print (std::ostream &os) const
{
	os << static_cast<uint8_t> (Operation_type::SWAP);
	os << _row_1 << _row_2;
	return os;
}

void RaptorQ::Impl::Operation_Swap::build_mtx (RaptorQ::Impl::DenseMtx &mtx)
																		const
{
	mtx.row(_row_1).swap (mtx.row(_row_2));
}

RaptorQ::Impl::DenseMtx RaptorQ::Impl::Operation_Swap::to_Matrix (
													const uint16_t size) const
{
	DenseMtx ret;
	ret.setIdentity (size, size);
	ret.row(_row_1).swap (ret.row(_row_2));

	return ret;
}

/////////////////////
// Operation_Add_Mul
/////////////////////

uint64_t RaptorQ::Impl::Operation_Add_Mul::size () const
{
	return sizeof(uint8_t) + 2 * sizeof(uint16_t) + sizeof(uint8_t);
}

std::ostream &RaptorQ::Impl::Operation_Add_Mul::print (std::ostream &os) const
{
	os << static_cast<uint8_t> (Operation_type::ADD_MUL);
	os << _row_1 << _row_2 << static_cast<uint8_t> (_scalar);
	return os;
}

void RaptorQ::Impl::Operation_Add_Mul::build_mtx (RaptorQ::Impl::DenseMtx &mtx)
																		const
{
	// rationale:
	//   no data -> insert the _scalar as it is.
	//   data -> multiply data by scalar, so that previous data is conserved.
	if (static_cast<uint8_t> (mtx (_row_1, _row_2)) == 0)
		mtx (_row_1, _row_2) = 1;
	mtx (_row_1, _row_2) *= _scalar;
}

RaptorQ::Impl::DenseMtx RaptorQ::Impl::Operation_Add_Mul::to_Matrix (
													const uint16_t size) const
{
	DenseMtx ret;
	ret.setIdentity (size, size);
	ret (_row_1, _row_2) = _scalar;

	return ret;
}

/////////////////////
// Operation_Div
/////////////////////
uint64_t RaptorQ::Impl::Operation_Div::size () const
{
	return sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t);
}

std::ostream &RaptorQ::Impl::Operation_Div::print (std::ostream &os) const
{
	os << static_cast<uint8_t> (Operation_type::DIV);
	os << _row_1 << static_cast<uint8_t> (_scalar);
	return os;
}


void RaptorQ::Impl::Operation_Div::build_mtx (RaptorQ::Impl::DenseMtx &mtx)
																		const
{
	// rationale:
	//   no data -> insert the _scalar as it is.
	//   data -> divide data by scalar, so that previous data is conserved.
	if (static_cast<uint8_t> (mtx (_row_1, _row_1)) == 0)
		mtx (_row_1, _row_1) = 1;
	mtx (_row_1, _row_1) /= _scalar;
}

RaptorQ::Impl::DenseMtx RaptorQ::Impl::Operation_Div::to_Matrix (
													const uint16_t size) const
{
	DenseMtx ret;
	ret.setIdentity (size, size);
	ret (_row_1, _row_1) = _scalar.inverse();

	return ret;
}

/////////////////////
// Operation_Block
/////////////////////

uint64_t RaptorQ::Impl::Operation_Block::size () const
{
	return sizeof(uint8_t) + sizeof(uint16_t) +
						static_cast<uint64_t> (_block.rows() * _block.cols());
}

std::ostream &RaptorQ::Impl::Operation_Block::print (std::ostream &os) const
{
	os << static_cast<uint8_t> (Operation_type::BLOCK);
	// X is a square matrix
	os << static_cast<uint16_t> (_block.cols());
	for (uint16_t row = 0; row < _block.cols(); ++row) {
		for (uint16_t col = 0; col < _block.cols(); ++col)
			os << _block (row, col);
	}
	return os;
}

void RaptorQ::Impl::Operation_Block::build_mtx (RaptorQ::Impl::DenseMtx &mtx)
																		const
{
	DenseMtx ret;
	ret.setIdentity(mtx.rows(), mtx.cols());
	ret.block (0, 0, _block.rows(), _block.cols()) = _block;
	mtx *= ret;
}

RaptorQ::Impl::DenseMtx RaptorQ::Impl::Operation_Block::to_Matrix (
													const uint16_t size) const
{
	DenseMtx ret;
	ret.setIdentity(size, size);
	ret.block (0, 0, _block.rows(), _block.cols()) = _block;
	return ret;
}

/////////////////////
// Operation_Reorder
/////////////////////

uint64_t RaptorQ::Impl::Operation_Reorder::size () const
{
	return sizeof(uint8_t) + sizeof(uint16_t) * _order.size();
}

std::ostream &RaptorQ::Impl::Operation_Reorder::print (std::ostream &os) const
{
	os << static_cast<uint8_t> (Operation_type::REORDER);
	os << static_cast<uint16_t> (_order.size());
	for (uint16_t val : _order)
		os << val;
	return os;
}

void RaptorQ::Impl::Operation_Reorder::build_mtx (RaptorQ::Impl::DenseMtx &mtx)
																		const
{
	DenseMtx ret = DenseMtx (mtx.rows(), mtx.cols());

	uint16_t row = 0;
	for (const uint16_t pos : _order)
		ret.row (pos) = mtx.row (row++);
}

RaptorQ::Impl::DenseMtx RaptorQ::Impl::Operation_Reorder::to_Matrix (
													const uint16_t size) const
{
	DenseMtx ret = DenseMtx (size, size);
	ret.setZero ();

	uint16_t row = 0;
	for (uint16_t pos : _order)
		ret (row++, pos) = 1;

	return ret;
}

