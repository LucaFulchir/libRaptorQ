/*
 * Copyright (c) 2015,Luca Fulchir<luker@fenrirproject.org>,All rights reserved.
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

#include "../src/RaptorQ/RFC6330.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>


// Demonstration of how to use the C interface.
// This is kinda basic, we use only one function
// to encode, drop some (random) packets, and then
// decode what remains.

// Encode / decode "mysize" uint32_t
// for a more elaborate examlpe with different alignments, see test_cpp
bool decode (struct RFC6330_v1 *rfc, uint32_t mysize, float drop_prob,
                                                            uint8_t overhead);
bool decode (struct RFC6330_v1 *rfc, uint32_t mysize, float drop_prob,
                                                            uint8_t overhead)
{
    uint32_t *myvec;

    srand((uint32_t)time(NULL));
    // initialize vector with random data
    myvec = (uint32_t *) malloc (mysize * sizeof(uint32_t));
    for (uint32_t i = 0; i < mysize; ++i)
        myvec[i] = (uint32_t)rand();

    // what needs to be sent in a packet.
    // 32bit id (sbn + esi) and the actual data.
    struct pair {
        uint32_t id;
        uint32_t *symbol;
    };

    // will keep "sent" symbols here.
    struct pair *encoded;

    // symbol and sub-symbol sizes
    const uint16_t subsymbol = 8;
    const uint16_t symbol_size = 16;

    /*
     * Now start decoding things
     */

    // use multiple blocks
    struct RFC6330_ptr *enc = rfc->Encoder (RQ_ENC_32, myvec, mysize, subsymbol,
                                                            symbol_size, 200);

    if (enc == NULL || !rfc->initialized (enc)) {
        fprintf(stderr, "Coud not initialize encoder.\n");
        free (myvec);
        return false;
    }

    // start background precomputation while we get the source symbols.
    struct RFC6330_future *async_enc = rfc->compute (enc, RQ_COMPUTE_COMPLETE);
    // wait for all block to finish computing:
    if (async_enc == NULL) {
        fprintf(stderr, "Error in starting computation\n");
        rfc->free (&enc);
        free (myvec);
        return false;
    }
    rfc->future_wait (async_enc);
    struct RFC6330_Result enc_stat = rfc->future_get (async_enc);
    rfc->future_free (&async_enc);
    if (enc_stat.error != RQ_ERR_NONE) {
        printf ("ERR in getting future\n");
        return false;
    }

    /* everything is encoded now.
     * well, it might be running in background, but don't worry:
     * if needed you'll just block on the call.
     */

    if (drop_prob > (float)90.0)
        drop_prob = 90.0;   // this is still too high probably.


    // we used a lot of memory before so there will be only one block.
    // allocate memory only for that data.
    uint32_t symbols_tot = 0;
    for (uint8_t b = 0; b < rfc->blocks (enc); ++b) {
        uint16_t sym = rfc->symbols (enc, b);
        symbols_tot += (sym + overhead);
    }

    encoded = (struct pair *) malloc (sizeof(struct pair) * symbols_tot);
    for (uint32_t i = 0; i < symbols_tot; ++i)
        encoded[i].symbol = NULL;

    uint32_t next_encoded = 0;

    uint32_t blocks = rfc->blocks (enc);
    for (uint8_t block = 0; block < blocks; ++block) {
        printf("Block: %i\n", block);
        uint32_t sym_for_blk = rfc->symbols (enc, block);
        // some source packets will be lost. Be sure we will have
        // exactly (overhead + dropped_source) repair symbols.
        // and "sym_for_blk + overhead" total symbols
        int32_t repair = overhead;
        for (uint32_t source = 0; source < sym_for_blk; ++source) {
            float dropped = ((float)(rand()) / (float) RAND_MAX) * (float)100.0;
            if (dropped < drop_prob) {
                // dropped source symbol. Don't even get it.
                ++repair;
                continue;
            }
            // successfully transmitted source symbol. Add it to "encoded";
            encoded[next_encoded].id = rfc->id (source, block);
            uint32_t data_size = symbol_size / sizeof(uint32_t);
            encoded[next_encoded].symbol = (uint32_t *) malloc (symbol_size);
            uint32_t *data = encoded[next_encoded].symbol;
            uint64_t written = rfc->encode (enc, (void **)&data, data_size,
                                                    source, (uint8_t)block);
            // "data" now points to "encoded[next_encoded].symbol + written"
            // you can use this for some tricks if you *really* need it.
            if (written != data_size) {
                fprintf(stderr, "Error in getting source symbol\n");
                free (myvec);
                for (uint32_t k = 0; k <= next_encoded; ++k)
                    free (encoded[k].symbol);
                free (encoded);
                rfc->free (&enc);
                return false;
            }
            ++next_encoded;
        }
        printf("Dropped %i source packets\n", repair - overhead);
        // some repair packets will be lost. Be sure we will have
        // exactly (overhead + dropped_source) repair symbols,
        // and "sym_for_blk + overhead" total symbols
        uint32_t sym_rep;
        for (sym_rep = sym_for_blk; repair > 0 &&
                        sym_rep < rfc->max_repair (enc, block); ++sym_rep) {
            // repair symbols can be dropped, too!
            float dropped = ((float)(rand()) / (float) RAND_MAX) * (float)100.0;
            if (dropped < drop_prob) {
                // dropped repair symbol. Don't even get it.
                continue;
            }
            --repair;
            // successfully transmitted repair symbol. Add it to "encoded";
            encoded[next_encoded].id = rfc->id (sym_rep, block);
            uint32_t data_size = symbol_size / sizeof(uint32_t);
            encoded[next_encoded].symbol = (uint32_t *) malloc (symbol_size);
            uint32_t *data = encoded[next_encoded].symbol;
            uint64_t written = rfc->encode (enc, (void **)&data, data_size,
                                                    sym_rep, (uint8_t)block);
            // "*data" now points to "encoded[next_encoded].symbol + written"
            // you can use this for some tricks if you *really* need it.
            if (written != data_size) {
                fprintf(stderr, "Error in getting repair symbol\n");
                free (myvec);
                for (uint32_t k = 0; k <= next_encoded; ++k)
                    free (encoded[k].symbol);
                free (encoded);
                rfc->free (&enc);
                return false;
            }
            ++next_encoded;
        }
        if (sym_rep == rfc->max_repair (enc, block)) {
            fprintf(stderr, "Maybe losing %f%% symbols is too much?\n",
                                                            (double)drop_prob);
            free (myvec);
            for (uint32_t k = 0; k < next_encoded; ++k)
                free (encoded[k].symbol);
            free (encoded);
            rfc->free (&enc);
            return false;
        }
    }

    uint32_t oti_scheme = rfc->OTI_Scheme_Specific (enc);
    uint64_t oti_common = rfc->OTI_Common (enc);

    // optionally: free the memory just of block 0
    // this is done by the RaptorQ_free call anyway
    // RaptorQ_free_block (&enc, 0);
    // free the whole encoder memory:
    rfc->free (&enc);
    // enc == NULL now


    /*
     * Now start decoding things
     */

    struct RFC6330_ptr *dec = rfc->Decoder (RQ_DEC_32, oti_common, oti_scheme);

    if (dec == NULL || !rfc->initialized (dec)) {
        fprintf(stderr, "Could not initialize decoder!\n");
        free (myvec);
        for (uint32_t k = 0; k < next_encoded; ++k)
            free (encoded[k].symbol);
        free (encoded);
        return false;
    }
    // start background precomputation while we get the source symbols.
    struct RFC6330_future *async_dec =rfc->compute (dec,RQ_COMPUTE_COMPLETE);
    // wait for all block to finish computing:
    if (async_dec == NULL) {
        fprintf(stderr, "Error in starting computation\n");
        rfc->free (&dec);
        free (myvec);
        return false;
    }

    // we made things so there is only one block.
    // if you want to generalize things,
    // RaptorQ_blocks (dec); will tell you how many blocks
    // there are.
    for (size_t i = 0; i < next_encoded; ++i) {
        uint32_t *data = encoded[i].symbol;
        uint32_t data_size =(uint32_t)rfc->symbol_size (dec) / sizeof(uint32_t);
        RFC6330_Error err = rfc->add_symbol_id (dec, (void **)&data,
                                                    data_size, encoded[i].id);
        if (err != RQ_ERR_NONE && err != RQ_ERR_NOT_NEEDED) {
            // this can happen if we receive the same symbol twice
            // (which doesn't make much sense). But we constructed
            // everything so that there are no duplicates here,
            // so this is some other error.
            fprintf(stderr, "Error: couldn't add the symbol to the decoder\n");
            free (myvec);
            for (uint32_t k = 0; k < next_encoded; ++k)
                free (encoded[k].symbol);
            free (encoded);
            rfc->future_free (&async_dec);
            rfc->free (&dec);
            return false;
        }
        // "data" now points to encoded[i].symbol +
        //                  ceil(RaptorQ_symbol_size(dec) / sizeof(uint32_t))
    }
    // tell the decoder not to wait indefinitely
    // and exit if it does not have enough data.
    rfc->end_of_input (dec, RQ_NO_FILL);
    // optional: if you know there is no additional data, get the de-interleaved
    // data and a bitmask of which byte we actually have and which we zet to 0
    // structRFC6330_Byte_Tracker bytes =
    //                              rfc->end_of_input (dec, RQ_FILL_WITH_ZEROS);
    // then when you are done, free (bytes.bitmask);


    // make sure that there's enough place in "received" to get the
    // whole decoded data.
    // note: the length of the decoded data is in bytes and might not fit
    // a whole uint32_t.
    size_t decoded_size =(size_t) (rfc->bytes (dec) / sizeof(uint32_t));
    // this is not necessary in this example, but remember that integer
    // division acts as a floor()
    decoded_size += rfc->bytes (dec) % sizeof(uint32_t);
    uint32_t *received = (uint32_t *) malloc (decoded_size * sizeof(uint32_t));

    uint32_t *rec = received;

    // wait for the whole data to be decoded
    rfc->future_wait (async_dec);
    struct RFC6330_Result dec_stat = rfc->future_get (async_dec);
    rfc->future_free (&async_dec);
    if (dec_stat.error != RQ_ERR_NONE) {
        if (dec_stat.error == RQ_ERR_NEED_DATA) {
            printf ("Decoding failed. try again?\n");
            return true;
        } else {
            printf ("ERR in getting future\n");
            return false;
        }
    }

    uint64_t written = rfc->decode_bytes (dec, (void **)&rec, decoded_size,0);
    // we are assuming no errors, all blocks decoded.
    // "rec" now points to "received + written"
    // This might help you to call RaptorQ_decode_sbn multiple time
    // on the same pointer.

    if ((written != mysize * sizeof(uint32_t)) || (decoded_size != mysize)) {
        fprintf(stderr, "Couldn't decode: %i - %lu: %lu\n", mysize,
                                                        decoded_size, written);
        free (myvec);
        free(received);
        for (uint32_t k = 0; k < next_encoded; ++k)
            free (encoded[k].symbol);
        free (encoded);
        rfc->free(&dec);
        return false;
    } else {
        printf("Decoded: %i\n", mysize);
    }
    // check if everything was decoded nicely
    for (uint16_t i = 0; i < mysize; ++i) {
        if (myvec[i] != received[i]) {
            fprintf(stderr, "FAILED, but we though otherwise! %i - %f: %i "
                                                "%i -- %i\n",
                                                mysize, (double) drop_prob, i,
                                                myvec[i], received[i]);
            free (myvec);
            free (received);
            for (uint32_t k = 0; k < next_encoded; ++k)
                free (encoded[k].symbol);
            free (encoded);
            rfc->free (&dec);
            return false;
        }
    }

    // optionally: free the memory just of block 0
    // this is done by the RaptorQ_free call anyway
    // RaptorQ_free_block (&dec, 0);
    // free the decoder memory
    free (myvec);
    free(received);
    for (uint32_t k = 0; k < next_encoded; ++k)
        free (encoded[k].symbol);
    free (encoded);
    rfc->free(&dec);
    // dec == NULL now
    return true;
}

int main (void)
{

    // get the api function pointers
    struct RFC6330_v1 *rfc = (struct RFC6330_v1*) RFC6330_api (1);
    if (rfc == NULL) {
        fprintf(stderr, "ERR: could not get RFC6330 API\n");
        return 1;
    }

    // set local cache size to 100MB
#ifdef RQ_USE_LZ4
    rfc->set_compression (RQ_COMPRESS_LZ4);
#else
    RaptorQ_set_compression (RQ_COMPRESS_NONE);
#endif
    rfc->local_cache_size (100*1024*1024);
    rfc->set_thread_pool (2, 2, RQ_WORK_ABORT_COMPUTATION);
    // encode and decode
    bool ret = decode (rfc, 501, 20.0, 4);

    RFC6330_free_api ((struct RFC6330_base_api**)&rfc);

    return (ret == true ? 0 : -1);
}

