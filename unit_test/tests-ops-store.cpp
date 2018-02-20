#include <RaptorQ/v1/Octet.hpp>
#include <RaptorQ/v1/DenseOctetMatrix.hpp>
#include <RaptorQ/v1/util/Matrix.hpp>
#include "RaptorQ/v1/Shared_Computation/Decaying_LF.hpp"
#include <memory>
#include <utility>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include "catch2/catch.hpp"

using namespace RaptorQ__v1::Impl;

TEST_CASE( "SWAP operation", "[Operations store]" )
{
    std::deque<Operation> ops;
    ops.emplace_back (Operation::_t::SWAP, 34567, 12);
    std::vector<uint8_t> raw = ops_to_raw(ops);
    REQUIRE( raw[0] == (uint8_t)Operation::_t::SWAP );
    REQUIRE( raw[0] != (uint8_t)Operation::_t::NONE );

    uint16_t number = raw[1] + (raw[2] << 8);
    REQUIRE( number == 34567 );

    number = raw[3] + (raw[4] << 8);
    REQUIRE( number == 12 );

    std::deque<Operation> new_ops = raw_to_ops(raw);
    std::vector<uint8_t> new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );

    ops.emplace_back (Operation::_t::SWAP, 0, 1);
    ops.emplace_back (Operation::_t::SWAP, 367, 512);
    raw = ops_to_raw(ops);
    REQUIRE( raw[0]  == (uint8_t)Operation::_t::SWAP );
    REQUIRE( raw[5]  == (uint8_t)Operation::_t::SWAP );
    REQUIRE( raw[10] == (uint8_t)Operation::_t::SWAP );

    REQUIRE( 0   == (raw[6]  + (raw[7]  << 8)) );
    REQUIRE( 1   == (raw[8]  + (raw[9]  << 8)) );
    REQUIRE( 367 == (raw[11] + (raw[12] << 8)) );
    REQUIRE( 512 == (raw[13] + (raw[14] << 8)) );

    new_ops = raw_to_ops(raw);
    new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );
}

TEST_CASE( "ADD_MUL operation", "[Operations store]" )
{
    std::deque<Operation> ops;
    ops.emplace_back (Operation::_t::ADD_MUL, 34567, 634, Octet(12));
    std::vector<uint8_t> raw = ops_to_raw(ops);
    REQUIRE( raw[0] == (uint8_t)Operation::_t::ADD_MUL );
    REQUIRE( raw[0] != (uint8_t)Operation::_t::NONE );

    uint16_t number = raw[1] + (raw[2] << 8);
    REQUIRE( number == 34567 );

    number = raw[3] + (raw[4] << 8);
    REQUIRE( number == 634 );

    number = raw[5];
    REQUIRE( number == 12 );

    std::deque<Operation> new_ops = raw_to_ops(raw);
    std::vector<uint8_t> new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );

    ops.emplace_back (Operation::_t::ADD_MUL, 0, 1, Octet(0));
    ops.emplace_back (Operation::_t::ADD_MUL, 367, 512, Octet(37));
    raw = ops_to_raw(ops);
    REQUIRE( raw[0]  == (uint8_t)Operation::_t::ADD_MUL );
    REQUIRE( raw[6]  == (uint8_t)Operation::_t::ADD_MUL );
    REQUIRE( raw[12] == (uint8_t)Operation::_t::ADD_MUL );

    REQUIRE( 0   == (raw[7]  + (raw[8]  << 8)) );
    REQUIRE( 1   == (raw[9]  + (raw[10]  << 8)) );
    REQUIRE( 0   == raw[11] );
    REQUIRE( 367 == (raw[13] + (raw[14] << 8)) );
    REQUIRE( 512 == (raw[15] + (raw[16] << 8)) );
    REQUIRE( 37   == raw[17] );

    new_ops = raw_to_ops(raw);
    new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );
}

