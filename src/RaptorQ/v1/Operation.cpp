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

#include "RaptorQ/v1/Operation.hpp"

RaptorQ__v1::Impl::Operation::~Operation () {}

/////////////////////
// Operation_Swap
/////////////////////
uint64_t RaptorQ__v1::Impl::Operation_Swap::size () const
{
    return sizeof(uint8_t) + 2 * sizeof(uint16_t);
}

void RaptorQ__v1::Impl::Operation_Swap::build_mtx (
                                        RaptorQ__v1::Impl::DenseMtx &mtx) const
{
    mtx.row(_row_1).swap (mtx.row(_row_2));
}

/////////////////////
// Operation_Add_Mul
/////////////////////

uint64_t RaptorQ__v1::Impl::Operation_Add_Mul::size () const
{
    return sizeof(uint8_t) + 2 * sizeof(uint16_t) + sizeof(uint8_t);
}

void RaptorQ__v1::Impl::Operation_Add_Mul::build_mtx (
                                        RaptorQ__v1::Impl::DenseMtx &mtx) const
{
    const auto row = mtx.row (_row_2) * _scalar;
    mtx.row (_row_1) += row;
}

/////////////////////
// Operation_Div
/////////////////////
uint64_t RaptorQ__v1::Impl::Operation_Div::size () const
{
    return sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t);
}

void RaptorQ__v1::Impl::Operation_Div::build_mtx (
                                        RaptorQ__v1::Impl::DenseMtx &mtx) const
{
    mtx.row (_row_1) /= _scalar;
}

/////////////////////
// Operation_Block
/////////////////////

uint64_t RaptorQ__v1::Impl::Operation_Block::size () const
{
    return sizeof(uint8_t) + sizeof(uint16_t) +
                        static_cast<uint64_t> (_block.rows() * _block.cols());
}

void RaptorQ__v1::Impl::Operation_Block::build_mtx (
                                        RaptorQ__v1::Impl::DenseMtx &mtx) const
{
    const auto orig = mtx.block (0,0, _block.cols(), mtx.cols());
    mtx.block (0, 0, _block.cols(), mtx.cols()) = _block * orig;
}

/////////////////////
// Operation_Reorder
/////////////////////

uint64_t RaptorQ__v1::Impl::Operation_Reorder::size () const
{
    return sizeof(uint8_t) + sizeof(uint16_t) * _order.size();
}

void RaptorQ__v1::Impl::Operation_Reorder::build_mtx (
                                        RaptorQ__v1::Impl::DenseMtx &mtx) const
{
    uint16_t overhead = static_cast<uint16_t> (
                            static_cast<uint16_t> (mtx.rows()) - _order.size());
    DenseMtx ret = DenseMtx (mtx.rows() - overhead , mtx.cols());

    // reorder some of the lines as requested by the _order vector
    uint16_t row = 0;
    for (const uint16_t pos : _order)
        ret.row (pos) = mtx.row (row++);
    mtx.swap (ret);
    // other lines will not influence the computation, ignore them
}
