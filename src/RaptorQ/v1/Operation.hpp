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
    Operation (const Operation &rhs)
        :_type (rhs._type)
    {
        switch (rhs._type)
        {
        case _t::SWAP:
            swap = rhs.swap;
            break;
        case _t::ADD_MUL:
            add_mul = rhs.add_mul;
            break;
        case _t::DIV:
            div = rhs.div;
            break;
        case _t::BLOCK:
            block = rhs.block;
            break;
        case _t::REORDER:
            reorder = rhs.reorder;
            break;
        case _t::NONE:
            break;
        }
    }
    Operation& operator= (const Operation &rhs)
    {
        assert (_type == rhs._type && "Operation types do not correspond.");
        switch (rhs._type)
        {
        case _t::SWAP:
            swap = rhs.swap;
            break;
        case _t::ADD_MUL:
            add_mul = rhs.add_mul;
            break;
        case _t::DIV:
            div = rhs.div;
            break;
        case _t::BLOCK:
            block = rhs.block;
            break;
        case _t::REORDER:
            reorder = rhs.reorder;
            break;
        case _t::NONE:
            break;
        }
        return *this;
    }

    Operation (Operation &&rhs)
        :_type (rhs._type)
    {
        switch (rhs._type)
        {
        case _t::SWAP:
            swap = std::move (rhs.swap);
            break;
        case _t::ADD_MUL:
            add_mul = std::move (rhs.add_mul);
            break;
        case _t::DIV:
            div = std::move (rhs.div);
            break;
        case _t::BLOCK:
            block = std::move (rhs.block);
            break;
        case _t::REORDER:
            reorder = std::move (rhs.reorder);
            break;
        case _t::NONE:
            break;
        }
    }

    Operation& operator= (Operation &&rhs)
    {
        assert (_type == rhs._type && "Operation types do not correspond.");
        switch (rhs._type)
        {
        case _t::SWAP:
            swap = std::move (rhs.swap);
            break;
        case _t::ADD_MUL:
            add_mul = std::move (rhs.add_mul);
            break;
        case _t::DIV:
            div = std::move (rhs.div);
            break;
        case _t::BLOCK:
            block = std::move (rhs.block);
            break;
        case _t::REORDER:
            reorder = std::move (rhs.reorder);
            break;
        case _t::NONE:
            break;
        }
        return *this;
    }

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
    Operation (const _t type)
        :_type (type) {}
    class RAPTORQ_LOCAL Swap
    {
    public:
        Swap (const uint16_t row_1, const uint16_t row_2)
            : _row_1 (row_1), _row_2 (row_2) {}
        Swap (const Swap&) = default;
        Swap& operator= (const Swap&) = default;
        Swap (Swap &&) = default;
        Swap& operator= (Swap &&) = default;
        ~Swap() {}
        void build_mtx (DenseMtx &mtx) const
            { mtx.row(_row_1).swap (mtx.row(_row_2)); }
    private:
        uint16_t _row_1, _row_2;
    };

    class RAPTORQ_LOCAL Add_Mul
    {
    public:
        Add_Mul (const uint16_t row_1, const uint16_t row_2, const Octet scalar)
            : _row_1 (row_1), _row_2 (row_2), _scalar (scalar) {}
        Add_Mul (const Add_Mul&) = default;
        Add_Mul& operator= (const Add_Mul&) = default;
        Add_Mul (Add_Mul&&) = default;
        Add_Mul& operator= (Add_Mul&&) = default;
        ~Add_Mul() {}
        void build_mtx (DenseMtx &mtx) const
        {
            const auto row = mtx.row (_row_2) * _scalar;
            mtx.row (_row_1) += row;
        }
    private:
        uint16_t _row_1, _row_2;
        Octet _scalar;
    };

    class RAPTORQ_LOCAL Div
    {
    public:
        Div (const uint16_t row_1, const Octet scalar)
            : _row_1 (row_1), _scalar (scalar) {}
        Div (const Div&) = default;
        Div& operator= (const Div&) = default;
        Div (Div&&) = default;
        Div& operator= (Div&&) = default;
        ~Div() {}
        void build_mtx (DenseMtx &mtx) const
            { mtx.row (_row_1) /= _scalar; }
    private:
        uint16_t _row_1;
        Octet _scalar;
    };

    class RAPTORQ_LOCAL Block
    {
    public:
        Block (const DenseMtx &block)
            : _block (block) {}
        Block (const Block&) = default;
        Block& operator= (const Block&) = default;
        Block (Block&&) = default;
        Block& operator= (Block&&) = default;
        ~Block() {}
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
        Reorder (const Reorder&) = default;
        Reorder& operator= (const Reorder&) = default;
        Reorder (Reorder&&) = default;
        Reorder& operator= (Reorder&&) = default;
        ~Reorder() {}
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
        Swap swap;
        Add_Mul add_mul;
        Div div;
        Block block;
        Reorder reorder;
    };
};

}   // namespace Impl
}   // namespace RaptorQ
