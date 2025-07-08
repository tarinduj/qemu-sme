# build qemu
mkdir build && cd build
../configure -target-list=aarch64-linux-user -enable-tcg
make -j$(nproc)

# build test
clang++ -march=armv9.2-a+sme2 test.cpp
# run test
/mnt/qemu-sme/build/qemu-aarch64 -cpu max,sme=on a.out 