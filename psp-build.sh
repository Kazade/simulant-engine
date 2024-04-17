docker run -v `pwd`:`pwd`:Z kazade/psp-sdk /bin/sh -c "cd `pwd`/pbuild; cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/PSP.cmake .. && make -j12"