TEST_CASE( "DIV operation", "[Operations store]" )
{
    std::deque<Operation> ops;
    ops.emplace_back (Operation::_t::DIV, 34567, Octet(12));
    std::vector<uint8_t> raw = ops_to_raw(ops);
    REQUIRE( raw[0] == (uint8_t)Operation::_t::DIV );
    REQUIRE( raw[0] != (uint8_t)Operation::_t::NONE );

    uint16_t number = raw[1] + (raw[2] << 8);
    REQUIRE( number == 34567 );

    REQUIRE( 12 == raw[3] );

    std::deque<Operation> new_ops = raw_to_ops(raw);
    std::vector<uint8_t> new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );

    ops.emplace_back (Operation::_t::DIV, 0, Octet(1));
    ops.emplace_back (Operation::_t::DIV, 367, Octet(255));
    raw = ops_to_raw(ops);
    REQUIRE( raw[0]  == (uint8_t)Operation::_t::DIV );
    REQUIRE( raw[4]  == (uint8_t)Operation::_t::DIV );
    REQUIRE( raw[8] == (uint8_t)Operation::_t::DIV );

    REQUIRE( 0   == (raw[5]  + (raw[6]  << 8)) );
    REQUIRE( 1   == raw[7] );
    REQUIRE( 367 == (raw[9] + (raw[10] << 8)) );
    REQUIRE( 255 == raw[11] );

    new_ops = raw_to_ops(raw);
    new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );
}

TEST_CASE( "BLOCK operation 1x1", "[Operations store]" )
{
    SparseMtx S = SparseMtx(1, 1);
    S.insert(0, 0) = 34;
    std::deque<Operation> ops;
    ops.emplace_back (Operation::_t::BLOCK, S);
    std::vector<uint8_t> raw = ops_to_raw(ops);
    REQUIRE( raw[0] == (uint8_t)Operation::_t::BLOCK );
    REQUIRE( raw[0] != (uint8_t)Operation::_t::NONE );

    uint16_t number = raw[1] + (raw[2] << 8);
    REQUIRE( number == 1 );

    number = raw[3] + (raw[4] << 8);
    REQUIRE( number == 1 );

    uint32_t nnz = raw[5] + (raw[6] << 8) + (raw[7] << 16) + (raw[8] << 24);
    REQUIRE( nnz == 1 );

    REQUIRE( 0   == (raw[9]  + (raw[10]  << 8)) );
    REQUIRE( 0   == (raw[11] + (raw[12]  << 8)) );
    REQUIRE( 34  == raw[13] );

    std::deque<Operation> new_ops = raw_to_ops(raw);
    std::vector<uint8_t> new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );
}

TEST_CASE( "BLOCK operation 2x3", "[Operations store]" )
{
    SparseMtx S = SparseMtx(2, 3);
    S.insert(1, 0) = 54;
    S.insert(1, 2) = 70;
    std::deque<Operation> ops;
    ops.emplace_back (Operation::_t::BLOCK, S);
    std::vector<uint8_t> raw = ops_to_raw(ops);
    REQUIRE( raw[0] == (uint8_t)Operation::_t::BLOCK );
    REQUIRE( raw[0] != (uint8_t)Operation::_t::NONE );

    uint16_t number = raw[1] + (raw[2] << 8);
    REQUIRE( number == 2 );

    number = raw[3] + (raw[4] << 8);
    REQUIRE( number == 3 );

    uint32_t nnz = raw[5] + (raw[6] << 8) + (raw[7] << 16) + (raw[8] << 24);
    REQUIRE( nnz == 2 );

    REQUIRE( 1   == (raw[9]  + (raw[10]  << 8)) );
    REQUIRE( 0   == (raw[11] + (raw[12]  << 8)) );
    REQUIRE( 54  == raw[13] );

    REQUIRE( 1   == (raw[14]  + (raw[15]  << 8)) );
    REQUIRE( 2   == (raw[16] + (raw[17]  << 8)) );
    REQUIRE( 70  == raw[18] );

    std::deque<Operation> new_ops = raw_to_ops(raw);
    std::vector<uint8_t> new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );
}

TEST_CASE( "REORDER operation", "[Operations store]" )
{
    std::deque<Operation> ops;
    std::vector<uint16_t> c;
    c.emplace_back (5479);
    c.emplace_back (38);
    c.emplace_back (3179);
    ops.emplace_back (Operation::_t::REORDER, c);
    std::vector<uint8_t> raw = ops_to_raw(ops);
    REQUIRE( raw[0] == (uint8_t)Operation::_t::REORDER );
    REQUIRE( raw[0] != (uint8_t)Operation::_t::NONE );

    uint32_t size = raw[1] + (raw[2] << 8) + (raw[3] << 16) + (raw[4] << 24);
    REQUIRE( size == 3 );

    uint16_t number = raw[5] + (raw[6] << 8);
    REQUIRE( number == 5479 );

    number = raw[7] + (raw[8] << 8);
    REQUIRE( number == 38 );

    number = raw[9] + (raw[10] << 8);
    REQUIRE( number == 3179 );

    std::deque<Operation> new_ops = raw_to_ops(raw);
    std::vector<uint8_t> new_raw = ops_to_raw(new_ops);

    REQUIRE( raw == new_raw );
}
