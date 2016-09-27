/*
 * Copyright (c) 2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

#include "Graph.hpp"
#include "multiplication.hpp"
#include "Precode_Matrix.hpp"
#include "Rand.hpp"

///////////////////
//
// Precode_Matrix
//
///////////////////

///
/// These methods are used to solve the system A * C = D, where we have
/// A and D. By doing this, we generate the intermediate symbols.
///

namespace RaptorQ {
namespace Impl {

DenseMtx Precode_Matrix::intermediate (DenseMtx &D)
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
	DenseMtx C, X = A;

	bool success;
	uint16_t i, u;
	for (i = 0; i < _params.L; ++i)
		c.emplace_back (i);

	std::tie (success, i, u) = decode_phase1 (X, D, c);
	if (!success)
		return C;
	success = decode_phase2 (D, i, u);
	if (!success)
		return C;
	// A now should be considered as being LxL from now
	decode_phase3 (X, D, i);
	X = DenseMtx ();	// free some memory, X is not needed anymore.
	decode_phase4 (D, i, u);
	decode_phase5 (D, i);
	// A now must be an LxL identity matrix: check it.
	// CHECK DISABLED: phase4  does not modify A, as it's never readed
	// again. So the Matrix is *not* an identity anymore.
	//auto id_A = A.block (0, 0, _params.L, _params.L);
	//for (uint16_t row = 0; row < id_A.rows(); ++row) {
	//	for (uint16_t col = 0; col < id_A.cols(); ++col) {
	//		if (static_cast<uint8_t> (id_A (row, col)) != (row == col ? 1 : 0))
	//			return C;
	//	}
	//}
	A = DenseMtx();	// free A memory.

	C = DenseMtx (D.rows(), D.cols());
	for (i = 0; i < _params.L; ++i)
		C.row (c[i]) = D.row (i);

	return C;
}

// Used in decoding
DenseMtx Precode_Matrix::intermediate (DenseMtx &D, const Bitmask &mask,
										const std::vector<uint32_t> &repair_esi)
{
	decode_phase0 (mask, repair_esi);
	DenseMtx C = intermediate (D);

	if (C.rows() == 0) {
		// error somewhere
		return DenseMtx();
	}

	DenseMtx missing = DenseMtx (mask.get_holes(), D.cols());
	uint16_t holes = mask.get_holes();
	uint16_t row = 0;
	for (uint16_t hole = 0; hole < mask._max_nonrepair && holes > 0; ++hole) {
		if (mask.exists (hole))
			continue;
		DenseMtx ret = encode (C, hole);
		missing.row (row) = ret.row(0);
		++row;
		--holes;
	}
	return missing;
}

void Precode_Matrix::decode_phase0 (const Bitmask &mask,
										const std::vector<uint32_t> &repair_esi)
{
	// D was built as follows:
	// - non-repair esi in their place
	// - for each hole in non-repair esi, put the *first available* repair esi
	//		in its place
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

	for (uint16_t rep_row = static_cast<uint16_t> (A.rows() -
                                    static_cast<int32_t> (_repair_overhead));
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

std::tuple<bool, uint16_t, uint16_t>
	Precode_Matrix::decode_phase1 (DenseMtx &X, DenseMtx &D,
													std::vector<uint16_t> &c)
{
	//rfc6330, page 33

	std::vector<std::pair<bool, size_t>> tracking;	// is_hdpc, row_degree

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
			uint16_t non_zero_tmp = 0;
			// if the row is NOT HDPC and has two ones,
			// it represents an edge in a graph between the two columns with "1"
			uint16_t ones = 0;
			std::array<uint16_t, 2> ones_idx = {{0, 0}};
			bool next_row = false;	// true => non_zero_tmp > zero_tmp
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
					if (!tracking[row].first)	// if not HDPC row
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
			return std::make_tuple (false, 0, 0);	// failure
		// search for r.
		if (non_zero != 2) {
			// search for row with minimum original degree.
			// Precedence to non-hdpc
			uint16_t min_row = static_cast<uint16_t> (V.rows());
			uint16_t min_row_hdpc = min_row;
			size_t min_degree = ~(static_cast<size_t> (0));	// max possible
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
		}	// done choosing

		// swap chosen row and first V row in A (not just in V)
		if (chosen != 0) {
			A.row (i).swap (A.row (chosen + i));
			X.row (i).swap (X.row (chosen + i));
			D.row (i).swap (D.row (chosen + i));
			std::swap (tracking[i], tracking[chosen + i]);
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
			A.col (i).swap (A.col (i + idx));
			X.col (i).swap (X.col (i + idx));
			std::swap (c[i], c[i + idx]);	// rfc6330, pg32
		}
		uint16_t col = static_cast<uint16_t> (V.cols()) - 1;
		uint16_t swap = 1;	// at most we swapped V(0,0)
		// put all the non-zero cols to the last columns.
		for (; col > V.cols() - non_zero; --col) {
			if (static_cast<uint8_t> (V (0, col)) != 0)
				continue;
			while (swap < col && static_cast<uint8_t> (V (0, swap)) == 0)
				++swap;

			if (swap >= col)
				break;	// line full of zeros, nothing to swap
			// now V(0, col) == 0 and V(0, swap != 0. swap them
			A.col (col + i).swap (A.col (swap + i));
			X.col (col + i).swap (X.col (swap + i));
			std::swap (c[col + i], c[swap + i]);	//rfc6330, pg32
		}
		// now add a multiple of the row V(0) to the other rows of *A* so that
		// the other rows of *V* have a zero first column.
		for (uint16_t row = 1; row < V.rows(); ++row) {
			if (static_cast<uint8_t> (V (row, 0)) != 0) {
				const Octet multiple = V (row, 0) / V (0, 0);
				A.row (row + i) += A.row (i) * multiple;
				D.row (row + i) += D.row (i) * multiple;	//rfc6330, pg32
			}
		}

		// finally increment i by 1, u by (non_zero - 1) and repeat.
		++i;
		u += non_zero - 1;
	}

	return std::make_tuple (true, i, u);
}

bool Precode_Matrix::decode_phase2 (DenseMtx &D, const uint16_t i,
															const uint16_t u)
{
	// rfc 6330, pg 35

	// U_Lower parameters (u x u):
	const uint16_t row_start = i, row_end = static_cast<uint16_t> (_params.L);
	const uint16_t col_start = static_cast<uint16_t> (A.cols() - u);
	// try to bring U_Lower to Identity with gaussian elimination.
	// remember that all row swaps affect A as well, not just U_Lower

	for (uint16_t row = row_start; row < row_end; ++row) {
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
			A.row (row).swap (A.row (row_nonzero));
			D.row (row).swap (D.row (row_nonzero));
		}

		// U_Lower (row, row) != 0. make it 1.
		if (static_cast<uint8_t> (A (row, col_diag)) > 1) {
			const auto divisor = A (row, col_diag);
			A.row (row) /= divisor;
			D.row (row) /= divisor;
		}

		// make U_Lower and identity up to row
		for (uint16_t del_row = row_start; del_row < row_end; ++del_row) {
			if (del_row == row)
				continue;
			// subtrace row "row" to "del_row" enough times to make
			// row "del_row" start with zero. but row "row" now starts
			// with "1", so this is easy.
			const auto multiple = A (del_row, col_diag);
			if (static_cast<uint8_t> (multiple) != 0) {
				A.row (del_row) -= A.row (row) * multiple;
				D.row (del_row) -= D.row (row) * multiple;
			}
		}
	}
	// A should be resized to LxL.
	// we don't really care, as we should not gain that much.
	// A.conservativeResize (params.L, params.L);
	return true;
}

void Precode_Matrix::decode_phase3 (const DenseMtx &X, DenseMtx &D,
															const uint16_t i)
{
	// rfc 6330, pg 35:
	//	To this end, the matrix X is
	//	multiplied with the submatrix of A consisting of the first i rows of
	//	A. After this operation, the submatrix of A consisting of the
	//	intersection of the first i rows and columns equals to X, whereas the
	//	matrix U_upper is transformed to a sparse form.
	const auto sub_X = X.block (0, 0, i, i);
	auto sub_A = A.block (0, 0, i, A.cols());
	sub_A = sub_X * sub_A;

	// Now fix D, too
	DenseMtx D_2 = D;

	for (uint16_t row = 0; row < sub_X.rows(); ++row) {
		D.row (row) = sub_X.row (row) * D_2.block (0,0, sub_X.cols(), D.cols());
	}
}

void Precode_Matrix::decode_phase4 (DenseMtx &D, const uint16_t i,
															const uint16_t u)
{
	// rfc 6330, pg 35:
	// For each of the first i rows of U_upper, do the following: if the row
	// has a nonzero entry at position j, and if the value of that nonzero
	// entry is b, then add to this row b times row j of I_u

	// basically: zero out U_upper. we still need to update D each time, though.

	auto U_upper = A.block (0, A.cols() - u, i, u);
	for (uint16_t row = 0; row < U_upper.rows(); ++row) {
		for (uint16_t col = 0; col < U_upper.cols(); ++col) {
			// col == j
			auto multiple = U_upper (row, col);
			if (static_cast<uint8_t> (multiple) != 0) {
				// U_upper is never read again, so we can avoid some writes
				//U_upper (row, col) = 0;

				// "b times row j of I_u" => row "j" in U_lower.
				// aka: U_upper.rows() + j
				D.row (row) += D.row (
								static_cast<uint16_t> (U_upper.rows()) + col) *
																	multiple;
			}
		}
	}
}

void Precode_Matrix::decode_phase5 (DenseMtx &D, const uint16_t i)
{
	// rc 6330, pg 36
	for (uint16_t j = 0; j <= i; ++j) {
		if (static_cast<uint8_t> (A (j, j)) != 1) {
			// A(j, j) is actually never 0, by construction.
			const auto multiple = A (j, j);
			A.row (j) /= multiple;
			D.row (j) /= multiple;
		}
		for (uint16_t tmp = 0; tmp < j; ++tmp) {	//tmp == "l" in rfc6330
			const auto multiple = A (j, tmp);
			if (static_cast<uint8_t> (multiple) != 0) {
				A.row (j) += A.row (tmp) * multiple;
				D.row (j) += D.row (tmp) * multiple;
			}
		}
	}
}

DenseMtx Precode_Matrix::encode (const DenseMtx &C, const uint32_t ISI) const
{
	// Generate repair symbols. same algorithm as "get_idxs"
	// rfc6330, pg29

	DenseMtx ret;

	ret = DenseMtx (1, C.cols());
	Tuple t = _params.tuple (ISI);

	ret.row (0) = C.row (t.b);

	for (uint16_t j = 1; j < t.d; ++j) {
		t.b = (t.b + t.a) % _params.W;
		ret.row (0) += C.row (t.b);
	}
	while (t.b1 >= _params.P)
		t.b1 = (t.b1 + t.a1) % _params.P1;

	ret.row (0) += C.row (_params.W + t.b1);
	for (uint16_t j = 1; j < t.d1; ++j) {
		t.b1 = (t.b1 + t.a1) % _params.P1;
		while (t.b1 >= _params.P)
			t.b1 = (t.b1 + t.a1) % _params.P1;
		ret.row (0) += C.row (_params.W + t.b1);
	}

	return ret;
}

}	// namespace RaptorQ
}	// namespace Impl
