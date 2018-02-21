/*
 * Copyright (c) 2018 by Intinor AB. All rights reserved.
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
#include "RaptorQ/v1/Octet.hpp"
#include <stdlib.h>
#include <cstdlib>
#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace RaptorQ__v1 {
namespace Impl {

using DenseMtx = Eigen::Matrix<Octet, Eigen::Dynamic, Eigen::Dynamic,
                                                            Eigen::RowMajor>;

class RAPTORQ_LOCAL DenseOctetMatrixBlock {
public:

    DenseOctetMatrixBlock(uint8_t *data, uint32_t startrow,
                          uint32_t startcolumn, uint32_t rows,
                          uint32_t columns, int32_t stride) {
        blockrowcnt = rows;
        blockcolumncnt = columns;
        blockstep = stride;
        blockdata = data + index(startrow, startcolumn);
    }

    ~DenseOctetMatrixBlock() {
    }

    uint8_t& operator() (uint32_t row, uint32_t col) {
        assert(row < blockrowcnt);
        assert(col < blockcolumncnt);
        return blockdata[index(row, col)];
    }

    int32_t rows() const { return (int)this->blockrowcnt; }
    int32_t cols() const { return (int)this->blockcolumncnt; }

    DenseMtx toEigen() {
        DenseMtx ret = DenseMtx(blockrowcnt, blockcolumncnt);

        for (uint32_t i = 0; i < blockrowcnt; i++) {
            for (uint32_t j = 0; j < blockcolumncnt; j++) {
                ret(i, j) = blockdata[index(i, j)];
            }
        }

        return ret;
    }

    void setZero() {
        for (int32_t i = 0; i < rows(); i++)
        {
            memset(&blockdata[index(i, 0)], 0, cols());
        }
    }

private:
    uint8_t *blockdata{NULL};
    uint32_t blockrowcnt{0};
    uint32_t blockcolumncnt{0};
    uint32_t blockstep{0};

    unsigned index(uint32_t row, uint32_t col) {
        return blockstep * row + col;
    }
};

}
}

