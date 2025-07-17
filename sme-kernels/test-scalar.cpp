#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

int smopa(int*& input, int*& output) {
    int* scale_arr = new int[8];
    for (int i = 0; i < 8; ++i) {
        scale_arr[i] = i+1;
    }

    __asm__ __volatile__(
        "smstart                                                  \n" // Start SME
        
        "mov x15, #0                                              \n" // Zero register
        "mov x1, %[input_arr]                                     \n" // input_arr pointer
        "mov x2, %[output_arr]                                    \n" // output_arr pointer
        "mov x3, %[scale_arr]                                     \n" // scale_arr pointer

        "ptrue	p0.s                                              \n" // Predicate vector

        "ld1w z0.s, p0/z, [x1, x15, lsl #2]                       \n" // Store input into z0
        
        //  dup      z0.s, #1.0   // z0 = 1.0f replicated
        // zero     ZA0.S        // clear the whole tile         :contentReference[oaicite:0]{index=0}
        "addha za0.s, p0/m, p0/m, z0.s                            \n" 

        // scale array
        "ld1w z1.s, p0/z, [x3, x15, lsl #2]                       \n" // Store scale_arr into z1
        "mov x0, #6                                               \n"
        "whilelo p0.s, xzr, x0                                    \n"

        // "sscale za0h.s, p0/m, z1.s                                \n"
        ".inst 0b10000000000110000000000000100000                 \n" //horizontal
        // ".inst 0b10000000000110001000000000100000                 \n" //vertical
        "ptrue p0.s                                               \n"
        
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
        [output_arr] "r" (output),
        [scale_arr] "r" (scale_arr)
    : 
    );

    int res = input[0] * input[0];
    return res;
}

int main() {
    // SME 
    int* input = new int[8];
    for (int i = 0; i < 8; ++i) {
        input[i] = 1;
    }

    int* output = new int[8*8];
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

    int x = smopa(input, output);

    std::cout << "output after: \n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            std::cout << output[i * 8 + j] << " ";
        }
        std::cout << std::endl;
    } std::cout << std::endl;

    std::cout << "x: " << x << std::endl;
}