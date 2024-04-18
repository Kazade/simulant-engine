docker run -v `pwd`:`pwd`:Z kazade/psp-sdk /bin/sh -c "cd `pwd`/pbuild; psp-cmake .. && make -j12"
