#include <RaptorQ/v1/Octet.hpp>
#include <RaptorQ/v1/DenseOctetMatrix.hpp>
#include <memory>
#include <utility>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include "catch2/catch.hpp"

using namespace RaptorQ__v1::Impl;

TEST_CASE( "Create, set and get dense", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(4, 4);

    // Get when empty
    REQUIRE( A(2, 3) == 0 );

    A(2, 3) = 45; // Set at position
    REQUIRE( A(2, 3) == 45 ); // Get at position
    REQUIRE( A.nonZeros() == 1 );

    A(2, 3) = 15;
    REQUIRE( A(2, 3) == 15 );
    REQUIRE( A.nonZeros() == 1 );

    A(3, 3) = 73;
    REQUIRE( A(3, 3) == 73 );
    REQUIRE( A.nonZeros() == 2 );

    A(2, 0) = 43;
    REQUIRE( A(2, 0) == 43 );
    REQUIRE( A.nonZeros() == 3 );

    A(0, 2) = 241;
    REQUIRE( A(0, 2) == 241 );
    REQUIRE( A.nonZeros() == 4 );

    REQUIRE( A(2, 3) == 15 );
    REQUIRE( A(3, 3) == 73 );
    REQUIRE( A(2, 0) == 43 );
    REQUIRE( A(0, 2) == 241 );

    A(1, 1) = 0;
    A(1, 2) = 0;
    REQUIRE( A.nonZeros() == 4 );
}

TEST_CASE( "Copy dense", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(4, 4);
    REQUIRE( A(2, 3) == 0 );

    A(2, 3) = 45;
    REQUIRE( A(2, 3) == 45 );
    REQUIRE( A.nonZeros() == 1 );

    DenseOctetMatrix B = A;
    REQUIRE( B(2, 3) == 45 );
    REQUIRE( B.nonZeros() == 1 );

    B(2, 3) = 74;
    REQUIRE( B(2, 3) == 74 );
    REQUIRE( A(2, 3) == 45 );
    REQUIRE( A.nonZeros() == 1 );

    DenseOctetMatrix C;
    C = DenseOctetMatrix(B);
    REQUIRE( C(2, 3) == 74 );
    REQUIRE( B(2, 3) == 74 );
    REQUIRE( A(2, 3) == 45 );
    REQUIRE( C.nonZeros() == 1 );

    C(0, 0) = 5;
    REQUIRE( A.nonZeros() == 1 );
    REQUIRE( B.nonZeros() == 1 );
    REQUIRE( C.nonZeros() == 2 );
}

TEST_CASE( "Multiply dense", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(2, 3);
    DenseOctetMatrix B = DenseOctetMatrix(3, 2);

    A(0, 0) = 1;
    A(0, 1) = 2;
    A(0, 2) = 3;
    A(1, 0) = 4;
    A(1, 1) = 5;
    A(1, 2) = 6;

    B(0, 0) = 7;
    B(0, 1) = 8;
    B(1, 0) = 9;
    B(1, 1) = 10;
    B(2, 0) = 11;
    B(2, 1) = 12;

    DenseOctetMatrix C = A * B;


    REQUIRE( C.rows() == 2 );
    REQUIRE( C.cols() == 2 );
    REQUIRE( C(0, 0) == 8 );
    REQUIRE( C(0, 1) == 8 );
    REQUIRE( C(1, 0) == 11 );
    REQUIRE( C(1, 1) == 42 );
    REQUIRE( C.nonZeros() == 4 );


    A = DenseOctetMatrix(3, 3);
    B = DenseOctetMatrix(3, 3);

    A(0, 1) = 2;
    A(0, 2) = 3;
    A(1, 0) = 4;
    A(2, 2) = 5;

    B(0, 2) = 7;
    B(1, 1) = 8;
    B(2, 1) = 9;
    B(2, 2) = 6;

    C = A * B;

    REQUIRE( C.rows() == 3 );
    REQUIRE( C.cols() == 3 );
    REQUIRE( C(0, 1) == 11 );
    REQUIRE( C(0, 2) == 10 );
    REQUIRE( C(1, 2) == 28 );
    REQUIRE( C(2, 1) == 45 );
    REQUIRE( C(2, 2) == 30 );
    REQUIRE( C.nonZeros() == 5 );
}

TEST_CASE( "Eigen conversion and verification", "[Dense Matrix]" )
{
    DenseMtx A = DenseMtx(2, 3);
    DenseMtx B = DenseMtx(3, 2);

    A(0, 0) = 1;
    A(0, 1) = 2;
    A(0, 2) = 3;
    A(1, 0) = 4;
    A(1, 1) = 5;
    A(1, 2) = 6;

    B(0, 0) = 7;
    B(0, 1) = 8;
    B(1, 0) = 9;
    B(1, 1) = 10;
    B(2, 0) = 11;
    B(2, 1) = 12;

    DenseMtx C = A * B;

    DenseOctetMatrix D = DenseOctetMatrix(A);
    DenseOctetMatrix E = DenseOctetMatrix(B);
    DenseOctetMatrix F = D * E;

    REQUIRE( C(0, 0) == F(0, 0) );
    REQUIRE( C(0, 1) == F(0, 1) );
    REQUIRE( C(1, 0) == F(1, 0) );
    REQUIRE( C(1, 1) == F(1, 1) );

    DenseMtx G = F.toEigen();
    REQUIRE( C.isApprox(G) );
}

