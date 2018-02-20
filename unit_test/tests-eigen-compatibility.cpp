#include <RaptorQ/v1/Octet.hpp>
#include <RaptorQ/v1/util/Matrix.hpp>
#include <RaptorQ/v1/DenseOctetMatrix.hpp>
#include <memory>
#include <utility>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <stdlib.h>
#include <ctime>
#include "catch2/catch.hpp"

using namespace RaptorQ__v1::Impl;

TEST_CASE( "Test eigen compatibility", "[Eigen compatibility]" )
{
    std::time_t result = std::time(nullptr);
    std::cout << "Random seed used in [Eigen compatibility]: " << (int)result << std::endl;
    srand( result );

    uint8_t number;
    DenseMtx original = DenseMtx(10, 1316);
    for ( int i = 0; i < original.rows(); i ++ )  {
        for ( int j = 0; j < original.cols(); j ++ ) {
            original(i,j) = static_cast<uint8_t> (rand() % 256);
        }
    }

    // Test for all types
    const Matrix::SIMD simd_types[] = { Matrix::SIMD::NONE, Matrix::SIMD::AUTO, Matrix::SIMD::SSSE3, Matrix::SIMD::AVX2 };
    for (int type = 0;type < 3;type++) {
        // Set SIMD type: in production use "Matrix::SIMD::AUTO"
        Matrix::get_instance().init_simd(simd_types[type]);
        // Test all numbers
        for (int i = 0;i < 256;i++) {
            uint8_t number = i;
            DenseMtx A = DenseMtx(original);
            DenseOctetMatrix B = DenseOctetMatrix(original);
            REQUIRE( A.isApprox(B.toEigen()) );

            // row_multiply_add
            A.row (0) += A.row (1) * Octet(number);
            Matrix::get_instance().row_multiply_add(B, 0, B, 1, number);
            REQUIRE( A.isApprox(B.toEigen()) );

            // row_multiply_sub
            A.row (1) -= A.row (2) * Octet(number);
            Matrix::get_instance().row_multiply_sub(B, 1, B, 2, number);
            REQUIRE( A.isApprox(B.toEigen()) );

            // row_div
            A.row (3) /= Octet(number);
            Matrix::get_instance().row_div(B, 3, number);
            REQUIRE( A.isApprox(B.toEigen()) );

            // row_add
            A.row (4) += A.row (5);
            Matrix::get_instance().row_add(B, 4, B, 5);
            REQUIRE( A.isApprox(B.toEigen()) );

            // row_swap
            A.row (4).swap (A.row (5));
            Matrix::get_instance().row_swap(B, 4, 5);
            A.row (9).swap (A.row (3));
            Matrix::get_instance().row_swap(B, 9, 3);
            A.row (2).swap (A.row (0));
            Matrix::get_instance().row_swap(B, 2, 0);
            REQUIRE( A.isApprox(B.toEigen()) );

            // col_swap
            A.col (4).swap (A.col (5));
            Matrix::get_instance().col_swap(B, 4, 5);
            A.col (78).swap (A.col (1311));
            Matrix::get_instance().col_swap(B, 78, 1311);
            A.col (0).swap (A.col (13));
            Matrix::get_instance().col_swap(B, 0, 13);
            REQUIRE( A.isApprox(B.toEigen()) );

            // row_assign
            A.row (5) = A.row (6);
            Matrix::get_instance().row_assign(B, 5, B, 6);
            REQUIRE( A.isApprox(B.toEigen()) );
        }
    }
}
