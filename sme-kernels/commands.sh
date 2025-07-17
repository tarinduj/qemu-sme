# build qemu
mkdir build && cd build
../configure -target-list=aarch64-linux-user -enable-tcg
make -j$(nproc)

# build test
clang++ -march=armv9.2-a+sme2 test.cpp
# run test
/mnt/qemu-sme/build/qemu-aarch64 -cpu max,sme=on a.out 

#512 bit/ 64 byte SVL
clang++ -march=armv9.2-a+sme2 -O3 pivot.cpp -o pivot.bin
/mnt/qemu-sme/build/qemu-aarch64 -cpu max,sme=on,sme-default-vector-length=64 pivot.bin 16 16 0 2 100000