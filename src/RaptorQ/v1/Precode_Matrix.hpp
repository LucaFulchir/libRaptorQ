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

#include "RaptorQ/v1/util/Bitmask.hpp"
#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/multiplication.hpp"
#include "RaptorQ/v1/Operation.hpp"
#include "RaptorQ/v1/Octet.hpp"
#include "RaptorQ/v1/Parameters.hpp"
#include "RaptorQ/v1/Thread_Pool.hpp"
#include <Eigen/Dense>
#include <deque>
#include <memory>

namespace RaptorQ__v1 {
namespace Impl {

using DenseMtx = Eigen::Matrix<Octet, Eigen::Dynamic, Eigen::Dynamic,
                                                            Eigen::RowMajor>;
enum class RAPTORQ_API Save_Computation : uint8_t {
    OFF = 0,
    ON = 1
};

enum class RAPTORQ_LOCAL Precode_Result : uint8_t {
    DONE = 0,
    STOPPED = 1,
    FAILED = 2
};

template<Save_Computation IS_OFFLINE>
class RAPTORQ_API Precode_Matrix
{
    using Op_Vec = std::deque<Operation>;
public:
    const Parameters _params;

    Precode_Matrix(const Parameters &params)
        :_params (params)
    {}
    Precode_Matrix() = delete;
    Precode_Matrix (const Precode_Matrix&) = default;
    Precode_Matrix& operator= (const Precode_Matrix&) = default;
    Precode_Matrix (Precode_Matrix&&) = default;
    Precode_Matrix& operator= (Precode_Matrix&&) = default;
    ~Precode_Matrix() = default;

    void gen (const uint32_t repair_overhead);


    std::pair<Precode_Result, DenseMtx> intermediate (DenseMtx &D, Op_Vec &ops,
                                        bool &keep_working,
                                        const Work_State *thread_keep_working);
    std::pair<Precode_Result, DenseMtx> intermediate (DenseMtx &D,
                                        const Bitmask &mask,
                                        const std::vector<uint32_t> &repair_esi,
                                        Op_Vec &ops, bool &keep_working,
                                        const Work_State *thread_keep_working);
    DenseMtx get_missing (const DenseMtx &C, const Bitmask &mask) const;
    DenseMtx encode (const DenseMtx &C, const uint32_t ISI) const;

private:
    DenseMtx A;
    uint32_t _repair_overhead = 0;

    // indenting here prepresent which function needs which other.
    // not standard, ask me if I care.
    void init_LDPC1 (DenseMtx &_A, const uint16_t S, const uint16_t B) const;
    void init_LDPC2 (DenseMtx &_A, const uint16_t skip, const uint16_t rows,
                                                    const uint16_t cols) const;
    void add_identity (DenseMtx &_A, const uint16_t size,
                                                const uint16_t skip_row,
                                                const uint16_t skip_col) const;

    void init_HDPC (DenseMtx &_A) const;
        DenseMtx make_MT() const;       // rfc 6330, pgg 24, used for HDPC
        DenseMtx make_GAMMA() const;    // rfc 6330, pgg 24, used for HDPC
    void add_G_ENC (DenseMtx &_A) const;

    //DenseMtx intermediate (DenseMtx &D, Op_Vec &ops, bool &keep_working);
    void decode_phase0 (const Bitmask &mask,
                                    const std::vector<uint32_t> &repair_esi);
    std::tuple<bool, uint16_t, uint16_t> decode_phase1 (DenseMtx &X,DenseMtx &D,
                                        std::vector<uint16_t> &c,
                                        Op_Vec &ops, bool &keep_working,
                                        const Work_State *thread_keep_working);
    bool decode_phase2 (DenseMtx &D, const uint16_t i,const uint16_t u,
                                        Op_Vec &ops, bool &keep_working,
                                        const Work_State *thread_keep_working);
    void decode_phase3 (const DenseMtx &X, DenseMtx &D, const uint16_t i,
                                        Op_Vec &ops);
    void decode_phase4 (DenseMtx &D, const uint16_t i, const uint16_t u,
                                        Op_Vec &ops, bool &keep_working,
                                        const Work_State *thread_keep_working);
    void decode_phase5 (DenseMtx &D, const uint16_t i, Op_Vec &ops,
                                        bool &keep_working,
                                        const Work_State *thread_keep_working);

    inline bool stop (bool keep_working, const Work_State *thread_keep_working)
    {
        return false == keep_working ||
                            Work_State::KEEP_WORKING != *thread_keep_working;
    }
};

}   // namespace Impl
}   // namespace RaptorQ

#include "Precode_Matrix_Init.hpp" // above template implementation
#include "Precode_Matrix_Solver.hpp" // above template implementation
