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

#include "RaptorQ/v1/common.hpp"
#include "RaptorQ/v1/block_sizes.hpp"
#include <tuple>
#include <array>

namespace RaptorQ__v1 {
namespace Impl {

constexpr uint16_t table_size = 477;

#ifdef USING_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif  //using_clang

using rq_tuple16 = std::tuple<uint16_t, uint16_t, uint16_t>;

static const uint16_t K_max = K_padded[table_size - 1];

static const std::array<uint16_t, table_size> J_K_padded = {
    254, 630, 682, 293,  80, 566, 860, 267, 822, 506, 589,  87, 520, 159, 235,
    157, 502, 334, 583,  66, 352, 365, 562,   5, 603, 721,  28, 660, 829, 900,
    930, 814, 661, 693, 780, 605, 551, 777, 491, 396, 764, 843, 646, 557, 608,
    265, 505, 722, 263, 999, 874, 160, 575, 210, 513, 503, 558, 932, 404, 520,
    846, 485, 728, 554, 471, 641, 732, 193, 934, 864, 790, 912, 617, 587, 800,
    923, 998,  92, 497, 559, 667, 912, 262, 152, 526, 268, 212,  45, 898, 527,
    558, 460,   5, 895, 996, 282, 513, 865, 870, 239, 452, 862, 852, 643, 543,
    447, 321, 287,  12, 251,  30, 621, 555, 127, 400,  91, 916, 935, 691, 299,
    282, 824, 536, 596,  28, 947, 162, 536,1000, 251, 673, 559, 923,  81, 478,
    198, 137,  75,  29, 231, 532,  58,  60, 964, 624, 502, 636, 986, 950, 735,
    866, 203,  83,  14, 522, 226, 282,  88, 636, 860, 324, 424, 999, 682, 814,
    979, 538, 278, 580, 773, 911, 506, 628, 282, 309, 858, 442, 654,  82, 428,
    442, 283, 538, 189, 438, 912,   1, 167, 272, 209, 927, 386, 653, 669, 431,
    793, 588, 777, 939, 864, 627, 265, 976, 988, 507, 640,  15, 667,  24, 877,
    240, 720,  93, 919, 635, 174, 647, 820,  56, 485, 210, 124, 546, 954, 262,
    927, 957, 726, 583, 782,  37, 758, 777, 104, 476, 113, 313, 102, 501, 332,
    786,  99, 658, 794,  37, 471,  94, 873, 918, 945, 211, 341,  11, 578, 494,
    694, 252, 451,  83, 689, 488, 214,  17, 469, 263, 309, 984, 123, 360, 863,
    122, 522, 539, 181,  64, 387, 967, 843, 999,  76, 142, 599, 576, 176, 392,
    332, 291, 913, 608, 212, 696, 931, 326, 228, 706, 144,  83, 743, 187, 654,
    359, 493, 369, 981, 276, 647, 389,  80, 396, 580, 873,  15, 976, 584, 267,
    876, 642, 794,  78, 736, 882, 251, 434, 204, 256, 106, 375, 148, 496, 88,
    826,  71, 925, 760, 130, 641, 400, 480,  76, 665, 910, 467, 964, 625, 362,
    759, 728, 343, 113, 137, 308, 800, 177, 961, 958,  72, 732, 145, 577, 305,
     50, 351, 175, 727, 902, 409, 776, 586, 451, 287, 246, 222, 563, 839, 897,
    409, 618, 439,  95, 448, 133, 938, 423,  90, 640, 922, 250, 367, 447, 559,
    121, 623, 450, 253, 106, 863, 148, 427, 138, 794, 247, 562,  53, 135,  21,
    201, 169,  70, 386, 226,   3, 769, 590, 672, 713, 967, 368, 348, 119, 503,
    181, 394, 189, 210,  62, 273, 554, 936, 483, 397, 241, 500,  12, 958, 524,
      8, 100, 339, 804, 510,  18, 412, 394, 830, 535, 199,  27, 298, 368, 755,
    379,  73, 387, 457, 761, 855, 370, 261, 299, 920, 269, 862, 349, 103, 115,
     93, 982, 432, 340, 173, 421, 330, 624, 233, 362, 963, 471};

static const std::array<rq_tuple16, table_size> S_H_W = {
    rq_tuple16(7, 10, 17), rq_tuple16(7, 10, 19), rq_tuple16(11, 10, 29),
    rq_tuple16(11, 10, 31), rq_tuple16(11, 10, 37), rq_tuple16(11, 10, 41),
    rq_tuple16(11, 10, 43), rq_tuple16(11, 10, 47), rq_tuple16(11, 10, 53),
    rq_tuple16(13, 10, 59), rq_tuple16(13, 10, 61), rq_tuple16(13, 10, 61),
    rq_tuple16(13, 10, 67), rq_tuple16(13, 10, 71), rq_tuple16(13, 10, 73),
    rq_tuple16(13, 10, 79), rq_tuple16(17, 10, 89), rq_tuple16(17, 10, 97),
    rq_tuple16(17, 10, 101), rq_tuple16(17, 10, 103), rq_tuple16(17, 10, 107),
    rq_tuple16(17, 10, 109), rq_tuple16(17, 10, 113), rq_tuple16(19, 10, 127),
    rq_tuple16(19, 10, 131), rq_tuple16(19, 10, 137), rq_tuple16(19, 10, 139),
    rq_tuple16(19, 10, 149), rq_tuple16(19, 10, 151), rq_tuple16(23, 10, 163),
    rq_tuple16(23, 10, 167), rq_tuple16(23, 10, 173), rq_tuple16(23, 10, 179),
    rq_tuple16(23, 10, 181), rq_tuple16(23, 10, 191), rq_tuple16(23, 10, 193),
    rq_tuple16(23, 10, 197), rq_tuple16(23, 10, 199), rq_tuple16(23, 10, 211),
    rq_tuple16(23, 10, 223), rq_tuple16(29, 10, 233), rq_tuple16(29, 10, 241),
    rq_tuple16(29, 10, 251), rq_tuple16(29, 10, 257), rq_tuple16(29, 10, 263),
    rq_tuple16(29, 10, 271), rq_tuple16(29, 10, 277), rq_tuple16(29, 10, 283),
    rq_tuple16(29, 10, 293), rq_tuple16(29, 10, 307), rq_tuple16(29, 10, 313),
    rq_tuple16(29, 10, 317), rq_tuple16(31, 10, 337), rq_tuple16(31, 10, 349),
    rq_tuple16(31, 10, 353), rq_tuple16(31, 10, 359), rq_tuple16(31, 10, 367),
    rq_tuple16(31, 10, 373), rq_tuple16(31, 10, 379), rq_tuple16(37, 10, 389),
    rq_tuple16(37, 10, 397), rq_tuple16(37, 10, 401), rq_tuple16(37, 10, 409),
    rq_tuple16(37, 10, 421), rq_tuple16(37, 10, 433), rq_tuple16(37, 10, 443),
    rq_tuple16(37, 10, 449), rq_tuple16(37, 10, 461), rq_tuple16(37, 10, 467),
    rq_tuple16(37, 10, 479), rq_tuple16(37, 10, 491), rq_tuple16(37, 10, 499),
    rq_tuple16(37, 10, 503), rq_tuple16(37, 10, 509), rq_tuple16(37, 10, 523),
    rq_tuple16(41, 10, 541), rq_tuple16(41, 10, 547), rq_tuple16(41, 10, 557),
    rq_tuple16(41, 10, 563), rq_tuple16(41, 10, 571), rq_tuple16(41, 10, 577),
    rq_tuple16(41, 10, 587), rq_tuple16(41, 10, 593), rq_tuple16(41, 10, 601),
    rq_tuple16(41, 10, 607), rq_tuple16(41, 10, 613), rq_tuple16(41, 10, 619),
    rq_tuple16(41, 10, 631), rq_tuple16(43, 10, 647), rq_tuple16(43, 10, 653),
    rq_tuple16(43, 10, 661), rq_tuple16(47, 10, 683), rq_tuple16(47, 10, 691),
    rq_tuple16(47, 10, 701), rq_tuple16(47, 10, 709), rq_tuple16(47, 10, 719),
    rq_tuple16(47, 10, 733), rq_tuple16(47, 10, 743), rq_tuple16(47, 10, 751),
    rq_tuple16(47, 10, 761), rq_tuple16(47, 10, 773), rq_tuple16(53, 10, 797),
    rq_tuple16(53, 10, 811), rq_tuple16(53, 10, 821), rq_tuple16(53, 10, 829),
    rq_tuple16(53, 10, 839), rq_tuple16(53, 10, 853), rq_tuple16(53, 10, 863),
    rq_tuple16(53, 10, 877), rq_tuple16(53, 10, 887), rq_tuple16(53, 10, 907),
    rq_tuple16(53, 10, 919), rq_tuple16(53, 10, 929), rq_tuple16(53, 10, 941),
    rq_tuple16(53, 10, 953), rq_tuple16(59, 10, 971), rq_tuple16(59, 10, 983),
    rq_tuple16(59, 10, 997), rq_tuple16(59, 10, 1009), rq_tuple16(59, 10, 1021),
    rq_tuple16(59, 10, 1039), rq_tuple16(59, 10, 1051),rq_tuple16(59, 11, 1069),
    rq_tuple16(59, 11, 1093), rq_tuple16(59, 11, 1103),rq_tuple16(59, 11, 1117),
    rq_tuple16(59, 11, 1129), rq_tuple16(59, 11, 1153),rq_tuple16(61, 11, 1171),
    rq_tuple16(61, 11, 1187), rq_tuple16(61, 11, 1201),rq_tuple16(61, 11, 1223),
    rq_tuple16(61, 11, 1237), rq_tuple16(67, 11, 1259),rq_tuple16(67, 11, 1277),
    rq_tuple16(67, 11, 1291), rq_tuple16(67, 11, 1307),rq_tuple16(67, 11, 1327),
    rq_tuple16(67, 11, 1367), rq_tuple16(67, 11, 1381),rq_tuple16(67, 11, 1409),
    rq_tuple16(67, 11, 1423), rq_tuple16(67, 11, 1439),rq_tuple16(71, 11, 1459),
    rq_tuple16(71, 11, 1483), rq_tuple16(71, 11, 1499),rq_tuple16(71, 11, 1523),
    rq_tuple16(71, 11, 1543), rq_tuple16(71, 11, 1559),rq_tuple16(73, 11, 1583),
    rq_tuple16(73, 11, 1601), rq_tuple16(73, 11, 1621),rq_tuple16(73, 11, 1637),
    rq_tuple16(73, 11, 1669), rq_tuple16(79, 11, 1699),rq_tuple16(79, 11, 1723),
    rq_tuple16(79, 11, 1741), rq_tuple16(79, 11, 1759),rq_tuple16(79, 11, 1783),
    rq_tuple16(79, 11, 1801), rq_tuple16(79, 11, 1823),rq_tuple16(79, 11, 1847),
    rq_tuple16(79, 11, 1867), rq_tuple16(83, 11, 1889),rq_tuple16(83, 11, 1913),
    rq_tuple16(83, 11, 1931), rq_tuple16(83, 11, 1951),rq_tuple16(83, 11, 1979),
    rq_tuple16(83, 11, 2003), rq_tuple16(83, 11, 2029),rq_tuple16(89, 11, 2069),
    rq_tuple16(89, 11, 2099), rq_tuple16(89, 11, 2131),rq_tuple16(89, 11, 2153),
    rq_tuple16(89, 11, 2179), rq_tuple16(89, 11, 2221),rq_tuple16(89, 11, 2243),
    rq_tuple16(89, 11, 2273), rq_tuple16(97, 11, 2311),rq_tuple16(97, 11, 2347),
    rq_tuple16(97, 11, 2371), rq_tuple16(97, 11, 2399),rq_tuple16(97, 11, 2423),
    rq_tuple16(97, 11, 2447), rq_tuple16(97, 11, 2477),rq_tuple16(97, 11, 2503),
    rq_tuple16(97, 11, 2531), rq_tuple16(97, 11, 2557),rq_tuple16(97, 11, 2593),
    rq_tuple16(101,11,2633), rq_tuple16(101,11,2671), rq_tuple16(101,11,2699),
    rq_tuple16(101,11,2731), rq_tuple16(101,11,2767), rq_tuple16(101,11,2801),
    rq_tuple16(103,11,2833), rq_tuple16(103,11,2861), rq_tuple16(107,11,2909),
    rq_tuple16(107,11,2939), rq_tuple16(107,11,2971), rq_tuple16(107,11,3011),
    rq_tuple16(109,11,3049), rq_tuple16(109,11,3089), rq_tuple16(113,11,3137),
    rq_tuple16(113,11,3187), rq_tuple16(113,11,3221), rq_tuple16(113,11,3259),
    rq_tuple16(113,11,3299), rq_tuple16(127,11,3347), rq_tuple16(127,11,3391),
    rq_tuple16(127,11,3433), rq_tuple16(127,11,3469), rq_tuple16(127,11,3511),
    rq_tuple16(127,11,3547), rq_tuple16(127,11,3583), rq_tuple16(127,11,3623),
    rq_tuple16(127,11,3659), rq_tuple16(127,11,3701), rq_tuple16(127,11,3739),
    rq_tuple16(127,11,3793), rq_tuple16(127,11,3833), rq_tuple16(127,11,3881),
    rq_tuple16(127,11,3923), rq_tuple16(131,11,3967), rq_tuple16(131,11,4013),
    rq_tuple16(131,11,4057), rq_tuple16(131,11,4111), rq_tuple16(137,11,4159),
    rq_tuple16(137,11,4211), rq_tuple16(137,11,4253), rq_tuple16(137,11,4297),
    rq_tuple16(137,11,4363), rq_tuple16(137,11,4409), rq_tuple16(139,11,4463),
    rq_tuple16(139,11,4513), rq_tuple16(149,11,4567), rq_tuple16(149,11,4621),
    rq_tuple16(149,11,4679), rq_tuple16(149,11,4733), rq_tuple16(149,11,4783),
    rq_tuple16(149,11,4831), rq_tuple16(149,11,4889), rq_tuple16(149,11,4951),
    rq_tuple16(149,11,5003), rq_tuple16(151,11,5059), rq_tuple16(151,11,5113),
    rq_tuple16(157,11,5171), rq_tuple16(157,11,5227), rq_tuple16(157,11,5279),
    rq_tuple16(157,11,5333), rq_tuple16(157,11,5387), rq_tuple16(157,11,5443),
    rq_tuple16(163,11,5507), rq_tuple16(163,11,5563), rq_tuple16(163,11,5623),
    rq_tuple16(163,11,5693), rq_tuple16(163,11,5749), rq_tuple16(167,11,5821),
    rq_tuple16(167,11,5881), rq_tuple16(167,11,5953), rq_tuple16(173,11,6037),
    rq_tuple16(173,11,6101), rq_tuple16(173,11,6163), rq_tuple16(173,11,6229),
    rq_tuple16(179,11,6299), rq_tuple16(179,11,6361), rq_tuple16(179,11,6427),
    rq_tuple16(179,11,6491), rq_tuple16(179,11,6581), rq_tuple16(181,11,6653),
    rq_tuple16(181,11,6719), rq_tuple16(191,11,6803), rq_tuple16(191,11,6871),
    rq_tuple16(191,11,6949), rq_tuple16(191,11,7027), rq_tuple16(191,11,7103),
    rq_tuple16(191,11,7177), rq_tuple16(191,11,7253), rq_tuple16(193,11,7351),
    rq_tuple16(197,11,7433), rq_tuple16(197,11,7517), rq_tuple16(197,11,7591),
    rq_tuple16(199,11,7669), rq_tuple16(211,11,7759), rq_tuple16(211,11,7853),
    rq_tuple16(211,11,7937), rq_tuple16(211,11,8017), rq_tuple16(211,11,8111),
    rq_tuple16(211,11,8191), rq_tuple16(211,11,8273), rq_tuple16(211,11,8369),
    rq_tuple16(223,11,8467), rq_tuple16(223,11,8563), rq_tuple16(223,11,8647),
    rq_tuple16(223,11,8741), rq_tuple16(223,11,8831), rq_tuple16(223,11,8923),
    rq_tuple16(223,11,9013), rq_tuple16(223,11,9103), rq_tuple16(227,11,9199),
    rq_tuple16(227,11,9293), rq_tuple16(229,11,9391), rq_tuple16(233,11,9491),
    rq_tuple16(233,11,9587), rq_tuple16(239,11,9697), rq_tuple16(239,11,9803),
    rq_tuple16(239,11,9907), rq_tuple16(239,11,10009), rq_tuple16(241,11,10111),
    rq_tuple16(251,11,10223), rq_tuple16(251,11,10343),rq_tuple16(251,11,10453),
    rq_tuple16(251,11,10559), rq_tuple16(251,11,10667),rq_tuple16(257,11,10781),
    rq_tuple16(257,11,10891), rq_tuple16(257,12,11003),rq_tuple16(257,12,11119),
    rq_tuple16(263,12,11239), rq_tuple16(263,12,11353),rq_tuple16(269,12,11471),
    rq_tuple16(269,12,11587), rq_tuple16(269,12,11701),rq_tuple16(269,12,11821),
    rq_tuple16(271,12,11941), rq_tuple16(277,12,12073),rq_tuple16(277,12,12203),
    rq_tuple16(277,12,12323), rq_tuple16(281,12,12451),rq_tuple16(281,12,12577),
    rq_tuple16(293,12,12721), rq_tuple16(293,12,12853),rq_tuple16(293,12,12983),
    rq_tuple16(293,12,13127), rq_tuple16(293,12,13267),rq_tuple16(307,12,13421),
    rq_tuple16(307,12,13553), rq_tuple16(307,12,13693),rq_tuple16(307,12,13829),
    rq_tuple16(307,12,13967), rq_tuple16(307,12,14107),rq_tuple16(311,12,14251),
    rq_tuple16(311,12,14407), rq_tuple16(313,12,14551),rq_tuple16(317,12,14699),
    rq_tuple16(317,12,14851), rq_tuple16(331,12,15013),rq_tuple16(331,12,15161),
    rq_tuple16(331,12,15319), rq_tuple16(331,12,15473),rq_tuple16(331,12,15643),
    rq_tuple16(337,12,15803), rq_tuple16(337,12,15959),rq_tuple16(337,12,16127),
    rq_tuple16(347,12,16319), rq_tuple16(347,12,16493),rq_tuple16(347,12,16661),
    rq_tuple16(349,12,16831), rq_tuple16(353,12,17011),rq_tuple16(353,12,17183),
    rq_tuple16(359,12,17359), rq_tuple16(359,12,17539),rq_tuple16(367,12,17729),
    rq_tuple16(367,12,17911), rq_tuple16(367,12,18097),rq_tuple16(373,12,18289),
    rq_tuple16(373,12,18481), rq_tuple16(379,12,18679),rq_tuple16(379,12,18869),
    rq_tuple16(383,12,19087), rq_tuple16(389,12,19309),rq_tuple16(389,12,19507),
    rq_tuple16(397,12,19727), rq_tuple16(397,12,19927),rq_tuple16(401,12,20129),
    rq_tuple16(401,12,20341), rq_tuple16(409,12,20551),rq_tuple16(409,12,20759),
    rq_tuple16(419,13,20983), rq_tuple16(419,13,21191),rq_tuple16(419,13,21401),
    rq_tuple16(419,13,21613), rq_tuple16(431,13,21841),rq_tuple16(431,13,22063),
    rq_tuple16(431,13,22283), rq_tuple16(433,13,22511),rq_tuple16(439,13,22751),
    rq_tuple16(439,13,22993), rq_tuple16(443,13,23227),rq_tuple16(449,13,23473),
    rq_tuple16(457,13,23719), rq_tuple16(457,13,23957),rq_tuple16(457,13,24197),
    rq_tuple16(461,13,24443), rq_tuple16(467,13,24709),rq_tuple16(467,13,24953),
    rq_tuple16(479,13,25219), rq_tuple16(479,13,25471),rq_tuple16(479,13,25733),
    rq_tuple16(487,13,26003), rq_tuple16(487,13,26267),rq_tuple16(491,13,26539),
    rq_tuple16(499,13,26821), rq_tuple16(499,13,27091),rq_tuple16(503,13,27367),
    rq_tuple16(509,13,27653), rq_tuple16(521,13,27953),rq_tuple16(521,13,28229),
    rq_tuple16(521,13,28517), rq_tuple16(523,13,28817),rq_tuple16(541,13,29131),
    rq_tuple16(541,13,29423), rq_tuple16(541,13,29717),rq_tuple16(541,13,30013),
    rq_tuple16(547,13,30323), rq_tuple16(547,13,30631),rq_tuple16(557,14,30949),
    rq_tuple16(557,14,31267), rq_tuple16(563,14,31583),rq_tuple16(569,14,31907),
    rq_tuple16(571,14,32251), rq_tuple16(577,14,32579),rq_tuple16(587,14,32917),
    rq_tuple16(587,14,33247), rq_tuple16(593,14,33601),rq_tuple16(593,14,33941),
    rq_tuple16(599,14,34283), rq_tuple16(607,14,34631),rq_tuple16(607,14,34981),
    rq_tuple16(613,14,35363), rq_tuple16(619,14,35731),rq_tuple16(631,14,36097),
    rq_tuple16(631,14,36457), rq_tuple16(641,14,36833),rq_tuple16(641,14,37201),
    rq_tuple16(643,14,37579), rq_tuple16(653,14,37967),rq_tuple16(653,14,38351),
    rq_tuple16(659,14,38749), rq_tuple16(673,14,39163),rq_tuple16(673,14,39551),
    rq_tuple16(677,14,39953), rq_tuple16(683,14,40361),rq_tuple16(691,15,40787),
    rq_tuple16(701,15,41213), rq_tuple16(701,15,41621),rq_tuple16(709,15,42043),
    rq_tuple16(709,15,42467), rq_tuple16(719,15,42899),rq_tuple16(727,15,43331),
    rq_tuple16(727,15,43801), rq_tuple16(733,15,44257),rq_tuple16(739,15,44701),
    rq_tuple16(751,15,45161), rq_tuple16(751,15,45613),rq_tuple16(757,15,46073),
    rq_tuple16(769,15,46549), rq_tuple16(769,15,47017),rq_tuple16(787,15,47507),
    rq_tuple16(787,15,47981), rq_tuple16(787,15,48463),rq_tuple16(797,15,48953),
    rq_tuple16(809,15,49451), rq_tuple16(809,15,49943),rq_tuple16(821,15,50461),
    rq_tuple16(821,16,50993), rq_tuple16(827,16,51503),rq_tuple16(839,16,52027),
    rq_tuple16(853,16,52571), rq_tuple16(853,16,53093),rq_tuple16(857,16,53623),
    rq_tuple16(863,16,54163), rq_tuple16(877,16,54713),rq_tuple16(877,16,55259),
    rq_tuple16(883,16,55817), rq_tuple16(907,16,56393),rq_tuple16(907,16,56951)
};

#ifdef USING_CLANG
#pragma clang diagnostic pop
#endif  //using_clang

}   // namespace Impl
}   // namespace RaptorQ
