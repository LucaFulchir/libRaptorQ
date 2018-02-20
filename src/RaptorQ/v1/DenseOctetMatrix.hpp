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
#include "RaptorQ/v1/multiplication.hpp"
#include "RaptorQ/v1/DenseOctetMatrixBlock.hpp"
#include <cmath>
#include <vector>
#include <stdlib.h>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace RaptorQ__v1 {
namespace Impl {

using DenseMtx = Eigen::Matrix<Octet, Eigen::Dynamic, Eigen::Dynamic,
                                                            Eigen::RowMajor>;
using SparseMtx = Eigen::SparseMatrix<Octet, Eigen::RowMajor>;

// Align memory access for sse3/avx2 instuctions
const unsigned int RowAlignment = 32;

class RAPTORQ_LOCAL DenseOctetMatrix {
    friend class Matrix;

public:
    DenseOctetMatrix() {};

    DenseOctetMatrix(const DenseMtx& mtx) {
        init(mtx.rows(), mtx.cols());
        for (unsigned int i = 0; i < rowcnt; i++) {
            for (unsigned int j = 0; j < columncnt; j++) {
                data[i * stride + j] = (uint8_t)mtx(i, j);
            }
        }
    }

    DenseOctetMatrix(const DenseOctetMatrix& rhs) {
        init(rhs.rowcnt, rhs.columncnt);
        memcpy(data, rhs.data, rhs.rowcnt * rhs.stride * sizeof(uint8_t));
    }

    DenseOctetMatrix(DenseOctetMatrix& rhs) {
        init(rhs.rowcnt, rhs.columncnt);
        memcpy(data, rhs.data, rhs.rowcnt * rhs.stride * sizeof(uint8_t));
    }

    DenseOctetMatrix& operator= (const DenseOctetMatrix &rhs) {
        if (rhs.rowcnt != rowcnt || rhs.columncnt != columncnt) {
            free(data);
            init(rhs.rowcnt, rhs.columncnt);
        }

        memcpy(data, rhs.data, rhs.rowcnt * rhs.stride * sizeof(uint8_t));
        return *this;
    };

    void setIdentity ()
    {
        assert( cols() == rows() );
        memset(data, 0, rows() * stride);
        for (int i = 0; i < rows(); i++) {
            data[i * stride + i] = 1;
        }
    }

    DenseMtx toEigen() {
        DenseMtx ret = DenseMtx(rows(), cols());
        for (int i = 0; i < rows(); i++) {
            memcpy(ret.data() + i * cols(), data + i * stride, cols() * sizeof(uint8_t));
        }
        return ret;
    }

    DenseOctetMatrixBlock block(unsigned int startrow, unsigned int startcol, 
                                unsigned int blockrows, unsigned int blockcols) {

        DenseOctetMatrixBlock ret = DenseOctetMatrixBlock(data, startrow, startcol, blockrows, blockcols, stride);
        return ret;
    }

    DenseMtx toEigen(int row, int col, int rows, int cols) {
        uint8_t *dest, *src;
        DenseMtx ret = DenseMtx(rows, cols);
        for (int i = row; i < row + rows; i++) {
            src = data + i * stride + col;
            dest = (uint8_t *)ret.data() + (i - row) * cols;
            memcpy(dest, src, cols * sizeof(uint8_t));
        }
        return ret;
    }

    void valuesFromEigen(const DenseMtx& mtx, int row, int col, int rows, int cols) {
        uint8_t *dest, *src;
        for (int i = row; i < row + rows; i++) {
            dest = data + i * stride + col;
            src = (uint8_t *)mtx.data() + (i - row) * mtx.cols();
            memcpy(dest, src, cols * sizeof(uint8_t));
        }
    }

    void valuesFromMatrix(const DenseOctetMatrix& mtx, int row, int col, int rows, int cols) {
        uint8_t *dest, *src;
        for (int i = row; i < row + rows; i++) {
            dest = data + i * stride + col;
            src = mtx.data + (i - row) * mtx.stride;
            memcpy(dest, src, cols * sizeof(uint8_t));
        }
    }

    void valuesFromEigen(const DenseMtx& mtx) {
        assert( mtx.rows() == rows() );
        assert( mtx.cols() == cols() );
        for (int i = 0; i < rows(); i++) {
            memcpy(data + i * stride, mtx.data() + i * cols(), cols() * sizeof(uint8_t));
        }
    }

    void valuesFromEigenTopLeft(const SparseMtx& mtx) {
        for (int i = 0; i < mtx.rows(); i++) {
            memset(data + i * stride, 0, mtx.cols());
        }
        for (int k=0; k<mtx.outerSize(); ++k) {
            for (SparseMtx::InnerIterator it(mtx,k); it; ++it)
            {
                data[it.row() * stride + it.col()] = (uint8_t)it.value();
            }
        }
    }

