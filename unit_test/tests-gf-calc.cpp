#include <RaptorQ/v1/Octet.hpp>
#include <memory>
#include <utility>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include "catch2/catch.hpp"

using namespace RaptorQ__v1::Impl;

/* From: https://en.wikipedia.org/wiki/Finite_field_arithmetic
 * Multiply two numbers in the GF(2^8) finite field defined
 * by the polynomial x^8 + x^4 + x^3 + x^2 + 1 = 0
 * using the Russian Peasant Multiplication algorithm
 * (the other way being to do carry-less multiplication followed by a modular reduction)
 */
uint8_t gf_multiplication(uint8_t a, uint8_t b) {
	uint8_t p = 0; /* the product of the multiplication */
	while (a && b) {
            if (b & 1) /* if b is odd, then add the corresponding a to p (final product = sum of all a's corresponding to odd b's) */
                p ^= a; /* since we're in GF(2^m), addition is an XOR */

            if (a & 0x80) /* GF modulo: if a >= 128, then it will overflow when shifted left, so reduce */
                a = (a << 1) ^ 0x11d; /* XOR with the primitive polynomial x^8 + x^4 + x^3 + x^2 + 1 (0b1_0001_1101) – you can change it but it must be irreducible */
            else
                a <<= 1; /* equivalent to a*2 */
            b >>= 1; /* equivalent to b // 2 */
	}
	return p;
}

TEST_CASE( "Octet multiplication", "[Galois Fields]" )
{

    for ( int a = 0; a < 256; a ++ )
    {
        for ( int b = 0; b < 256; b ++ )
        {
            REQUIRE( Octet(a) * Octet(b) == Octet(b) * Octet(a) );
            REQUIRE( Octet(a) * Octet(b) == gf_multiplication(a, b) );
        }
    }
}

TEST_CASE( "Octet no-if multiplication", "[Galois Fields]" )
{
    for ( int a = 0; a < 256; a ++ )
    {
        for ( int b = 0; b < 256; b ++ )
        {
            uint8_t c = oct_exp_no_if[oct_log_no_if[a] + oct_log_no_if[b]];
            uint8_t d = oct_exp_no_if[oct_log_no_if[b] + oct_log_no_if[a]];
            REQUIRE( c == d );
            REQUIRE( c == gf_multiplication(a, b) );
        }
    }
}