TEST_CASE( "Eigen multiplication", "[Dense Matrix]" )
{
    DenseMtx A = DenseMtx(10, 10);
    DenseMtx B = DenseMtx(10, 10);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            A(i, j) = (i * j) % 256;
            B(i, j) = (3 * i * j) % 256;
        }
    }

    DenseMtx C = A * B;

    DenseOctetMatrix D = DenseOctetMatrix(A);
    DenseOctetMatrix E = DenseOctetMatrix(B);
    DenseOctetMatrix F = D * E;
    REQUIRE( C.isApprox(F.toEigen()) );
}

TEST_CASE( "Eigen top left", "[Dense Matrix]" )
{
    DenseMtx A = DenseMtx(10, 10);
    DenseMtx B = DenseMtx(10, 10);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            A(i, j) = ((i + 12) * (j + 7)) % 256;
            B(i, j) = (3 * i + 2 * j) % 256;
        }
    }

    DenseMtx C = A * B;
    auto Cb = C.block(0, 0, 5, 5);
    Cb = A.block(0, 0, 5, 5) * B.block(0, 0, 5, 5);

    DenseOctetMatrix D = DenseOctetMatrix(A);
    DenseOctetMatrix E = DenseOctetMatrix(B);
    DenseOctetMatrix F = D * E;

    SparseMtx Sd = D.topLeftSparseView(5, 5);
    SparseMtx Se = E.topLeftSparseView(5, 5);

    SparseMtx Sf = Sd * Se;

    REQUIRE( !C.isApprox(F.toEigen()) );
    F.valuesFromEigenTopLeft(Sf);

    REQUIRE( Cb.isApprox(DenseMtx(Sf)) );
    REQUIRE( C.isApprox(F.toEigen()) );
}

TEST_CASE( "Eigen block", "[Dense Matrix]" )
{
    // Basic test comparing Eigen to DenseOctetMatrix
    const int org_w = 15, org_h = 15;

    const int new_w = 5,  new_h = 5;
    const int x0    = 3,  y0    = 4;

    DenseMtx A = DenseMtx(org_w, org_w);
    for (int i = 0; i < org_w; i++) {
        for (int j = 0; j < org_h; j++) {
            A(i, j) = ((i + 12) * (j + 7)) % 256;
        }
    }

    DenseOctetMatrix B = DenseOctetMatrix(A);

    auto Ab = A.block(x0, y0, new_w, new_h);
    auto Bb = B.block(x0, y0, new_w, new_h);

    REQUIRE( Ab.isApprox(Bb.toEigen()) );

}

#if 0
TEST_CASE( "Eigen block 2", "[Dense Matrix]" )
{
    DenseMtx A = DenseMtx(10, 10);
    DenseMtx B = DenseMtx(10, 10);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            A(i, j) = ((i + 12) * (j + 7)) % 256;
            B(i, j) = (3 * i + 2 * j) % 256;
        }
    }

    DenseMtx C = A * B;
    auto Cb = C.block(0, 0, 5, 5);
    Cb = A.block(0, 0, 5, 5) * B.block(0, 0, 5, 5);

    DenseOctetMatrix D = DenseOctetMatrix(A);
    DenseOctetMatrix E = DenseOctetMatrix(B);
    DenseOctetMatrix F = D * E;

    REQUIRE( !C.isApprox(F.toEigen()) );

    auto Fb = F.block(0,0,5,5);
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            Fb(i, j) = Cb(i, j);
        }
    }

    REQUIRE( Cb.isApprox(DenseMtx(Fb)) );
    REQUIRE( C.isApprox(F.toEigen()) );
}
#endif

TEST_CASE( "SetZero block matrix", "[Dense Matrix]" )
{
    const int org_w = 15, org_h = 15;

    const int new_w = 5,  new_h = 5;
    const int x0    = 3,  y0    = 4;

    DenseMtx A = DenseMtx(org_w, org_w);
    for (int i = 0; i < org_w; i++) {
        for (int j = 0; j < org_h; j++) {
            A(i, j) = ((i + 12) * (j + 7)) % 255 + 1;
        }
    }

    DenseOctetMatrix B = DenseOctetMatrix(A);

    REQUIRE( A == B.toEigen() );
    auto Ab = A.block(x0, y0, new_w, new_h);
    auto Bb = B.block(x0, y0, new_w, new_h);

    for (int i = x0; i < new_w; i++) {
        for (int j = x0; j < new_h; j++) {
            REQUIRE( A(i, j) != 0 );
            REQUIRE( B(i, j) != 0 );
        }
    }

    Ab.setZero();
    Bb.setZero();

    for (int i = 0; i < new_w; i++) {
        for (int j = 0; j < new_h; j++) {
            REQUIRE( Ab(i, j) == 0 );
            REQUIRE( Bb(i, j) == 0 );
        }
    }

    for (int i = x0; i < x0 + new_w; i++) {
        for (int j = y0; j < y0 + new_h; j++) {
            REQUIRE( A(i, j) == 0 );
            REQUIRE( B(i, j) == 0 );
        }
    }

    REQUIRE( A == B.toEigen() );
}

