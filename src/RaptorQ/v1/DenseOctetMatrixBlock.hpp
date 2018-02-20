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

    DenseOctetMatrixBlock(uint8_t *data, unsigned int startrow, unsigned int startcolumn,
                          unsigned int rows, unsigned int columns, int stride) {
        blockrowcnt = rows;
        blockcolumncnt = columns;
        blockstep = stride;
        blockdata = data + index(startrow, startcolumn);
    }

    ~DenseOctetMatrixBlock() {
    }

    uint8_t& operator() (unsigned int row, unsigned int col) {
        assert(row < blockrowcnt);
        assert(col < blockcolumncnt);
        return blockdata[index(row, col)];
    }

    int rows() const { return (int)this->blockrowcnt; }
    int cols() const { return (int)this->blockcolumncnt; }

    DenseMtx toEigen() {
        DenseMtx ret = DenseMtx(blockrowcnt, blockcolumncnt);

        for (unsigned int i = 0; i < blockrowcnt; i++) {
            for (unsigned int j = 0; j < blockcolumncnt; j++) {
                ret(i, j) = blockdata[index(i, j)];
            }
        }

        return ret;
    }

    void setZero() {
        for (int i = 0; i < rows(); i++)
        {
            memset(&blockdata[index(i, 0)], 0, cols());
        }
    }

private:
    uint8_t *blockdata{NULL};
    unsigned int blockrowcnt{0};
    unsigned int blockcolumncnt{0};
    unsigned int blockstep{0};

    unsigned index(unsigned int row, unsigned int col) {
        return blockstep * row + col;
    }
};

}
}

