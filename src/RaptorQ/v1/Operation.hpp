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
#include "RaptorQ/v1/Parameters.hpp"
#include "RaptorQ/v1/Octet.hpp"
#include <Eigen/Dense>

namespace RaptorQ__v1 {
namespace Impl {

using DenseMtx = Eigen::Matrix<Octet, Eigen::Dynamic, Eigen::Dynamic,
                                                            Eigen::RowMajor>;


class RAPTORQ_LOCAL Operation
{
public:
    enum class _t : uint8_t {
        NONE = 0x00,
        SWAP = 0x01,
        ADD_MUL = 0x02,
        DIV = 0x03,
        BLOCK = 0x04,
        REORDER = 0x05
    };
    Operation() = delete;
    Operation (const _t type, const uint16_t row_1, const uint16_t row_2)
        : _type (type), swap (row_1, row_2) { assert (type == _t::SWAP); }
    Operation (const _t type, const uint16_t row_1, const uint16_t row_2,
                                                            const Octet scalar)
        : _type (type), add_mul (row_1, row_2, scalar)
                                            { assert (type == _t::ADD_MUL); }
    Operation (const _t type, const uint16_t row, const Octet scalar)
        : _type (type), div (row, scalar) { assert (type == _t::DIV); }
    Operation (const _t type, const DenseMtx &mtx)
        : _type (type), block (mtx) { assert (type == _t::BLOCK); }
    Operation (const _t type, const std::vector<uint16_t> &order)
        : _type (type), reorder (order) { assert (type == _t::REORDER); }

    ~Operation ()
    {
        if (_type == _t::BLOCK)
            block.clear();
        if (_type == _t::REORDER)
            reorder.clear();
    }
    void build_mtx (DenseMtx &mtx) const
    {
        switch (_type)
        {
        case _t::SWAP:
            return swap.build_mtx (mtx);
        case _t::ADD_MUL:
            return add_mul.build_mtx (mtx);
        case _t::DIV:
            return div.build_mtx (mtx);
        case _t::BLOCK:
            return block.build_mtx (mtx);
        case _t::REORDER:
            return reorder.build_mtx (mtx);
        case _t::NONE:
            break;
        }
    }
private:
    class RAPTORQ_LOCAL Swap
    {
    public:
        Swap (const uint16_t row_1, const uint16_t row_2)
            : _row_1 (row_1), _row_2 (row_2) {}
        void build_mtx (DenseMtx &mtx) const
            { mtx.row(_row_1).swap (mtx.row(_row_2)); }
    private:
        const uint16_t _row_1, _row_2;
    };

    class RAPTORQ_LOCAL Add_Mul
    {
    public:
        Add_Mul (const uint16_t row_1, const uint16_t row_2, const Octet scalar)
            : _row_1 (row_1), _row_2 (row_2), _scalar (scalar) {}
        void build_mtx (DenseMtx &mtx) const
        {
            const auto row = mtx.row (_row_2) * _scalar;
            mtx.row (_row_1) += row;
        }
    private:
        const uint16_t _row_1, _row_2;
        const Octet _scalar;
    };

    class RAPTORQ_LOCAL Div
    {
    public:
        Div (const uint16_t row_1, const Octet scalar)
            : _row_1 (row_1), _scalar (scalar) {}
        void build_mtx (DenseMtx &mtx) const
            { mtx.row (_row_1) /= _scalar; }
    private:
        const uint16_t _row_1;
        const Octet _scalar;
    };

    class RAPTORQ_LOCAL Block
    {
    public:
        Block (const DenseMtx &block)
            : _block (block) {}
        void build_mtx (DenseMtx &mtx) const
        {
            const auto orig = mtx.block (0,0, _block.cols(), mtx.cols());
            mtx.block (0, 0, _block.cols(), mtx.cols()) = _block * orig;
        }
        void clear()
            { _block = DenseMtx(); }
        private:
        DenseMtx _block;
    };

    class RAPTORQ_LOCAL Reorder
    {
    public:
        Reorder (const std::vector<uint16_t> &order)
            : _order (order) {}
        void build_mtx (DenseMtx &mtx) const
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
        void clear()
            { _order = std::vector<uint16_t>(); }
    private:
        std::vector<uint16_t> _order;
    };

    const _t _type;
    union {
        const Swap swap;
        const Add_Mul add_mul;
        const Div div;
        Block block;
        Reorder reorder;
    };
};

}   // namespace Impl
}   // namespace RaptorQ
