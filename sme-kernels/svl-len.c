#include <stdio.h>
// #include <sys/sysctl.h>

size_t get_sme_vector_length(void) {
  size_t vl;

  __asm__ __volatile__ (
    "smstart          \n"
    "rdvl %[vl], #1   \n"
    "smstop           \n"
    : [vl] "=r" (vl)
  );

  return vl;
}

int main(void) {
    size_t vl = get_sme_vector_length();
    printf("SME vector length: %zu\n", vl);

    #if defined(__ARM_FEATURE_SME2)
    printf("Vectorization enabled: SME is supported!\n");
    #endif

    return 0;
}
