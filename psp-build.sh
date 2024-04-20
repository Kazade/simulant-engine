docker run -v `pwd`:`pwd`:Z kazade/psp-sdk /bin/sh -c "cd `pwd`/pbuild; psp-cmake -DCMAKE_BUILD_TYPE=Release -DSIMULANT_PROFILE=ON .. && make -j12"
