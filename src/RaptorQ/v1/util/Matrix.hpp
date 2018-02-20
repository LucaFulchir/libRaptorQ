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
#include "RaptorQ/v1/DenseOctetMatrix.hpp"
#include "RaptorQ/v1/util/Matrix_SSSE3.hpp"
#include "RaptorQ/v1/util/Matrix_AVX2.hpp"
#include "RaptorQ/v1/util/CPU_Info.hpp"
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <fstream>

namespace RaptorQ__v1 {
namespace Impl {

typedef void (*ADD_FPTR)(uint8_t *dest, uint8_t *src, int bytes);
typedef void (*DIV_FPTR)(uint8_t *data, uint8_t num, int bytes);
typedef void (*ADD_MUL_FPTR)(uint8_t *dest, uint8_t *src, uint8_t num, int bytes);

class RAPTORQ_LOCAL Matrix
{
public:
    enum class SIMD : uint8_t {
        NONE = 0x00,
        AUTO = 0x01,
        SSSE3 = 0x02,
        AVX2 = 0x03,
    };

    static Matrix & get_instance() {
        static Matrix matrix;
        return matrix;
    }

    static void init_simd(SIMD type) {
        Matrix::get_instance()._init_simd(type);
    }

    static void print_matrix(DenseOctetMatrix &mtx) {
        for (int i = 0; i < mtx.rows(); i++) {
            for (int j = 0; j < mtx.cols(); j++) {
                    std::cout << (int)mtx.data[i * mtx.stride + j] << ", ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    /**
     * Write Matrix to file in format "rows cols v0, v1, v2, ..." in row-major
     * @param fileName path to output file
     * @param A The Matrix
     */
    static void write_matrix_to_file (std::string fileName, const DenseOctetMatrix &mtx)
    {
        std::ofstream file(fileName);
        file << mtx.rows() << ' ' << mtx.cols();
        if (file.is_open())
        {
            for (int i = 0; i < mtx.rows(); i++) {
                for (int j = 0; j < mtx.cols(); j++) {
                    file << ' ' << mtx(i, j);
                }
            }
        }
        file << '\n';
    }

    static void row_multiply_add(DenseOctetMatrix &lhs, uint16_t lhsRow, DenseOctetMatrix &rhs, uint16_t rhsRow, uint8_t scalar)
    {
        assert(lhs.stride == rhs.stride);
        uint8_t *dest = lhs.data + lhsRow * lhs.stride;
        uint8_t *src = rhs.data + rhsRow * rhs.stride;
        if (scalar == 1) {
            Matrix::get_instance().add_cb(dest, src, lhs.stride);
        } else {
            Matrix::get_instance().add_mul_cb(dest, src, scalar, lhs.stride);
        }
    }

    static void row_div(DenseOctetMatrix &mtx, uint16_t row, uint8_t scalar)
    {
        if (scalar == 0) {
            return;
        }
        uint8_t *data = mtx.data + row * mtx.stride;
        Matrix::get_instance().div_cb(data, scalar, mtx.stride);
    }

    static void row_add(DenseOctetMatrix &lhs, uint16_t lhsRow, const DenseOctetMatrix &rhs, uint16_t rhsRow)
    {
        uint8_t *dest = lhs.data + lhsRow * lhs.stride;
        uint8_t *src = rhs.data + rhsRow * rhs.stride;
        Matrix::get_instance().add_cb(dest, src, lhs.stride);
    }

    static void row_multiply_sub(DenseOctetMatrix &lhs, uint16_t lhsRow, DenseOctetMatrix &rhs, uint16_t rhsRow, uint8_t scalar)
    {
        row_multiply_add(lhs, lhsRow, rhs, rhsRow, scalar);
    }

    static void row_swap(DenseOctetMatrix &mtx, uint16_t row1, uint16_t row2) {
        if (row1 == row2)
            return;
        uint8_t *temp = mtx.data + mtx.rows() * mtx.stride;
        uint8_t *r1 = mtx.data + row1 * mtx.stride;
        uint8_t *r2 = mtx.data + row2 * mtx.stride;
        memcpy(temp, r1,   mtx.stride);
        memcpy(r1,   r2,   mtx.stride);
        memcpy(r2,   temp, mtx.stride);
    }

    static void col_swap(DenseOctetMatrix &mtx, uint16_t col1, uint16_t col2) {
        if (col1 == col2)
            return;

        uint8_t temp;
        for (int row = 0; row < mtx.rows(); row++) {
            if (mtx.data[row * mtx.stride + col1] != 0 || mtx.data[row * mtx.stride + col2] != 0) {
                temp = mtx.data[row * mtx.stride + col1];
                mtx.data[row * mtx.stride + col1] = mtx.data[row * mtx.stride + col2];
                mtx.data[row * mtx.stride + col2] = temp;
            }
        }
    }

    static void inline row_assign(DenseOctetMatrix &lhs, uint16_t lhsRow, const DenseOctetMatrix &rhs, uint16_t rhsRow)
    {
        assert(lhs.cols() == rhs.cols());
        uint8_t *dest = lhs.data + lhsRow * lhs.stride;
        uint8_t *src = rhs.data + rhsRow * rhs.stride;
        memcpy(dest, src, lhs.cols());
    }

private:
    ADD_FPTR add_cb;
    DIV_FPTR div_cb;
    ADD_MUL_FPTR add_mul_cb;

    Matrix() {
        _init_simd(SIMD::AUTO);
    }

    /* Ordered from most wanted to least wanted */
    SIMD get_simd_type()
    {
        if (CPU_Info::has_avx2()) {
            return SIMD::AVX2;
        }
        if (CPU_Info::has_ssse3()) {
            return SIMD::SSSE3;
        }
        return SIMD::NONE;
    }

    void _init_simd(SIMD type) {
        if (type == SIMD::AUTO) {
            type = get_simd_type();
        }

        switch( type )
        {
        case SIMD::SSSE3:
            add_cb = &Matrix_SSSE3::add;
            div_cb = &Matrix_SSSE3::div;
            add_mul_cb = &Matrix_SSSE3::multiply_and_add;
            break;
        case SIMD::AVX2:
            add_cb = &Matrix_AVX2::add;
            div_cb = &Matrix_AVX2::div;
            add_mul_cb = &Matrix_AVX2::multiply_and_add;
            break;
        default:
            add_cb = &add;
            div_cb = &div;
            add_mul_cb = &multiply_and_add;
            break;
        }
    }

    static void add(uint8_t *dest, uint8_t *src, int bytes)
    {
        for (int i = 0; i < bytes;i++) {
            dest[i] = dest[i] ^ src[i];
        }
    }

    static void div(uint8_t *data, uint8_t num, int bytes)
    {
        num = oct_log[num - 1];
        for (int i = 0; i < bytes;i++) {
            if (data[i] != 0) {
                data[i] = oct_exp[oct_log[data[i] - 1] - num + 255];
            }
        }
    }

    /* The SIMD versions are: dest[i] ^= (high[src[i]>>4] ^ low[src[i]&0xf]);
     * high and low are 16 byte lookup tables (high four bits and low four bits)
     */
    static void multiply_and_add(uint8_t *dest, uint8_t *src, uint8_t num, int bytes)
    {
        if (num == 0) {
            return;
        }
        /* TODO: Probably faster with a single lookup based on num, a single
         * lookup require an additional lookup table */
        uint16_t log_num = oct_log_no_if[num];
        for (int i = 0; i < bytes;i++) {
            dest[i] = dest[i] ^ oct_exp_no_if[oct_log_no_if[src[i]] + log_num];
        }
    }
};
} // namespace Impl
} // namespace RaptorQ__v1
