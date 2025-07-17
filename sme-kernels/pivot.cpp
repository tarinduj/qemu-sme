#include <cassert>
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <random>
#include <iostream>
#include <chrono>
#include <fstream>

#define SWAP 1

#define SME_START_STOP 1
#define SME_HELPERS 1
#define SME_MATRIX_LOAD 0
#define SME_MATRIX_STORE 0
#define SME_SAVE_ROWS_COLS 0
#define SME_MULTIPLY_ROWS 0
#define SME_OUTER_PRODUCT 0

#define MASKED_COLUMN_MULTIPLIES 1
#define MASKED_MULTIPLY_ADD 1

#if MASKED_MULTIPLY_ADD
  #undef OUTER_PRODUCT
  #define OUTER_PRODUCT 1
#endif

#if SME_MATRIX_LOAD_STORE
  #undef SME_HELPERS
  #define SME_HELPERS 1
#endif


inline unsigned nextPowOfTwo(unsigned n) {
  unsigned ret = 1;
  while (n > ret)
    ret *= 2;
  return ret;
}

template <typename Int>
class Matrix {
public:
  Matrix() = delete;

  Matrix(unsigned rows, unsigned columns)
      : nRows(rows), nColumns(columns), nReservedColumns(16), data(nRows * nReservedColumns) {}

  __attribute__((always_inline))
  Int &at(unsigned row, unsigned column) {
    assert(row < getNumRows() && "Row outside of range");
    assert(column < getNumColumns() && "Column outside of range");
    return data[row * nReservedColumns + column];
  }

  __attribute__((always_inline))
  Int at(unsigned row, unsigned column) const {
    assert(row < getNumRows() && "Row outside of range");
    assert(column < getNumColumns() && "Column outside of range");
    return data[row * nReservedColumns + column];
  }

  __attribute__((always_inline))
  Int &operator()(unsigned row, unsigned column) {
    return at(row, column);
  }

  __attribute__((always_inline))
  Int operator()(unsigned row, unsigned column) const {
    return at(row, column);
  }

  Int* getDataPointer() {
    return data.data();
  }

  void randomInit(int lb, int ub) {
    // std::default_random_engine e;
    // std::uniform_int_distribution<int> dist(lb, ub);

    for (int i = 0; i < nRows; ++i) {
      for (int j = 1; j < nColumns; ++j) {
        (*this)(i, j) = 1;
      }
      // Additional adjustment for the 0-th column
      (*this)(i, 0) = 1;
      (*this)(i, 2) = 1;
    }
    // Original code set m[0][2] = 3;
    // (*this)(0, 2) = 1;
  }

  void custominit(const std::vector<std::vector<int>> &input) {
    for (int i = 0; i < nRows; ++i) {
      for (int j = 0; j < nColumns; ++j) {
            (*this)(i, j) = input[i][j];
        }
    }
  }

  unsigned getNumRows() const { return nRows; }
  unsigned getNumColumns() const { return nColumns; }
  unsigned getNReservedColumns() const { return nReservedColumns; }

  void dump() const {
    std::cout << "Matrix " << nRows << "x" << nColumns << ":\n";
    for (unsigned row = 0; row < nRows; ++row) {
      for (unsigned column = 0; column < nColumns; ++column)
        std::cout << at(row, column) << '\t';
      std::cout << '\n';
    }
  }

  void dumpMatrixToFile(const std::string &filename = "matrix-dump.txt") {
    std::ofstream outfile(filename);
    std::streambuf *coutbuf = std::cout.rdbuf(); // Save old buffer
    std::cout.rdbuf(outfile.rdbuf()); // Redirect std::cout to outfile

    dump();

    std::cout.rdbuf(coutbuf); // Reset to standard output again
    outfile.close();
  }

private:
  unsigned nRows, nColumns, nReservedColumns;
  using VectorType = std::vector<Int>;
  VectorType data;
};

