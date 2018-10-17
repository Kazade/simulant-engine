sh -c "mkdir -p buildw; cd buildw; cmake -DCMAKE_TOOLCHAIN_FILE=/home/dev/dc/simulant-engine/toolchains/Mingw.cmake -DCMAKE_BUILD_TYPE=Release  .. &&  make -j5"
