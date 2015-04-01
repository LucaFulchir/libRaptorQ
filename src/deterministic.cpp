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

#include <array>
#include <cstdio>
#include <iostream>


int main (int argc, char **argv)
{
	const char date[11] = "0000000000";
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " libRaptorQ_static.a\n";
		return 1;
	}
	FILE *static_lib;

	static_lib = fopen (argv[1], "r+");
	if (static_lib == nullptr) {
		std::cout << "Usage: " << argv[0] << " libRaptorQ_static.a\n";
		return 1;
	}
	if (fseek (static_lib, 24, 0) != 0) {
		std::cout << "Can not seek\n";
		fclose(static_lib);
		return 1;
	}
	if (10 != fwrite (date, sizeof(char), 10, static_lib)) {
		std::cout << "Can not write all data\n";
		fclose(static_lib);
		return 1;
	}
	std::cout << "All ok\n";
	fclose(static_lib);
	return 0;
}