// Pivot function outside of the Matrix class
template <typename Int>
__attribute__((noinline))
void pivotBaseline(Matrix<Int> &mat, int repeat, int pivotRow, int pivotCol, int nPivots) {
  for (int i = 0; i < repeat; i++) {
    #if SWAP
    // swap
    std::swap(mat(pivotRow, 0), mat(pivotRow, pivotCol));

    if (mat(pivotRow, 0) < 0) {
      mat(pivotRow, 0) = -mat(pivotRow, 0);
      mat(pivotRow, pivotCol) = -mat(pivotRow, pivotCol);
    } else {
      for (int col = 1; col < mat.getNumColumns(); ++col) {
        if (col != pivotCol)
          mat(pivotRow, col) = -mat(pivotRow, col);
      }
    }
    #endif

    for (int row = 0; row < mat.getNumRows(); ++row) {
      if (row == pivotRow || mat(row, pivotCol) == 0)
        continue;

      #if MASKED_COLUMN_MULTIPLIES
      /* masked column multiplies 1 */
      mat(row, 0) *= mat(pivotRow, 0);
      #endif

      for (int col = 1; col < mat.getNumColumns(); ++col) {
        if (col != pivotCol) {
          #if MASKED_MULTIPLY_ADD
          /* masked multiply add + outer product */
          mat(row, col) = mat(row, col) * mat(pivotRow, 0) + mat(row, pivotCol) * mat(pivotRow, col);
          #elif OUTER_PRODUCT
          /* outer product ONLY */
          mat(row, col) = mat(row, pivotCol) * mat(pivotRow, col);
          #endif
        }
      }

      #if MASKED_COLUMN_MULTIPLIES
      /* masked column multiplies 2 */
      mat(row, pivotCol) *= mat(pivotRow, pivotCol);
      #endif
    }
  }
}

// Pivot function outside of the Matrix class
template <typename Int>
__attribute__((noinline))
void pivotNew(Matrix<Int> &mat, int repeat, int pivotRow, int pivotCol, int nPivots) {
  for (int i = 0; i < repeat; i++) {
    #if SWAP
    // swap
    Int tmp = mat(pivotRow, 0);
    mat(pivotRow, 0) = -mat(pivotRow, pivotCol);
    mat(pivotRow, pivotCol) = -tmp;

    if (mat(pivotRow, 0) < 0) {
      for (int j = 0; j < mat.getNumColumns(); j++) {
        mat(pivotRow, j) = -mat(pivotRow, j);
      }
    }
    #endif

    // // dump swap
    // std::cout << "Swap:\n";
    // mat.dump();
    // std::cout << "--------------------------------\n";

    #if OUTER_PRODUCT
    // matrix outer product
    #if MASKED_MULTIPLY_ADD
    Matrix<Int> outerProduct(mat.getNumRows(), mat.getNumColumns());
    #endif
    for (int row = 0; row < mat.getNumRows(); ++row) {
      for (int col = 0; col < mat.getNumColumns(); ++col) {
        #if MASKED_MULTIPLY_ADD
        outerProduct(row, col) = mat(row, pivotCol) * mat(pivotRow, col);
        #else
        // operation to prevent O3 from removing the outer product calculation
        mat(row, col) = mat(row, pivotCol) * mat(pivotRow, col);
        #endif
      }
    }
    #endif

    // // dump outer product
    // std::cout << "Outer product:\n";
    // outerProduct.dump();
    // std::cout << "--------------------------------\n";

    #if MASKED_COLUMN_MULTIPLIES
    // masked column multiplies 1
    Int coeff1 = mat(pivotRow, 0);
    for (int row = 0; row < mat.getNumRows(); ++row) {
      if (row != pivotRow) {
        mat(row, 0) *= coeff1;
      }
    }

    // masked column multiplies 2
    Int coeff2 = mat(pivotRow, pivotCol);
    for (int row = 0; row < mat.getNumRows(); ++row) {
      if (row != pivotRow) {
        mat(row, pivotCol) *= coeff2;
      }
    }
    #endif

    // // dump masked column multiplies 1
    // std::cout << "Masked column multiplie:\n";
    // mat.dump();
    // std::cout << "--------------------------------\n";

    #if MASKED_MULTIPLY_ADD
    // masked multiply-add
    Int coeff3 = mat(pivotRow, 0);
    for (int row = 0; row < mat.getNumRows(); ++row) {
      if (row != pivotRow) {
        for (int col = 1; col < pivotCol; ++col) {
          mat(row, col) = mat(row, col) * coeff3 + outerProduct(row, col);
        }

        for (int col = pivotCol + 1; col < mat.getNumColumns(); ++col) {
          mat(row, col) = mat(row, col) * coeff3 + outerProduct(row, col);
        }
      }
    }
    #endif

    // // dump masked multiply-add
    // std::cout << "Masked multiply-add:\n";
    // mat.dump();
    // std::cout << "--------------------------------\n";
  }
}

