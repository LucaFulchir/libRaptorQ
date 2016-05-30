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

#include "common.hpp"
#include "Parameters.hpp"
#include <Eigen/Dense>

namespace RaptorQ {
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
	virtual ~Operation ();
	virtual void build_mtx (DenseMtx &mtx) const = 0;
	virtual uint64_t size() const = 0;
	friend std::ostream &operator<< (std::ostream &os, const Operation &op) {
		return op.print (os);
	}
private:
	virtual std::ostream &print (std::ostream &os) const = 0;
};



class RAPTORQ_LOCAL Operation_Swap : public Operation
{
public:
	Operation_Swap (const uint16_t row_1, const uint16_t row_2)
		: _row_1 (row_1), _row_2 (row_2) {}
	void build_mtx (DenseMtx &mtx) const override;
	uint64_t size () const override;
private:
	const uint16_t _row_1, _row_2;

	std::ostream &print (std::ostream &os) const override;
};

class RAPTORQ_LOCAL Operation_Add_Mul : public Operation
{
public:
	Operation_Add_Mul (const uint16_t row_1, const uint16_t row_2,
															const Octet scalar)
		: _row_1 (row_1), _row_2 (row_2), _scalar (scalar) {}
	void build_mtx (DenseMtx &mtx) const override;
	uint64_t size () const override;
private:
	const uint16_t _row_1, _row_2;
	const Octet _scalar;

	std::ostream &print (std::ostream &os) const override;
};

class RAPTORQ_LOCAL Operation_Div : public Operation
{
public:
	Operation_Div (const uint16_t row_1, const Octet scalar)
		: _row_1 (row_1), _scalar (scalar) {}
	void build_mtx (DenseMtx &mtx) const override;
	uint64_t size () const override;
private:
	const uint16_t _row_1;
	const Octet _scalar;

private:
	std::ostream &print (std::ostream &os) const override;
};

class RAPTORQ_LOCAL Operation_Block : public Operation
{
public:
	Operation_Block (const DenseMtx &block)
		: _block (block) {}
	void build_mtx (DenseMtx &mtx) const override;
	uint64_t size () const override;
private:
	const DenseMtx _block;

private:
	std::ostream &print (std::ostream &os) const override;
};

class RAPTORQ_LOCAL Operation_Reorder : public Operation
{
public:
	Operation_Reorder (const std::vector<uint16_t> &order)
		: _order (order) {}
	void build_mtx (DenseMtx &mtx) const override;
	uint64_t size () const override;
private:
	const std::vector<uint16_t> _order;

private:
	std::ostream &print (std::ostream &os) const override;
};

}	// namespace Impl
}	// namespace RaptorQ
