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
enum class Operation_type : uint8_t {
    NONE = 0x00,
    SWAP = 0x01,
    ADD_MUL = 0x02,
    DIV = 0x03,
    BLOCK = 0x04,
    REORDER = 0x05
};

class RAPTORQ_LOCAL Operation
{
public:
    virtual ~Operation () {}
    virtual void build_mtx (DenseMtx &mtx) const = 0;
    virtual uint64_t size() const = 0;
};



class RAPTORQ_LOCAL Operation_Swap final : public Operation
{
public:
    Operation_Swap (const uint16_t row_1, const uint16_t row_2)
        : _row_1 (row_1), _row_2 (row_2) {}
    void build_mtx (DenseMtx &mtx) const override
        { mtx.row(_row_1).swap (mtx.row(_row_2)); }
    uint64_t size () const override
         { return sizeof(uint8_t) + 2 * sizeof(uint16_t); }
private:
    const uint16_t _row_1, _row_2;
};

class RAPTORQ_LOCAL Operation_Add_Mul final : public Operation
{
public:
    Operation_Add_Mul (const uint16_t row_1, const uint16_t row_2,
                                                            const Octet scalar)
        : _row_1 (row_1), _row_2 (row_2), _scalar (scalar) {}
    void build_mtx (DenseMtx &mtx) const override
    {
        const auto row = mtx.row (_row_2) * _scalar;
        mtx.row (_row_1) += row;
    }
    uint64_t size () const override
        { return sizeof(uint8_t) + 2 * sizeof(uint16_t) + sizeof(uint8_t); }
private:
    const uint16_t _row_1, _row_2;
    const Octet _scalar;
};

class RAPTORQ_LOCAL Operation_Div final : public Operation
{
public:
    Operation_Div (const uint16_t row_1, const Octet scalar)
        : _row_1 (row_1), _scalar (scalar) {}
    void build_mtx (DenseMtx &mtx) const override
        { mtx.row (_row_1) /= _scalar; }
    uint64_t size () const override
        { return sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t); }
private:
    const uint16_t _row_1;
    const Octet _scalar;
};

class RAPTORQ_LOCAL Operation_Block final : public Operation
{
public:
    Operation_Block (const DenseMtx &block)
        : _block (block) {}
    void build_mtx (DenseMtx &mtx) const override
    {
        const auto orig = mtx.block (0,0, _block.cols(), mtx.cols());
        mtx.block (0, 0, _block.cols(), mtx.cols()) = _block * orig;
    }
    uint64_t size () const override
    {
        return sizeof(uint8_t) + sizeof(uint16_t) +
                        static_cast<uint64_t> (_block.rows() * _block.cols());
    }
private:
    const DenseMtx _block;
};

class RAPTORQ_LOCAL Operation_Reorder final : public Operation
{
public:
    Operation_Reorder (const std::vector<uint16_t> &order)
        : _order (order) {}
    void build_mtx (DenseMtx &mtx) const override
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
    uint64_t size () const override
        { return sizeof(uint8_t) + sizeof(uint16_t) * _order.size(); }
private:
    const std::vector<uint16_t> _order;
};

}   // namespace Impl
}   // namespace RaptorQ
