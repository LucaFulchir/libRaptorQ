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

#ifdef __cplusplus
extern "C"
{
#endif
	typedef enum { RQ_NONE = 0, RQ_ENC_8 = 1, RQ_ENC_16 = 2, RQ_ENC_32 = 3,
					RQ_ENC_64 = 4, RQ_DEC_8 = 5, RQ_DEC_16 = 6, RQ_DEC_32 = 7,
					RQ_DEC_64 = 8 } RaptorQ_type;


#ifdef __cplusplus
}	// extern "C"
#endif
