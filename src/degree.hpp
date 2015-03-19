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

#ifndef RAPTORQ_DEGREE_HPP
#define RAPTORQ_DEGREE_HPP

#include <array>

namespace RaptorQ {
namespace Impl {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"

static const std::array<uint32_t, 31> degree_distribution = {
							  0,    5243,  529531,  704294,  791675,  844104,
						 879057,  904023,  922747,  937311,  948962,  958494,
						 966438,  973160,  978921,  983914,  988283,  992138,
						 995565,  998631, 1001391, 1003887, 1006157, 1008229,
						1010129, 1011876, 1013490, 1014983, 1016370, 1017662,
						1048576};

#pragma clang diagnostic pop

}	// namespace Impl
}	// namespace RaptorQ

#endif