template <typename Int>
inline void SMEPivotHelper(Int *matrix, int rows, int cols, int pivot_row, int pivot_col) {
  /*
    rows - reserved rows
    cols - reserved columns
  */
  __asm__ __volatile__(
      
      #if SME_HELPERS
      "zero {za}                                                \n" // Zero ZA  

      "mov x0, %[src]                                           \n" // Source matrix pointer

      "mov w1, %w[nrows]                                        \n" // Number of rows
      "mov w2, %w[ncols]                                        \n" // Number of columns

      "mov w3, %w[prow]                                         \n" // Move pivot_row to w3
      "mov w4, %w[pcol]                                         \n" // Move pivot_col to w4

      "mov x5, #4                                               \n" // Move 4 to x5 for unroll factor

      "mov w6, %w[coeff]                                        \n" // Coefficient

      // ZA tiles can only be indexed through w12-w15
      // "mov x12, #0                                              \n" // Zeroth column index
      "mov w13, %w[prow]                                        \n" // Move pivot_row to w13
      "mov w14, %w[pcol]                                        \n" // Move pivot_col to w14
      // w15 will be used o iterate through the tile

      // predicates
      "ptrue p0.s                                               \n" // Predicate p0.s is set to true
      "whilelt p1.s, xzr, x2                                    \n" // Predicate p1.s is set to true for ncols elements to prevent seg faults in load/store
      "ptrue pn8.s                                              \n" // Predicate pn8.s is set to true to load/store 4 rows at a time
      #endif

      #if SME_MATRIX_LOAD
      // /* ******************** */
      // Load the matrix into ZA0

      // Initialize registers
      "mov w15, #0                                              \n" // Loop counter i = 
      "mov x8, #0                                               \n" // Offset in source matrix

      // Loop label
      "1:                                                       \n"
      "cmp w15, w1                                              \n" // Compare i with nrows
      "b.ge 2f                                                  \n" // If i >= nrows, exit loop

      // Load i-i+3 th rows of source matrix into z0-z3
      "ld1w {z0.s-z3.s}, pn8/z, [x0, x8, lsl #2]                \n"

      // Move the loaded values to ZA0
      "mov za0h.s[w15, 0:3], {z0.s-z3.s}                        \n"

      // Increment loop counter
      "add w15, w15, #4                                         \n" // i = i + 4
      "madd x8, x2, x5, x8                                      \n" // offset += ncols * unroll factor (x8 = x8 + x2 * x5)

      // Loop back
      "b 1b                                                     \n"

      // Loop exit label
      "2:                                                       \n"
      #endif

      /* ******************** */
      #if SME_SAVE_ROWS_COLS
      // Save the pivot row and column
      "mov z30.s, p0/m, za0h.s[w13, 0]                          \n" // Move pivot row from ZA0 to z30
      "mov z31.s, p0/m, za0v.s[w14, 0]                          \n" // Move pivot column from ZA0 to z31
      // these are needed for outer product
  
      /* SME Registers:
      za0 - original matrix
      z30 - pivot row
      z31 - pivot column
      */
      #endif

      /* ******************** */
      #if SME_MULTIPLY_ROWS
      // Multiply rows with coefficients
      
      // Broadcast coeff
      "dup z20.s, w6                                            \n" // z20.s = [coeff, coeff, coeff, ...]

      // Initialize registers
      "mov w15, #0                                              \n" // Loop counter i = 0

      // Loop label (0 -> nrows)
      "1:                                                       \n"
      "cmp w15, w1                                              \n" // Compare i with nrows
      "b.ge 2f                                                  \n" // If i >= nrows, exit loop

      // move matrix rows to z0, z1, z2, z3
      "mov {z0.s, z1.s, z2.s, z3.s}, za0h.s[w15, 0:3]           \n"

      // Multiply matrix rows with coeff
      "mul z0.s, p0/m, z0.s, z20.s                             \n"
      "mul z1.s, p0/m, z1.s, z20.s                             \n"
      "mul z2.s, p0/m, z2.s, z20.s                             \n"
      "mul z3.s, p0/m, z3.s, z20.s                             \n"

      // NOTE: Has somnething like this in 2024-06 version; But M4 was realeased in 2024-05.
      // "fmul {z0.s-z3.s}, {z4.s-z8.s}, {z4.s-z8.s} \n"

      // Move the multiplied values to ZA1
      "mov za1h.s[w15, 0:3], {z0.s, z1.s, z2.s, z3.s}           \n"

      // Increment loop counter
      "add w15, w15, #4                                         \n" // i++

      // Loop back
      "b 1b                                                     \n"

      // Loop exit label
      "2:                                                       \n"

      #endif

      /* ******************** */
      #if SME_SAVE_ROWS_COLS
      "mov w12, #0                                              \n" // Move 0 to w12 for zeroth column index
      // save the zeroth column
      "mov z29.s, p0/m, za1v.s[W12, 0]                          \n" // Move zeroth column from ZA1 to z29

      // zero out pivot column
      // no need to zero z28 because smstart already zeros out all SME/SVE registers!?!
      "mov za1v.s[w14, 0], p0/m, z28.s                          \n" // Move z28 to ZA1 pivot column

      #endif

      /* ******************** */
      #if SME_OUTER_PRODUCT
      // Cast z31 and z30 to 16-bit integers
      "sqxtnb z21.h, z31.s                                      \n" // [a, 0, b, 0, c, 0, d, 0, ...]
      "sqxtnb z20.h, z30.s                                      \n"

      "ptrue p2.h                                               \n"

      "smopa	za1.s, p2/m, p2/m, z21.h, z20.h                   \n" // old pivot column x pivot row

      #endif

      /* ******************** */
      #if SME_SAVE_ROWS_COLS
      // Replaced masked rows/ columns with the saved values
      "mov za1v.s[w12, 0], p0/m, z29.s                          \n" // Move zeroth column from z29 to ZA1
      "mov za1h.s[w13, 0], p0/m, z30.s                          \n" // Move old pivot row from z30 to ZA1

      #endif

      /* ******************** */
      #if SME_MATRIX_STORE
      // Store the result back into the matrix

      // Initialize registers
      "mov w15, #0                                              \n" // Loop counter i = 0
      "mov x8, #0                                               \n" // Offset in source matrix

      // Loop label
      "1:                                                       \n"
      "cmp w15, w1                                              \n" // Compare i with nrows
      "b.ge 2f                                                  \n" // If i >= nrows, exit loop

      // move i-i+3 th rows of source matrix into z0-z3
      "mov {z0.s-z3.s}, za1h.s[w15, 0:3]                        \n"

      // Store the loaded values to the matrix
      "st1w {z0.s-z3.s}, pn8, [x0, x8, lsl #2]                  \n"

      // Increment loop counter
      "add w15, w15, #4                                         \n" // i = i + 4
      "madd x8, x2, x5, x8                                      \n" // offset += ncols * unroll factor (x8 = x8 + x2 * x5)

      // Loop back
      "b 1b                                                     \n"

      // Loop exit label
      "2:                                                       \n"
      #endif
      ""
  : 
  : [src] "r"(matrix),
    [nrows] "r"(rows),
    [ncols] "r"(cols),
    [prow] "r"(pivot_row), 
    [pcol] "r"(pivot_col),
    [coeff] "r"(matrix[pivot_row * cols])
  : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x8",
    "x12", "x13", "x14", "x15",
    "z0", "z4", "z8", "z12", 
    "z29", "z30", "z31",
    "z1", "z2", "z3",
    "z20",
    "za",
    "p0", "p1", "pn8",
    "memory"
  );

}

