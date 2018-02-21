/*
 * Copyright (c) 2015-2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#include "RaptorQ/v1/Precode_Matrix.hpp"
#include "RaptorQ/v1/DenseOctetMatrix.hpp"
#include "RaptorQ/v1/util/Graph.hpp"
#include "RaptorQ/v1/util/Matrix.hpp"

///////////////////
//
// Precode_Matrix
//
///////////////////

namespace RaptorQ__v1 {
namespace Impl {

// The template parameter "IS_OFFLINE" lets us identify
// wether we should save the computation we are doing for offline usage
// or we can avoid saving it, and thus be faster and more memory efficient.

template <Save_Computation IS_OFFLINE>
std::pair<Precode_Result, DenseOctetMatrix>
                                    Precode_Matrix<IS_OFFLINE>::intermediate (
                                    DenseOctetMatrix &D, Op_Vec &ops,
                                    bool &keep_working,
                                    const Work_State *thread_keep_working)
{
    // rfc 6330, pg 32
    // "c" and "d" are used to track row and columns exchange.
    // since Eigen should track row exchange without actually swapping
    // the data, we can call DenseMtx.row.swap without more overhead
    // than actually having "d". so we're left only with "c",
    // which is needed 'cause D does not have _params.L columns.

    std::vector<uint16_t> c;

    c.clear();
    c.reserve (_params.L);
    DenseOctetMatrix C;
    DenseOctetMatrix X = A;

    bool success;
    uint16_t i, u;
    for (i = 0; i < _params.L; ++i)
        c.emplace_back (i);

    DenseOctetMatrix CP_D;
    if (debug)
        CP_D = D;
    std::tie (success, i, u) = decode_phase1 (X, D, c , ops,
                                            keep_working, thread_keep_working);
    if (stop (keep_working, thread_keep_working))
        return std::make_pair (Precode_Result::STOPPED, DenseOctetMatrix());
    if (!success)
        return std::make_pair (Precode_Result::FAILED, DenseOctetMatrix());

    success = decode_phase2 (D, i, u, ops, keep_working, thread_keep_working);
    if (stop (keep_working, thread_keep_working))
        return std::make_pair (Precode_Result::STOPPED, DenseOctetMatrix());
    if (!success)
        return std::make_pair (Precode_Result::FAILED, DenseOctetMatrix());
    // A now should be considered as being LxL from now
    decode_phase3 (X, D, i, ops);
    if (stop (keep_working, thread_keep_working))
        return std::make_pair (Precode_Result::STOPPED, DenseOctetMatrix());

    X = DenseOctetMatrix();    // free some memory, X is not needed anymore.
    decode_phase4 (D, i, u, ops, keep_working, thread_keep_working);
    if (stop (keep_working, thread_keep_working))
        return std::make_pair (Precode_Result::STOPPED, DenseOctetMatrix());
    if (!success)
        return std::make_pair (Precode_Result::FAILED, DenseOctetMatrix());

    decode_phase5 (D, i, ops, keep_working, thread_keep_working);
    if (stop (keep_working, thread_keep_working))
        return std::make_pair (Precode_Result::STOPPED, DenseOctetMatrix());
    if (!success)
        return std::make_pair (Precode_Result::FAILED, DenseOctetMatrix());

    // A now must be an LxL identity matrix: check it.
    // CHECK DISABLED: phase4  does not modify A, as it's never readed
    // again. So the Matrix is *not* an identity anymore.
    //auto id_A = A.block (0, 0, _params.L, _params.L);
    //for (uint16_t row = 0; row < id_A.rows(); ++row) {
    //  for (uint16_t col = 0; col < id_A.cols(); ++col) {
    //      if (static_cast<uint8_t> (id_A (row, col)) != (row == col ? 1 : 0))
    //          return C;
    //  }
    //}
    A = DenseOctetMatrix();

    if (IS_OFFLINE == Save_Computation::ON)
        ops.emplace_back (Operation::_t::REORDER, c);

    C = DenseOctetMatrix (_params.L, D.cols());
    for (i = 0; i < _params.L; ++i)
        Matrix::row_assign(C, c[i], D, i);

    if (debug && ops.size() != 0) {
        DenseOctetMatrix test_off (D.rows(), D.rows());
        test_off.setIdentity ();
        for (const auto &op : ops)
            op.build_mtx (test_off);
        DenseOctetMatrix test_res = test_off * CP_D;
        assert (test_res == C && "RQ: I'm different!");
    }
    return std::make_pair (Precode_Result::DONE, C);
}

template <Save_Computation IS_OFFLINE>
std::pair<Precode_Result, DenseOctetMatrix>
                                    Precode_Matrix<IS_OFFLINE>::intermediate (
                                    DenseOctetMatrix &D, const Bitmask &mask,
                                    const std::vector<uint32_t> &repair_esi,
                                    Op_Vec &ops, bool &keep_working,
                                    const Work_State *thread_keep_working)
{
    decode_phase0 (mask, repair_esi);
    return intermediate (D, ops, keep_working, thread_keep_working);
}

template <Save_Computation IS_OFFLINE>
DenseOctetMatrix Precode_Matrix<IS_OFFLINE>::get_missing (
                                                    const DenseOctetMatrix &C,
                                                    const Bitmask &mask) const
{
    if (C.rows() == 0)
        return C;
    DenseOctetMatrix missing = DenseOctetMatrix (mask.get_holes(), C.cols());
    uint16_t holes = mask.get_holes();
    uint16_t row = 0;
    for (uint16_t hole = 0; hole < mask._max_nonrepair && holes > 0; ++hole) {
        if (mask.exists (hole))
            continue;
        DenseOctetMatrix ret = encode (C, hole);
        Matrix::row_assign(missing, row, ret, 0);
        ++row;
        --holes;
    }
    return missing;
}

template <Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::decode_phase0 (const Bitmask &mask,
                                        const std::vector<uint32_t> &repair_esi)
{
    // D was built as follows:
    // - non-repair esi in their place
    // - for each hole in non-repair esi, put the *first available* repair esi
    //      in its place. they are already ordered anyway
    // - compact remaining repair esis


    // substitute missing symbols in A with appropriate repair line.
    // we substituted some symbols with repair ones (rfc 6330, phase1, pg35),
    // so we need to fix the corresponding rows in A, to say that a
    // repair symbol comes from a set of other symbols.

    const size_t padding = _params.K_padded - mask._max_nonrepair;

    uint16_t holes = mask.get_holes();
    auto r_esi = repair_esi.begin();

    for (uint16_t hole_from = 0; hole_from < _params.L && holes > 0;
                                                                ++hole_from) {
        if (mask.exists (hole_from))
            continue;
        // now hole_from is the esi hole, and hole_to is our repair sym.
        // put the repair dependancy in the hole row
        auto depends = _params.get_idxs (static_cast<uint16_t> (
                                                            *r_esi + padding));
        ++r_esi;
        // erease the line, mark the dependencies of the repair symbol.
        const uint16_t row = hole_from + _params.H + _params.S;
        for (uint16_t col = 0; col < A.cols(); ++col) {
            A (row, col) = 0;
        }
        for (auto isi: depends) {
            A (row, isi) = 1;
        }
        --holes;
    }
    // we put the repair symbols in the right places,
    // but we still need to do the same modifications to A also for repair
    // symbols. And those have been compacted.

    for (uint16_t rep_row = static_cast<uint16_t> (
                            static_cast<uint32_t>(A.rows()) - _repair_overhead);
                                                rep_row < A.rows(); ++rep_row) {
        auto depends = _params.get_idxs (static_cast<uint16_t> (
                                                            *r_esi + padding));
        ++r_esi;
        // erease the line, mark the dependencies of the repair symbol.
        for (uint16_t col = 0; col < A.cols(); ++col) {
            A (rep_row, col) = 0;
        }
        for (auto isi: depends) {
            A (rep_row, isi) = 1;
        }
    }
}

template <Save_Computation IS_OFFLINE>
std::tuple<bool, uint16_t, uint16_t>
    Precode_Matrix<IS_OFFLINE>::decode_phase1 (DenseOctetMatrix &X,
                                        DenseOctetMatrix &D,
                                        std::vector<uint16_t> &c,
                                        Op_Vec &ops, bool &keep_working,
                                        const Work_State *thread_keep_working)
{
    // rfc6330, page 33

    std::vector<std::pair<bool, size_t>> tracking;  // is_hdpc, row_degree

    // optimization: r_rows tracks the rows that can be chosen, and if the row
    // is added to the graph, track also the id of one of the nodes with "1",
    // so that it will be easy to verify it. The row represents an edge
    // between nodes (1) of a maximum component (see rfc 6330, pg 33-34)
    std::vector<std::pair<uint16_t, uint16_t>> r_rows;

    tracking.reserve (static_cast<size_t> (A.rows()));

    uint16_t i = 0;
    uint16_t u = _params.P;

    auto V_tmp = A.block (0, 0, A.rows(), A.cols() - u);

    // track hdpc rows and original degree of each row
    for (uint16_t row = 0; row < V_tmp.rows(); ++row) {
        size_t original_degree = 0;
        for (uint16_t col = 0; col < V_tmp.cols(); ++col)
            original_degree += static_cast<uint8_t> (V_tmp (row, col));
        bool is_hdpc = (row >= _params.S && row < (_params.S + _params.H));
        tracking.emplace_back (is_hdpc, original_degree);;
    }

    while (i + u < _params.L) {
        if (stop (keep_working, thread_keep_working))
            return std::tuple<bool,uint16_t,uint16_t> (false, 0, 0); // stop
        auto V = A.block (i, i, A.rows() - i, (A.cols() - i) - u);
        uint16_t chosen = static_cast<uint16_t> (V.rows());
        // search for minium "r" (number of nonzero elements in row)
        uint16_t non_zero = static_cast<uint16_t> (V.cols()) + 1;
        bool only_two_ones = false;
        r_rows.clear();
        Graph G = Graph (static_cast<uint16_t> (V.cols()));

        // build graph, get minimum non_zero and track rows that
        // will be needed later
        for (uint16_t row = 0; row < V.rows(); ++row) {
            if (stop (keep_working, thread_keep_working))
                return std::tuple<bool,uint16_t,uint16_t> (false, 0, 0); // stop
            uint16_t non_zero_tmp = 0;
            // if the row is NOT HDPC and has two ones,
            // it represents an edge in a graph between the two columns with "1"
            uint16_t ones = 0;
            std::array<uint16_t, 2> ones_idx = {{0, 0}};
            bool next_row = false;  // true => non_zero_tmp > zero_tmp
            for (uint16_t col = 0; col < V.cols(); ++col) {
                if (static_cast<uint8_t> (V (row, col)) != 0) {
                    if (++non_zero_tmp > non_zero) {
                        next_row = true;
                        break;
                    }
                }
                if (static_cast<uint8_t> (V (row, col)) == 1) {
                    // count the ones and update ones_idx at the same time
                    if (++ones <= 2)
                        ones_idx[ones - 1] = col;
                }
            }
            if (next_row || non_zero_tmp == 0)
                continue;
            // now non_zero >= non_zero_tmp, and both > 0

            // rationale & optimization, rfc 6330 pg 34
            // we need to track the rows that have the least number "r"
            // of non-zero elements.
            // if r == 2 and even just one row has the two elements to "1",
            // then we need to track only the rows with "1" in the two
            // non-zero elements.
            if (non_zero == non_zero_tmp) {
                // do not add if "only_two_ones && ones != 2"
                if (!only_two_ones || ones == 2)
                    r_rows.emplace_back (row, ones_idx[0]);
            } else {
                // non_zero > non_zero_tmp)
                non_zero = non_zero_tmp;
                r_rows.clear();
                r_rows.emplace_back (row, ones_idx[0]);
            }

            if (ones == 2) {
                // track the maximum component in the graph
                if (non_zero == 2) {
                    if (!tracking[row].first)   // if not HDPC row
                        G.connect (ones_idx[0], ones_idx[1]);
                    if (!only_two_ones) {
                        // must keep only rows with two ones,
                        // so delete the other ones.
                        only_two_ones = true;
                        r_rows.clear();
                        r_rows.emplace_back (row, ones_idx[0]);
                    }
                }
            }
        }
        if (non_zero == V.cols() + 1)
            return std::tuple<bool,uint16_t,uint16_t> (false, 0, 0); // failure
        // search for r.
        if (non_zero != 2) {
            // search for row with minimum original degree.
            // Precedence to non-hdpc
            uint16_t min_row = static_cast<uint16_t> (V.rows());
            uint16_t min_row_hdpc = min_row;
            size_t min_degree = ~(static_cast<size_t> (0)); // max possible
            size_t min_degree_hdpc = min_degree;
            for (auto row_pair : r_rows) {
                uint16_t row = row_pair.first;
                if (tracking[row + i].first) {
                    // HDPC
                    if (tracking[row + i].second < min_degree_hdpc) {
                        min_degree_hdpc = tracking[row + i].second;
                        min_row_hdpc = row;
                    }
                } else {
                    // NON-HDPC
                    if (tracking[row + i].second < min_degree) {
                        min_degree = tracking[row + i].second;
                        min_row = row;
                    }
                }
            }
            if (min_row != V.rows()) {
                chosen = min_row;
            } else {
                chosen = min_row_hdpc;
            }
        } else {
            // non_zero == 2 => graph, else any r
            if (only_two_ones) {
                for (auto id : r_rows) {
                    if (G.is_max (id.second)) {
                        chosen = id.first;
                        break;
                    }
                }
            }
            if (chosen == V.rows()) {
                chosen = r_rows[0].first;
            }
        }   // done choosing

        // swap chosen row and first V row in A (not just in V)
        if (chosen != 0) {
            Matrix::row_swap(A, i, chosen + i);
            Matrix::row_swap(X, i, chosen + i);
            Matrix::row_swap(D, i, chosen + i);
            std::swap (tracking[i], tracking[chosen + i]);
            if (IS_OFFLINE == Save_Computation::ON)
                ops.emplace_back (Operation::_t::SWAP, i, chosen + i);
        }
        // column swap in A. looking at the first V row,
        // the first column must be nonzero, and the other non-zero must be
        // put to the last columns of V.
        if (static_cast<uint8_t> (V (0, 0)) == 0) {
            uint16_t idx = 1;
            for (; idx < V.cols(); ++idx) {
                if (static_cast<uint8_t> (V (0, idx)) != 0)
                    break;
            }
            Matrix::col_swap(A, i, i + idx);
            Matrix::col_swap(X, i, i + idx);
            std::swap (c[i], c[i + idx]);   // rfc6330, pg32
        }
        uint16_t col = static_cast<uint16_t> (V.cols()) - 1;
        uint16_t swap = 1;  // at most we swapped V(0,0)
        if (stop (keep_working, thread_keep_working))
            return std::tuple<bool,uint16_t,uint16_t> (false, 0, 0); // stop
        // put all the non-zero cols to the last columns.
        for (; col > V.cols() - non_zero; --col) {
            if (static_cast<uint8_t> (V (0, col)) != 0)
                continue;
            while (swap < col && static_cast<uint8_t> (V (0, swap)) == 0)
                ++swap;

            if (swap >= col)
                break;  // line full of zeros, nothing to swap
            // now V(0, col) == 0 and V(0, swap != 0. swap them
            Matrix::col_swap(A, col + i, swap + i);
            Matrix::col_swap(X, col + i, swap + i);
            std::swap (c[col + i], c[swap + i]);    //rfc6330, pg32
        }
        if (stop (keep_working, thread_keep_working))
            return std::tuple<bool,uint16_t,uint16_t> (false, 0, 0); // stop
        // now add a multiple of the row V(0) to the other rows of *A* so that
        // the other rows of *V* have a zero first column.
        for (uint16_t row = 1; row < V.rows(); ++row) {
            if (static_cast<uint8_t> (V (row, 0)) != 0) {
                const Octet multiple = V (row, 0) / V (0, 0);
                //rfc6330, pg32
                Matrix::row_multiply_add(A, row + i, A, i,
                                            static_cast<uint8_t> (multiple));
                Matrix::row_multiply_add(D, row + i, D, i,
                                            static_cast<uint8_t> (multiple));
                if (IS_OFFLINE == Save_Computation::ON) {
                    ops.emplace_back (Operation::_t::ADD_MUL, row + i, i,
                                                                    multiple);
                }
            }
        }

        // finally increment i by 1, u by (non_zero - 1) and repeat.
        ++i;
        u += non_zero - 1;
    }

    return std::make_tuple (true, i, u);
}

template<Save_Computation IS_OFFLINE>
bool Precode_Matrix<IS_OFFLINE>::decode_phase2 (DenseOctetMatrix &D,
                                        const uint16_t i, const uint16_t u,
                                        Op_Vec &ops, bool &keep_working,
                                        const Work_State *thread_keep_working)
{
    // rfc 6330, pg 35

    // U_Lower parameters (u x u):
    const uint16_t row_start = i, row_end = static_cast<uint16_t> (_params.L);
    const uint16_t col_start = static_cast<uint16_t> (A.cols() - u);
    // try to bring U_Lower to Identity with gaussian elimination.
    // remember that all row swaps affect A as well, not just U_Lower

    for (uint16_t row = row_start; row < row_end; ++row) {
        if (stop (keep_working, thread_keep_working))
            return false; // stop
        // make sure the considered row has nonzero on the diagonal
        uint16_t row_nonzero = row;
        const uint16_t col_diag = col_start + (row - row_start);
        for (; row_nonzero < row_end; ++row_nonzero) {
            if (static_cast<uint8_t> (A (row_nonzero, col_diag)) != 0) {
                break;
            }
        }
        if (row_nonzero == row_end) {
            // U_Lower is square, we can return early (rank < u, not solvable)
            return false;
        } else if (row != row_nonzero) {
            Matrix::row_swap(A, row, row_nonzero);
            Matrix::row_swap(D, row, row_nonzero);
            if (IS_OFFLINE == Save_Computation::ON)
                ops.emplace_back (Operation::_t::SWAP, row, row_nonzero);
        }

        // U_Lower (row, row) != 0. make it 1.
        if (static_cast<uint8_t> (A (row, col_diag)) > 1) {
            const auto divisor = A (row, col_diag);
            Matrix::row_div(A, row, static_cast<uint8_t> (divisor));
            Matrix::row_div(D, row, static_cast<uint8_t> (divisor));
            if (IS_OFFLINE == Save_Computation::ON)
                ops.emplace_back (Operation::_t::DIV, row, Octet(divisor));
        }

        // make U_Lower and identity up to row
        for (uint16_t del_row = row_start; del_row < row_end; ++del_row) {
            if (stop (keep_working, thread_keep_working))
                return false;   // stop
            if (del_row == row)
                continue;
            // subtract row "row" to "del_row" enough times to make
            // row "del_row" start with zero. but row "row" now starts
            // with "1", so this is easy.
            const auto multiple = A (del_row, col_diag);
            if (static_cast<uint8_t> (multiple) != 0) {
                Matrix::row_multiply_sub(A, del_row, A, row,
                                            static_cast<uint8_t> (multiple));
                Matrix::row_multiply_sub(D, del_row, D, row,
                                            static_cast<uint8_t> (multiple));
                if (IS_OFFLINE == Save_Computation::ON)
                    ops.emplace_back (Operation::_t::ADD_MUL, del_row, row,
                                                                    multiple);
            }
        }
    }
    // A should be resized to LxL.
    // we don't really care, as we should not gain that much.
    // A.conservativeResize (params.L, params.L);
    return true;
}

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::decode_phase3 (DenseOctetMatrix &X,
                                                DenseOctetMatrix &D,
                                                const uint16_t i, Op_Vec &ops)
{
    // rfc 6330, pg 35:
    //  To this end, the matrix X is
    //  multiplied with the submatrix of A consisting of the first i rows of
    //  A. After this operation, the submatrix of A consisting of the
    //  intersection of the first i rows and columns equals to X, whereas the
    //  matrix U_upper is transformed to a sparse form.
    SparseMtx sub_X = X.topLeftSparseView(i, i);
    if (IS_OFFLINE == Save_Computation::ON)
        ops.emplace_back (Operation::_t::BLOCK, sub_X);

    SparseMtx sub_A = A.topLeftSparseView(i, A.cols());

    SparseMtx res = sub_X * sub_A;
    A.valuesFromEigenTopLeft(res);

    // Now fix D, too
    DenseMtx D_2 = D.toEigen(0, 0, sub_X.cols(), D.cols());

    D_2 = sub_X * D_2;
    D.valuesFromEigen(D_2, 0, 0, sub_X.cols(), D.cols());
}

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::decode_phase4 (DenseOctetMatrix &D,
                                        const uint16_t i, const uint16_t u,
                                        Op_Vec &ops, bool &keep_working,
                                        const Work_State *thread_keep_working)
{
    // rfc 6330, pg 35:
    // For each of the first i rows of U_upper, do the following: if the row
    // has a nonzero entry at position j, and if the value of that nonzero
    // entry is b, then add to this row b times row j of I_u

    // basically: zero out U_upper. we still need to update D each time, though.

    auto U_upper = A.block (0, A.cols() - u, i, u);
    for (uint16_t row = 0; row < U_upper.rows(); ++row) {
        if (stop (keep_working, thread_keep_working))
            return;
        for (uint16_t col = 0; col < U_upper.cols(); ++col) {
            // col == j
            uint8_t multiple = U_upper (row, col);
            if (multiple != 0) {
                // U_upper is never read again, so we can avoid some writes
                //U_upper (row, col) = 0;

                // "b times row j of I_u" => row "j" in U_lower.
                // aka: U_upper.rows() + j
                uint16_t row_2 = static_cast<uint16_t> (U_upper.rows()) + col;
                Matrix::row_multiply_add(D, row, D, row_2, multiple);
                if (IS_OFFLINE == Save_Computation::ON) {
                    ops.emplace_back (Operation::_t::ADD_MUL, row, row_2,
                                                                    multiple);
                }
            }
        }
    }
}

template<Save_Computation IS_OFFLINE>
void Precode_Matrix<IS_OFFLINE>::decode_phase5 (DenseOctetMatrix &D,
                                        const uint16_t i, Op_Vec &ops,
                                        bool &keep_working,
                                        const Work_State *thread_keep_working)
{
    // rc 6330, pg 36
    for (uint16_t j = 0; j <= i; ++j) {
        if (stop (keep_working, thread_keep_working))
            return;
        if (static_cast<uint8_t> (A (j, j)) != 1) {
            // A(j, j) is actually never 0, by construction.
            const auto multiple = A (j, j);
            Matrix::row_div(A, j, static_cast<uint8_t> (multiple));
            Matrix::row_div(D, j, static_cast<uint8_t> (multiple));
            if (IS_OFFLINE == Save_Computation::ON)
                ops.emplace_back (Operation::_t::DIV, j, Octet(multiple));
        }
        for (uint16_t tmp = 0; tmp < j; ++tmp) {    //tmp == "l" in rfc6330
            const auto multiple = A (j, tmp);
            if (static_cast<uint8_t> (multiple) != 0) {
                Matrix::row_multiply_add(A, j, A, tmp,
                                            static_cast<uint8_t> (multiple));
                Matrix::row_multiply_add(D, j, D, tmp,
                                            static_cast<uint8_t> (multiple));
                if (IS_OFFLINE == Save_Computation::ON)
                    ops.emplace_back (Operation::_t::ADD_MUL, j, tmp, multiple);
            }
        }
    }
}

template<Save_Computation IS_OFFLINE>
DenseOctetMatrix Precode_Matrix<IS_OFFLINE>::encode (const DenseOctetMatrix &C,
                                                    const uint32_t ISI) const
{
    // Generate repair symbols. same algorithm as "get_idxs"
    // rfc6330, pg29

    DenseOctetMatrix ret;

    ret = DenseOctetMatrix (1, C.cols());
    Tuple t = _params.tuple (ISI);

    Matrix::row_assign(ret, 0, C, t.b);

    for (uint16_t j = 1; j < t.d; ++j) {
        t.b = (t.b + t.a) % _params.W;
        Matrix::row_add(ret, 0, C, t.b);
    }
    while (t.b1 >= _params.P)
        t.b1 = (t.b1 + t.a1) % _params.P1;

    Matrix::row_add(ret, 0, C, _params.W + t.b1);
    for (uint16_t j = 1; j < t.d1; ++j) {
        t.b1 = (t.b1 + t.a1) % _params.P1;
        while (t.b1 >= _params.P)
            t.b1 = (t.b1 + t.a1) % _params.P1;
        Matrix::row_add(ret, 0, C, _params.W + t.b1);
    }

    return ret;
}

}   // namespace RaptorQ
}   // namespace Impl
