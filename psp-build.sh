docker run -v `pwd`:`pwd`:Z kazade/psp-sdk /bin/sh -c "cd `pwd`/pbuild; psp-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_PRX=1 -DENC_PRX=1 -DSIMULANT_PROFILE=ON .. && make -j12"