template <typename Int>
__attribute__((noinline))
void pivotSME(Matrix<Int> &mat, int repeat, int pivotRow, int pivotCol, int nPivots) {
  Int* dataptr = mat.getDataPointer();

  #if SME_START_STOP
  __asm__ __volatile__(
    // IMP: smstart za only enables SME and not SVE. So, use smart to enable both.
    "smstart                                                  \n" // Start SME
    :::
  );
  #endif

  // #pragma unroll
  for (int i = 0; i < repeat; i++) {
    #if SWAP
    // swap
    std::swap(mat(pivotRow, 0), mat(pivotRow, pivotCol));

    if (mat(pivotRow, 0) < 0) {
      mat(pivotRow, 0) = -mat(pivotRow, 0);
      mat(pivotRow, pivotCol) = -mat(pivotRow, pivotCol);
    } else {
      for (int col = 1; col < mat.getNumColumns(); ++col) {
        if (col != pivotCol)
          mat(pivotRow, col) = -mat(pivotRow, col);
      }
    }

    #endif

    SMEPivotHelper(dataptr, mat.getNumRows(), mat.getNReservedColumns(), pivotRow, pivotCol);
  }

  #if SME_START_STOP
  __asm__ __volatile__(

    "smstop                                                  \n" // Stop SME
    
    :
    :
    :
  );
  #endif
}


