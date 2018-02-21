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
#include "RaptorQ/v1/DenseOctetMatrix.hpp"
#include "RaptorQ/v1/util/Matrix.hpp"
#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace RaptorQ__v1 {
namespace Impl {

using DenseMtx = Eigen::Matrix<Octet, Eigen::Dynamic, Eigen::Dynamic,
                                                            Eigen::RowMajor>;
using SparseMtx = Eigen::SparseMatrix<Octet, Eigen::RowMajor>;


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
    Operation (const _t type, const SparseMtx &mtx)
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

    void build_mtx (DenseOctetMatrix &mtx) const
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

    void serialize (std::vector<uint8_t> &vec) const
    {
        switch (_type)
        {
        case _t::SWAP:
            return swap.serialize (vec);
        case _t::ADD_MUL:
            return add_mul.serialize (vec);
        case _t::DIV:
            return div.serialize (vec);
        case _t::BLOCK:
            return block.serialize (vec);
        case _t::REORDER:
            return reorder.serialize (vec);
        case _t::NONE:
            break;
        }
    }

    static uint16_t deserialize_uint16(const std::vector<uint8_t> &vec,
                                                                    int index)
    {
        return vec[index] + (vec[index + 1] << 8);
    }

    static void serialize_uint16(std::vector<uint8_t> &vec, uint16_t number)
    {
        vec.emplace_back(number & 0xff);
        vec.emplace_back(number >> 8);
    }

    static uint32_t deserialize_uint32(const std::vector<uint8_t> &vec,
                                                                    int index)
    {
        return vec[index] + (vec[index + 1] << 8) + (vec[index + 2] << 16) +
                                                        (vec[index + 3] << 24);
    }

    static void serialize_uint32(std::vector<uint8_t> &vec, uint32_t number)
    {
        vec.emplace_back(number & 0xff);
        vec.emplace_back((number >> 8)  & 0xff);
        vec.emplace_back((number >> 16)  & 0xff);
        vec.emplace_back(number >> 24);
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
        void build_mtx (DenseOctetMatrix &mtx) const
        {
            //mtx.row(_row_1).swap (mtx.row(_row_2));
            Matrix::row_swap(mtx, _row_1, _row_2);
        }
        void serialize (std::vector<uint8_t> &vec) const
        {
            vec.emplace_back(static_cast<uint8_t> (_t::SWAP));
            serialize_uint16(vec, _row_1);
            serialize_uint16(vec, _row_2);
        }
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
        void build_mtx (DenseOctetMatrix &mtx) const
        {
            //const auto row = mtx.row (_row_2) * _scalar;
            //mtx.row (_row_1) += row;
            Matrix::row_multiply_add(mtx, _row_1, mtx, _row_2,
                                                static_cast<uint8_t> (_scalar));
        }
        void serialize (std::vector<uint8_t> &vec) const
        {
            vec.emplace_back(static_cast<uint8_t> (_t::ADD_MUL));
            serialize_uint16(vec, _row_1);
            serialize_uint16(vec, _row_2);
            vec.emplace_back(static_cast<uint8_t> (_scalar));
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
        void build_mtx (DenseOctetMatrix &mtx) const
        {
            //mtx.row (_row_1) /= _scalar;
            Matrix::row_div(mtx, _row_1, static_cast<uint8_t> (_scalar));
        }
        void serialize (std::vector<uint8_t> &vec) const
        {
            vec.emplace_back(static_cast<uint8_t> (_t::DIV));
            serialize_uint16(vec, _row_1);
            vec.emplace_back(static_cast<uint8_t> (_scalar));
        }
    private:
        uint16_t _row_1;
        Octet _scalar;
    };

    class RAPTORQ_LOCAL Block
    {
    public:
        Block (const SparseMtx &block)
            : _block (block) {}
        Block (const Block&) = default;
        Block& operator= (const Block&) = default;
        Block (Block&&) = default;
        Block& operator= (Block&&) = default;
        ~Block() {}
        void build_mtx (DenseOctetMatrix &mtx) const
        {
            //const auto orig = mtx.block (0,0, _block.cols(), mtx.cols());
            //mtx.block (0, 0, _block.cols(), mtx.cols()) = _block * orig;

            DenseMtx orig = mtx.toEigen(0, 0, _block.cols(), mtx.cols());
            DenseMtx res = _block * orig; // Sparse * Dense
            mtx.valuesFromEigen(res, 0, 0, _block.cols(), mtx.cols());
        }
        void serialize (std::vector<uint8_t> &vec) const
        {
            vec.emplace_back(static_cast<uint8_t> (_t::BLOCK));
            serialize_uint16(vec, _block.rows());
            serialize_uint16(vec, _block.cols());
            serialize_uint32(vec, _block.nonZeros());

            for (int k=0; k<_block.outerSize(); ++k) {
                for (SparseMtx::InnerIterator it(_block,k); it; ++it)
                {
                    serialize_uint16(vec, it.row());
                    serialize_uint16(vec, it.col());
                    vec.emplace_back(static_cast<uint8_t> (it.value()));
                }
            }
        }
        /* It's not good to call the destructor explicit but I see no other way.
         * A sparse matrix thas has memory allocated even when it's of size 0x0.
         */
        void clear()
            { _block.~SparseMtx(); }
    private:
        SparseMtx _block;
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
        void build_mtx (DenseOctetMatrix &mtx) const
        {
            uint16_t overhead = static_cast<uint16_t> (
                            static_cast<uint16_t> (mtx.rows()) - _order.size());
            DenseOctetMatrix ret = DenseOctetMatrix (mtx.rows() - overhead,
                                                                    mtx.cols());

            // reorder some of the lines as requested by the _order vector
            uint16_t row = 0;
            for (const uint16_t pos : _order) {
                //ret.row (pos) = mtx.row (row++);
                Matrix::row_assign(ret, pos, mtx, row++);
            }
            mtx = ret;
            // other lines will not influence the computation, ignore them
        }
        void serialize (std::vector<uint8_t> &vec) const
        {
            vec.emplace_back(static_cast<uint8_t> (_t::REORDER));
            serialize_uint32(vec, _order.size());
            for (const uint16_t pos : _order) {
                serialize_uint16(vec, pos);
            }
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
