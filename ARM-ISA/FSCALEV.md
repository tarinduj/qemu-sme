### FSCALE – Floating-point **Tile Scale by Vector** (single-precision)

|                         |                                                                                                                                                                                                                                                                                                                                         |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Mnemonic**            | `FSCALE`                                                                                                                                                                                                                                                                                                                                |
| **Syntax**              | `FSCALE <ZAd><HV>.S, <Pg>/M, <Zn>.S`                                                                                                  |
| **Element size**        | 32-bit (FP32)                                                                                                                                                                                                                                                                                                                           |
| **Description**         | Multiplies every element of a ZA tile by the corresponding element of a source vector and writes the result back to the same tile. The `<HV>` bit chooses whether the vector is broadcast **horizontally** across each row (`H` = 0) or **vertically** down each column (`V` = 1). Execution is predicated by `<Pg>` in **MERGE** form. |

---

#### Encoding (31-0)

| 31-25 | 24 | 23 | 22-16 | 15 | 14 | 13 | 12-10 | 9-5 | 4 | 3-2 | 1-0 | 
| - | - | - | - | - | -- | - | - | - | - | - | - |
| `1000000` | –  | `0`  | – | **V** | `1` | – |**Pg** | **Zn** | – | **ZAd** | – |

* **V** (1 bit) – horizontal/vertical selector (`0` = rows, `1` = columns)
* **Pg** (3 bits) – predicate register P0-P7 (streaming mode)
* **Zn** (5 bits) – vector register Z0-Z31 supplying the scaling factors
* **ZAd** (2 bits) – destination ZA tile ZA0-ZA3

---

#### Operation 

```text
CheckStreamingSVEAndZAEnabled();
constant integer VL  = CurrentVL;
constant integer PL  = VL DIV 8;
constant integer dim = VL DIV esize;

constant bits(PL) mask = P[g, PL];
constant bits(VL) vec  = Z[n, VL];
constant bits(dim*dim*esize) tile = ZAtile[da, esize, dim*dim*esize];
bits(dim*dim*esize) result;

for row = 0 to dim-1
    for col = 0 to dim-1
        if ( (HV == 0 AND ActivePredicateElement(mask, col, esize)) OR
             (HV == 1 AND ActivePredicateElement(mask, row, esize)) ) then
            constant bits(esize) factor = Elem[vec, (HV == 0) ? row : col, esize];
            constant bits(esize) elem   = Elem[tile, row*dim+col, esize];
            Elem[result, row*dim+col, esize] = FPMul_ZA(elem, factor, FPCR);
        else
            Elem[result, row*dim+col, esize] = Elem[tile, row*dim+col, esize];

ZAtile[da, esize, dim*dim*esize] = result;
```



---

#### Example (row scaling)

```asm
// Multiply each row of ZA0 by the corresponding element of Z2
// Only columns with P0 bits set are updated.
FSCALE   ZA0H.S, P0/M, Z2.S
```

#### Example (column scaling)

```asm
// Multiply each column of ZA1 by elements of Z1, updating all rows
FSCALE   ZA1V.S, P0/M, Z1.S
```

