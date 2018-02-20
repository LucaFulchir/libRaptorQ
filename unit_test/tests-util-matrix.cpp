#include <RaptorQ/v1/Octet.hpp>
#include <RaptorQ/v1/DenseOctetMatrix.hpp>
#include <RaptorQ/v1/util/Matrix.hpp>
#include <memory>
#include <utility>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include "catch2/catch.hpp"

using namespace RaptorQ__v1::Impl;

TEST_CASE( "Row multiply add", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(2, 4);

    A(0, 0) = 5;
    A(0, 1) = 0;
    A(0, 2) = 1;
    A(0, 3) = 117;
    Matrix::row_multiply_add(A, 1, A, 0, 2);

    REQUIRE( A(1, 0) ==  10 );
    REQUIRE( A(1, 1) ==   0 );
    REQUIRE( A(1, 2) ==   2 );
    REQUIRE( A(1, 3) == 234 );

    Matrix::row_multiply_add(A, 0, A, 0, 2);

    REQUIRE( A(0, 0) == (5 ^ 10) );
    REQUIRE( A(0, 1) == (0 ^ 0) );
    REQUIRE( A(0, 2) == (1 ^ 2) );
    REQUIRE( A(0, 3) == (117 ^ 234) );
}

TEST_CASE( "Row multiply sub", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(2, 4);

    A(0, 0) = 5;
    A(0, 1) = 0;
    A(0, 2) = 1;
    A(0, 3) = 117;
    Matrix::row_multiply_sub(A, 1, A, 0, 2);

    REQUIRE( A(1, 0) ==  10 );
    REQUIRE( A(1, 1) ==   0 );
    REQUIRE( A(1, 2) ==   2 );
    REQUIRE( A(1, 3) == 234 );

    Matrix::row_multiply_sub(A, 0, A, 0, 2);

    REQUIRE( A(0, 0) == (5 ^ 10) );
    REQUIRE( A(0, 1) == (0 ^ 0) );
    REQUIRE( A(0, 2) == (1 ^ 2) );
    REQUIRE( A(0, 3) == (117 ^ 234) );
}

TEST_CASE( "Row div", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(1, 4);

    A(0, 0) = 5;
    A(0, 1) = 0;
    A(0, 2) = 1;
    A(0, 3) = 117;
    Matrix::row_div(A, 0, 0);

    REQUIRE( A(0, 0) ==   5 );
    REQUIRE( A(0, 1) ==   0 );
    REQUIRE( A(0, 2) ==   1 );
    REQUIRE( A(0, 3) == 117 );

    Matrix::row_div(A, 0, 37);

    REQUIRE( A(0, 0) == 19 ); //TODO: verify that this is correct
    REQUIRE( A(0, 1) ==  0 );
    REQUIRE( A(0, 2) == 86 ); //TODO: verify that this is correct
    REQUIRE( A(0, 3) == 44 ); //TODO: verify that this is correct
}

TEST_CASE( "Row swap", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(2, 31);

    A(0, 0)  = 5;
    A(0, 1)  = 0;
    A(0, 2)  = 1;
    A(0, 3)  = 117;
    A(0, 30) = 7;

    A(1, 0)  = 0;
    A(1, 1)  = 3;
    A(1, 2)  = 15;
    A(1, 3)  = 23;
    A(1, 29) = 21;

    Matrix::row_swap(A, 0, 1);

    REQUIRE( A(0, 0)  ==  0 );
    REQUIRE( A(0, 1)  ==  3 );
    REQUIRE( A(0, 2)  == 15 );
    REQUIRE( A(0, 3)  == 23 );
    REQUIRE( A(0, 29) == 21 );

    REQUIRE( A(1, 0)  ==   5 );
    REQUIRE( A(1, 1)  ==   0 );
    REQUIRE( A(1, 2)  ==   1 );
    REQUIRE( A(1, 3)  == 117 );
    REQUIRE( A(1, 30) ==   7 );
}

TEST_CASE( "Col swap", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(2, 31);

    A(0, 0)  = 5;
    A(0, 1)  = 0;
    A(0, 2)  = 1;
    A(0, 3)  = 117;
    A(0, 30) = 7;

    A(1, 0)  = 0;
    A(1, 1)  = 3;
    A(1, 2)  = 15;
    A(1, 3)  = 23;
    A(1, 29) = 21;

    Matrix::col_swap(A, 0, 1);

    REQUIRE( A(0, 0)  ==  0 );
    REQUIRE( A(1, 0)  ==  3 );
    REQUIRE( A(0, 1)  ==  5 );
    REQUIRE( A(1, 1)  ==  0 );

    Matrix::col_swap(A, 3, 7);

    REQUIRE( A(0, 3)  ==   0 );
    REQUIRE( A(1, 3)  ==   0 );
    REQUIRE( A(0, 7)  == 117 );
    REQUIRE( A(1, 7)  ==  23 );
}

TEST_CASE( "Row add", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(2, 31);

    A(0, 0)  = 5;
    A(0, 1)  = 0;
    A(0, 2)  = 1;
    A(0, 3)  = 117;
    A(0, 30) = 7;

    A(1, 0)  = 0;
    A(1, 1)  = 3;
    A(1, 2)  = 15;
    A(1, 3)  = 23;
    A(1, 29) = 21;

    Matrix::row_add(A, 0, A, 1);

    REQUIRE( A(0, 0)  == (5 ^ 0) );
    REQUIRE( A(0, 1)  == (0 ^ 3) );
    REQUIRE( A(0, 2)  == (1 ^ 15) );
    REQUIRE( A(0, 3)  == (117 ^ 23) );
    REQUIRE( A(0, 29) == (0 ^ 21) );
    REQUIRE( A(0, 30) == (7 ^ 0) );
}
