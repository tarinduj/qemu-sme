#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

float fmopa(float*& input, float*& output) {
    __asm__ __volatile__(
        "smstart                                                  \n" // Start SME
        
        "mov x15, #0                                              \n" // Zero register
        "mov x1, %[input_arr]                                     \n" // input_arr pointer
        "mov x2, %[output_arr]                                    \n" // output_arr pointer

        "ptrue	p0.s                                              \n" // Predicate vector

        "ld1w z0.s, p0/z, [x1, x15, lsl #2]                       \n" // Store input into z0
        "fmopa	za0.s, p0/m, p0/m, z0.s, z0.s                     \n" // fmopa

        "dup z1.s, #2                                             \n"
        // "fscale za0h.s, p0/m, z1.s                                \n"
        // ".inst 0b10000000000000000100000000100000                 \n"

        // Save ZA0 to output
        // Initialize registers
        "mov w15, #0                                              \n" // Loop counter i = 0
        "mov x9, #0                                               \n" // Offset in source matrix

        // Loop label
        "1:                                                       \n"
        "cmp w15, #8                                             \n" // Compare i with nrows
        "b.ge 2f                                                  \n" // If i >= nrows, exit loop

        // Store ith row of ZA into matrix
        "st1w {za0h.s[w15, 0]}, p0, [x2, x9, lsl #2]              \n"

        // Increment loop counter
        "add w15, w15, #1                                         \n" // i++
        "add x9, x9, #8                                          \n" // Increment offset by number of columns

        // Loop back
        "b 1b                                                     \n"

        // Loop exit label
        "2:                                                       \n"

        "smstop                                                   \n" // Stop SME
    :
    :   [input_arr] "r" (input),
        [output_arr] "r" (output)
    : 
    );

    float res = input[0] * input[0];
    return res;
}

int main() {
    // SME 
    float* input = new float[8];
    for (int i = 0; i < 8; ++i) {
        input[i] = 1;
    }

    float* output = new float[8*8];
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            output[i * 8 + j] = 5;
        }
    }

    std::cout << "input: \n";
    for (int i = 0; i < 8; ++i) {
        std::cout << input[i] << " ";
    } std::cout << std::endl;

    std::cout << "output before: \n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            std::cout << output[i * 8 + j] << " ";
        }
        std::cout << std::endl;
    } std::cout << std::endl;

    float x = fmopa(input, output);

    std::cout << "output after: \n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            std::cout << output[i * 8 + j] << " ";
        }
        std::cout << std::endl;
    } std::cout << std::endl;

    std::cout << "x: " << x << std::endl;
}