TEST_CASE( "Copy operations", "[Dense Matrix]" )
{
    DenseMtx A = DenseMtx(10, 10);
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            A(i, j) = ((i + 12) * (j + 7)) % 255 + 1;
        }
    }

    DenseOctetMatrix B = DenseOctetMatrix(A);

    REQUIRE( A == B.toEigen() );

    DenseOctetMatrix C = DenseOctetMatrix(10, 10);
    DenseOctetMatrix D = DenseOctetMatrix(10, 10);
    DenseMtx F = DenseMtx(10, 10);
    F.setZero();

    C.valuesFromEigen(A, 1, 2, 5, 4);
    D.valuesFromMatrix(B, 1, 2, 5, 4);
    F.block(1, 2, 5, 4) = A.block(0, 0, 5, 4);

    REQUIRE( A != C.toEigen() );
    REQUIRE( F == C.toEigen() );
    REQUIRE( C == D );

    C.valuesFromEigen(A, 3, 1, 6, 8);
    D.valuesFromMatrix(B, 3, 1, 6, 8);
    F.block(3, 1, 6, 8) = A.block(0, 0, 6, 8);

    REQUIRE( A != C.toEigen() );
    REQUIRE( F == C.toEigen() );
    REQUIRE( C == D );

    C.valuesFromEigen(A, 0, 0, 6, 8);
    D.valuesFromMatrix(B, 0, 0, 6, 8);
    F.block(0, 0, 6, 8) = A.block(0, 0, 6, 8);
    C.valuesFromEigen(A, 4, 4, 6, 6);
    D.valuesFromMatrix(B, 4, 4, 6, 6);
    F.block(4, 4, 6, 6) = A.block(0, 0, 6, 6);

    REQUIRE( A != C.toEigen() );
    REQUIRE( F == C.toEigen() );
    REQUIRE( C == D );

    C.valuesFromEigen(A, 0, 4, 10, 6);
    D.valuesFromMatrix(B, 0, 4, 10, 6);
    F.block(0, 4, 10, 6) = A.block(0, 0, 10, 6);
    C.valuesFromEigen(A, 4, 0, 6, 10);
    D.valuesFromMatrix(B, 4, 0, 6, 10);
    F.block(4, 0, 6, 10) = A.block(0, 0, 6, 10);

    REQUIRE( A != C.toEigen() );
    REQUIRE( F == C.toEigen() );
    REQUIRE( C == D );

    C.valuesFromEigen(A, 0, 0, 10, 10);
    D.valuesFromMatrix(B, 0, 0, 10, 10);
    F.block(0, 0, 10, 10) = A.block(0, 0, 10, 10);

    REQUIRE( A == C.toEigen() );
    REQUIRE( F == C.toEigen() );
    REQUIRE( B == C );
    REQUIRE( C == D );
}

TEST_CASE( "Value to/from Eigen", "[Dense Matrix]" )
{
    DenseMtx A = DenseMtx(10, 10);
    DenseMtx B = DenseMtx(10, 10);
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            A(i, j) = ((i + 12) * (j + 7)) % 255 + 1;
            B(i, j) = ((i + 7) * (j + 13)) % 255 + 1;
        }
    }

    DenseOctetMatrix C = DenseOctetMatrix(A);

    A.block (2, 3, 4, 5) = B.block (4, 2, 4, 5);
    REQUIRE( A != C.toEigen() );

    C.valuesFromEigen(B.block (4, 2, 4, 5), 2, 3, 4, 5);
    REQUIRE( A == C.toEigen() );

    A.block (0, 0, 5, 7) = B.block (0, 0, 5, 7);
    REQUIRE( A != C.toEigen() );

    C.valuesFromEigen(B, 0, 0, 5, 7);
    REQUIRE( A == C.toEigen() );

    REQUIRE( C.toEigen(0, 0, 5, 7) == A.block (0, 0, 5, 7) );
    REQUIRE( C.toEigen(1, 0, 5, 7) == A.block (1, 0, 5, 7) );
    REQUIRE( C.toEigen(0, 2, 5, 7) == A.block (0, 2, 5, 7) );
    REQUIRE( C.toEigen(4, 6, 3, 2) == A.block (4, 6, 3, 2) );
}


TEST_CASE( "Copy", "[Dense Matrix]" )
{
    DenseOctetMatrix A = DenseOctetMatrix(10, 10);
    DenseOctetMatrix B = DenseOctetMatrix(10, 10);
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            A(i, j) = ((i + 12) * (j + 7)) % 255 + 1;
        }
    }

    REQUIRE( A.toEigen() != B.toEigen() );
    B = A;
    REQUIRE( A.toEigen() == B.toEigen() );
    B(0, 0) += 1;
    REQUIRE( A.toEigen() != B.toEigen() );
}