    SparseMtx topLeftSparseView(int rows, int cols) const{
        std::vector<Eigen::Triplet<Octet>> tripletList;
        tripletList.reserve((rows * cols) / 10);

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (data[i * stride + j] != 0) {
                    tripletList.emplace_back(Eigen::Triplet<Octet>(i, j, Octet(data[i * stride + j])));
                }
            }
        }
        SparseMtx ret = SparseMtx(rows, cols);
        ret.setFromTriplets(tripletList.begin(), tripletList.end());
        return ret;
    }

    DenseOctetMatrix& operator= (DenseOctetMatrix &rhs) {
        if (rhs.rowcnt != rowcnt || rhs.columncnt != columncnt) {
            free(data);
            init(rhs.rowcnt, rhs.columncnt);
        }

        memcpy(data, rhs.data, rhs.rowcnt * rhs.stride * sizeof(uint8_t));
        return *this;
    };

    DenseOctetMatrix(unsigned int m, unsigned int n) {
        init(m, n);
    }

    ~DenseOctetMatrix() {
        free(data);
    }

    // Other methods
    int rows() const { return (int)this->rowcnt; }
    int cols() const { return (int)this->columncnt; }

    uint8_t& operator()(int row, int col)
    {
        assert((int)rowcnt >= row);
        assert((int)columncnt >= col);
        return data[row * stride + col];
    }

    uint8_t operator()(int row, int col) const
    {
        assert((int)rowcnt >= row);
        assert((int)columncnt >= col);
        return data[row * stride + col];
    }

    bool operator== (const DenseOctetMatrix mtx) const
    {
        if (rows() != mtx.rows() || cols() != mtx.cols()) {
            return false;
        }
        for (int i = 0; i < rows(); i++) {
            if (memcmp(data + i * stride, mtx.data + i * mtx.stride, cols()) != 0) {
                return false;
            }
        }
        return true;
    }

    // Multiplication operators
    // TODO: Avoid copy off return value, how?
    friend DenseOctetMatrix operator* (const DenseOctetMatrix &lhs, const DenseOctetMatrix &rhs)
    {
        assert(lhs.columncnt == rhs.rowcnt);

        int m = lhs.rowcnt;
        int n = lhs.columncnt;
        int p = rhs.columncnt;

        DenseOctetMatrix res = DenseOctetMatrix(m, p);

        uint16_t *a = lhs.getCopy(); // No stride
        uint16_t *b = rhs.getTransposed(); // No stride
        uint8_t *c = res.data; // Stride

        /* Pre calculate oct_log for a */
        for (int i = 0; i < m * n;i++) {
            a[i] = oct_log_no_if[a[i]];
        }

        /* Pre calculate oct_log for b */
        for (int i = 0; i < n * p;i++) {
            b[i] = oct_log_no_if[b[i]];
        }

        for (int i = 0; i < m; i++) {
            for (int j = 0; j < p; j++) {
                uint8_t dot = 0;
                for (int k = 0; k < n; k++) {
                    dot = dot ^ oct_exp_no_if[a[i * n + k] + b[j * n + k]];
                }
                c[i * res.stride + j] = dot;
            }
        }

        free(a);
        free(b);
        return res;
    }

    unsigned int nonZeros() {
        unsigned int m = rowcnt;
        unsigned int n = columncnt;
        unsigned int nonzerocnt = 0;
        
        for (unsigned int i = 0; i < m; i++) {
            for (unsigned int j = 0; j < n; j++) {
                if (data[i * stride + j] != 0)
                    nonzerocnt++;
            }
        }

        return nonzerocnt;
    }

private:
    uint8_t *data {NULL};

    unsigned int rowcnt {0};
    unsigned int columncnt {0};
    unsigned int stride {0};

    void init (unsigned int row, unsigned int col) {
        unsigned int stride = ((col + RowAlignment-1) / RowAlignment) * RowAlignment;
        // The size is one extra row to use as temp during row_swap
        const size_t size = (row + 1) * stride * sizeof(uint8_t);

        uint8_t *data = NULL;
        if (!posix_memalign((void **) &data, RowAlignment, size)) {
            memset(data, 0, size);

            this->stride = stride;
            this->data = data;
            this->rowcnt = row;
            this->columncnt = col;
        }
    }

    /**
     * Get a transposed copy of the Matrix without stride, will need stride if
     * SSE is added.
     *
     * @return pointer to a transposed copy of the Matrix
     */
    uint16_t *getTransposed() const
    {
        uint16_t *copy = (uint16_t*)calloc(rowcnt * columncnt, sizeof(uint16_t));

        for (unsigned int i = 0; i < rowcnt; i ++ )
        {
            for (unsigned int j = 0; j < columncnt; j ++ )
            {
                copy[j * rowcnt + i] = data[i * stride + j];
            }
        }
        return copy;
    }

    /**
     * Get a copy of the Matrix without stride, will need stride if SSE is
     * added.
     *
     * @return pointer to a copy of the Matrix
     */
    uint16_t *getCopy() const
    {
        uint16_t *copy = (uint16_t*)calloc(rowcnt * columncnt, sizeof(uint16_t));

        for (unsigned int i = 0; i < rowcnt; i ++ )
        {
            for (unsigned int j = 0; j < columncnt; j ++ )
            {
                copy[i * columncnt + j] = data[i * stride + j];
            }
        }
        return copy;
    }
};
}
}
