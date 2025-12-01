# strip exif

```strip_exif``` command line utility for removing all EXIF data from jpg files.

Note that this leaves IPTC data (administrative data, such as copyright) in place.

# requirements

* cmake >= 3.30
* C++ >= 17 compiler

# building

    mkdir bld run
    cd bld
    cmake -DCMAKE_INSTALL_PREFIX=../run ../src
    make install

# testing

    cd bld
    ctest

# license

MIT license
