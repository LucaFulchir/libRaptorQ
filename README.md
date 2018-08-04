# RaptorQ #


[![build status](https://www.fenrirproject.org/Luker/libRaptorQ/badges/master/build.svg)](https://www.fenrirproject.org/Luker/libRaptorQ/commits/master)  
[![Donate](https://img.shields.io/badge/Donate-PayPal-blue.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=79L8P7TXWFQK4)
[![Donate](https://img.shields.io/badge/Donate-Patreon-orange.svg)](https://patreon.com/Luker)  
[Homepage](https://www.fenrirproject.org/Luker/libRaptorQ/wikis/home)


stable  release: **v0.1.10**  
> Note: v0.1.10 is not compatible with prefivous versions, which were not RFC compliant!

current release: **v1.0.0-rc1**  

RaptorQ is a **Forward Error Correction** algorithm designed to deliver your data
efficiently and without retransmissions for lost packets.

After sending K packets of your data as-is, RaptorQ generates as many repair
symbols as you need. Once the receiver has at least K symbols, be it the source
symbols, repair symbols or any combination of the two, it can reconstruct the
whole input it was meant to receive.

This is called a Fountain code, and RaptorQ is the latest and most efficient
code in this category.


**libRaptorQ** implements **RFC6330**, which specifies the RaptorQ algorithm.  
Since the RFC is really complex, there is also a simpler (and maybe slightly faster)
RAW API that you can use.  The RFC API is not recommended due to the RFC complexity.

libRaptorQ is a header-only library written in *C++11*, and uses eigen to handle matrix
manipulation.  
Although header-only, the library can be compiled to create shared and static libraries
with both C and C++98 compatibility.


Currently it's only been tested under Linux, but should work well under
*BSD and MacOSX, too.

Unfortunately Windows is not supported. support for <future> is broken,
so you can not compile it with Visual Studio 2015.

## Features

- RFC6330 API (complex, not recommended)
- RAW API (simpler, recommended)
- multi language:
  - header only C++11
  - compiled, C
  - compiled, C++98
- cached precomputations (configurable)


## Developers ##

See the [CONTRIBUTING](CONTRIBUTING.md) file.

## The Source Code ##

Although things seems to work, no stable release has been released yet.

This means you can only check this out with git.

to check out the repository:

``$ git clone https://github.com/LucaFulchir/libRaptorQ.git``

you can also get it from our main server:

``$ git clone https://www.fenrirproject.org/Luker/libRaptorQ.git``


### GPG source check ###

Once you have cloned it, it's always a good thing to check the repository gpg
signatures, so you can import my key with:

new long term key: **7393 DAD2 55BE B575 1DBD A04A B11C D823 BA27 8C85**  
``$ gpg --keyserver pgp.mit.edu --recv-key 7393DAD255BEB5751DBDA04AB11CD823BA278C85``  
2016 key:  
``$ gpg --keyserver pgp.mit.edu --recv-key F61F6137``  
2015 key:  
``$ gpg --keyserver pgp.mit.edu --recv-key D42DDF0A``  

please check the full fingerprint.

Now you have the source, and the key, it's enough to check the signature of the
last commit:

``$ git log -n 1 --show-signature``

As long as you got the right key, and you find the "gpg: Goog signature",
you can be sure you have the right code.


## Install ##

#### Dependencies ####

libRaptorQ depends from **Eigen3** and **LZ4**  
In case your system does not have Eigen3, Eigenv3.2.8 is included
in the source files, so you do not need it.  
LZ4 is included as a git submodule, so if you do not have it, run:  
```git submodule init```  
```git submodule update```  
To get the library, which will be statically linked and **NOT INSTALLED**
on your system.

#### Building ####

The build system uses CMake. So enter the source directory and we'll create a
directory and build everything there:

```bash
$ mkdir build
$ cd build

$ cmake -DCMAKE_BUILD_TYPE=Release ../

$ make -j 4
```

Optional targets are available:  
``$ make tests examples docs``  
where:  
* tests: benchmarks, rfc tests.
* examples: C/C++ examples
* docs: LATEX documentation.

or simply:  
``$ make -j 4 everything``

finally, install everything  
``$ make install``

libRaptorQ uses **deterministic (reproducible) builds**,
so if you compile it twice, or on two different computers
(but with the same compiler), the hash of the resulting
libraries will be the same.

_There are combinations of compiler and LTO/Profiling that
break deterministic builds, so check the cmake warnings._

You can customize the CMake build with the following variables:

```
PROFILING   ON/OFF: Default:ON. Activate or deactivate profiling.
                Profiling compiles everything, then runs a test to see
                which code paths are more used. Then it recompiles everything
                but optimizing for for those code paths.
                Only for gcc/clang.
LTO         ON/OFF: Default:ON. Activate or deactivate Link time Optimization
                Makes the library smaller and better optimized.
                Only for gcc/clang.
CLANG_STDLIB ON/OFF: Default:OFF. use clang's libc++
                Note: only clang can use its standard library
USE_LZ4     ON/OFF: use the lz4 compression for caching precomputations.
                Default: ON
CLI         ON/OFF Build Command Line Interface tools.
CMAKE_C_COMPILER    gcc, clang...
CMAKE_CXX_COMPILER  choose between g++ or clang++.
RQ_LINKER   gold/ld/bsd Choose your linker. Default:autodetect.
CMAKE_BUILD_TYPE    Debug,MinSizeRel,Release,RelWithDebInfo
CMAKE_INSTALL_PREFIX Default: /usr/local
```

## Using libRaptorQ ##

For the C++11, header-only version, you can include:
 - "RaptorQ/RaptorQ_v1_hdr.hpp"
 - "RaptorQ/RFC6330_v1_hdr.hpp"

For the linked, C+98/C++11 API:
 - "RaptorQ/RaptorQ_v1.hpp"
 - "RaptorQ/RFC6330_v1.hpp"

For the linked, C API:
 - "RaptorQ/RaptorQ.h"
 - "RaptorQ/RFC6330.h"


The C++ api is completely in sync between the linked and header-only version,
so you can switch between the two just by changing the included header file.

You can compile a PDF of the documentation by doing:  
``$ make docs``

Or you can simply visit the [wiki](https://www.fenrirproject.org/Luker/libRaptorQ/wikis/libRaptorQ.pdf)
for the full up-to-date documentation.