void process(int numRows, int numCols, int pivotRow, int pivotCol, int nPivots) {
  using Int = int32_t;

  Matrix<Int> mat(numRows, numCols);

  size_t repeat = nPivots;

  mat.randomInit(1, 1);
    // mat.custominit(input);
  pivotNew(mat, repeat, pivotRow, pivotCol, nPivots);
  mat.dumpMatrixToFile("matrix-dump-new.txt");

  mat.randomInit(1, 1);
  // mat.custominit(input);
  pivotBaseline(mat, repeat, pivotRow, pivotCol, nPivots);
  mat.dumpMatrixToFile("matrix-dump-baseline.txt");

  mat.randomInit(1, 1);
  // mat.custominit(input);
  pivotSME(mat, repeat, pivotRow, pivotCol, nPivots);
  mat.dumpMatrixToFile("matrix-dump-sme.txt");
}

int main(int argc, char *argv[]) {
  if (argc == 6) {
    int numRows = std::stoi(argv[1]);
    int numCols = std::stoi(argv[2]);
    int pivotRow = std::stoi(argv[3]);
    int pivotCol = std::stoi(argv[4]);
    int nPivots = std::stoi(argv[5]);

    process(numRows, numCols, pivotRow, pivotCol, nPivots);
    return EXIT_SUCCESS;

  } else {
    std::cerr << "Usage: " << argv[0] << " <numRows> <numCols> <pivotRow> <pivotCol> <nPivots>" << std::endl;
    return EXIT_FAILURE;
  }
  
}