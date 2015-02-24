/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:

 WTF !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  6. Use any form of casting. 
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;  
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* 
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int evenBits(void) {
  int patch_01010101 = 0x55;
  int result = patch_01010101;             // result: 1010 1010
  result = (result << 8) + patch_01010101; // result: 1010 1010 1010 1010
  result = (result << 8) + patch_01010101; // result: 1010 1010 1010 1010 1010 1010
  result = (result << 8) + patch_01010101; // result: 1010 1010 1010 1010 1010 1010 1010 1010
  return result;
}
/* 
 * isEqual - return 1 if x == y, and 0 otherwise 
 *   Examples: isEqual(5,5) = 1, isEqual(4,5) = 0
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int isEqual(int x, int y) {
  int result = x ^ y;
  return !result;
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x 12 34 56 78        , 1, 3) = 0x56 34 12 78
 *            byteSwap(0x DE AD BE EF        , 0, 2) = 0xDE EF BE AD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
  int byte_mask = 0xFF;
  int byte1 = 0;
  int byte2 = 0;
  int n_bits = n << 3;
  int m_bits = m << 3; 
  int result = x;

  byte1 = x & (byte_mask << n_bits);
  result = result + (~byte1 + 1);
  byte1 = ((byte1 >> n_bits) & byte_mask) << m_bits;

  byte2 = x & (byte_mask << m_bits);
  result = result + (~byte2 + 1);
  byte2 = ((byte2 >> m_bits) & byte_mask) << n_bits;

  result = result + byte1 + byte2;

  return result;
}
/* 
 * rotateRight - Rotate x to the right by n
 *   Can assume that 0 <= n <= 31
 *   Examples: rotateRight(0x87654321,4) = 0x18765432
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 3 
 */
int rotateRight(int x, int n) {
  int sub = 0;
  int result = 0;
  int minus_n = ~n + 1;
  int low_mask = 0;
  int high_mask = 0;

  low_mask = high_mask = ~0;
  low_mask = low_mask << n; //low_mask = (32-n)1 (n)0
  low_mask = ~low_mask;     //low_mask = (32-n)0 (n)1

  high_mask = high_mask << (32+minus_n);  //high_mask = (n)1 (32-n)0
  high_mask = ~high_mask;                 //high_mask = (n)0 (32-n)1
  high_mask = high_mask | ((!high_mask)<<31)>>31;

  sub = x & low_mask;
  x = x >> n;
  x = x & high_mask;
  sub = sub << (32 + minus_n);
  result = sub + x;

  return result;
}
/* 
 * logicalNeg - implement the ! operator using any of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {   
  int y = x;        
  int result = 0;

  result = y | (y >> 16);
  y = result;
  result = y | (y >> 8);
  y = result;
  result = y | (y >> 4);
  y = result;
  result = y | (y >> 2);
  y = result;
  result = y | (y >> 1);
  result = ~result;
  result = result & 0x01;
  return result;
}
/* 
 * TMax - return maximum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmax(void) {
  int result = 0x80;
  result = result << 24;
  return ~result;
}
/* 
 * sign - return 1 if positive, 0 if zero, and -1 if negative
 *  Examples: sign(130) = 1
 *            sign(-23) = -1
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 10
 *  Rating: 2
 */
int sign(int x) {
  int result = 0;
  result = (x >> 31) | !(!x);
  return result;
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {   
  int is_xy_equal = 0;
  int is_xy_same_sign = 0;
  int sign_bit_x = 0;
  int sign_bit_y = 0;
  int sign_bit_mask = 0x80 << 24;
  int minus_y = 0;
  int result = 0;

  minus_y = ~y + 1;
  result = x + minus_y;

  is_xy_equal = !!(x ^ y); //false=00..001  true=00..000

  sign_bit_x = x & sign_bit_mask;
  sign_bit_y = y & sign_bit_mask;
  is_xy_same_sign = (~(sign_bit_x ^ sign_bit_y))>>31; //true=11..111  false=00..0000

  result = (is_xy_same_sign & ((~(result>>31))&1)) | (~is_xy_same_sign & ((sign_bit_y>>31)&1));
  result = is_xy_equal & result;

  return result;
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) { 
  
  int result = 0;
  int sign_bit_x = 0;
  int sign_bit_y = 0;
  int sign_bit_result = 0;
  int sign_bit_mask = (0x80 << 24);
  int is_xy_same_sign = 0;
  int is_x_result_same_sign = 0;
  int minus_y = 0;

  minus_y = ~y + 1;
  result = x + minus_y; //result = x-y

  sign_bit_x = x & sign_bit_mask;
  sign_bit_y = y & sign_bit_mask;
  sign_bit_result = result & sign_bit_mask;  

  is_xy_same_sign = (~(sign_bit_x ^ sign_bit_y))>>31; //true=11..111 false=00...0
  is_x_result_same_sign = !(sign_bit_x ^ sign_bit_result); //true=00..001 false=00...0

  result = (is_xy_same_sign & 1) | (~is_xy_same_sign & is_x_result_same_sign);
  return result;
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          maximum possible value, and when negative overflow occurs,
 *          it returns minimum possible value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff 
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y) { 
  int sum = 0;
  int sign_bit_x = 0;
  int sign_bit_y = 0;
  int sign_bit_sum = 0;
  int sign_bit_mask = (0x80 << 24);
  int is_xy_same_sign = 0;
  int is_x_sum_same_sign = 0;
  int sum_if_overflow = 0;
  int result = 0;

  sum = x+y;
  sum_if_overflow = (sum >> 31) + sign_bit_mask;
  sign_bit_x = x & sign_bit_mask; 
  sign_bit_y = y & sign_bit_mask; 
  sign_bit_sum = sum & sign_bit_mask;

  is_xy_same_sign = (~(sign_bit_x ^ sign_bit_y))>>31; //true=11..111 false=00...0
  is_x_sum_same_sign = (~(sign_bit_x ^ sign_bit_sum))>>31; //true=11..111 false=00...0

  result = (is_x_sum_same_sign & sum) | (~is_x_sum_same_sign & sum_if_overflow);
  sum = (is_xy_same_sign & result) | (~is_xy_same_sign & sum);

  return sum;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  int is_x_minus = 0;
  int sign_bit_mask = 0x80 << 24;
  int sign_bit_x = 0;
  int n_bits_required_for_x = 0;
  int inverse_x = 0;
  int x_to_be_computed = 0;
  int n_bits_ahead = 0;
  int byte_after_shift = 0;
  int n_bits_shifted = 0;
  int is_over_24bits = 0;
  int is_over_16bits = 0;
  int is_over_8bits  = 0;
  int has_bit = 0;

  inverse_x = ~x;
  sign_bit_x = x & sign_bit_mask;
  is_x_minus = sign_bit_x >> 31; // true: 0xFFFFFFFF  false: 0x00000000

  x_to_be_computed = (is_x_minus & inverse_x) | (~is_x_minus & x);

  is_over_24bits = ((!!(x_to_be_computed >> 24)) <<31) >>31;
  is_over_16bits = ((!!(x_to_be_computed >> 16)) <<31) >>31;
  is_over_8bits =  ((!!(x_to_be_computed >>  8)) <<31) >>31;

  n_bits_ahead = (is_over_8bits  &  8) | (~is_over_8bits  & n_bits_ahead);
  n_bits_ahead = (is_over_16bits & 16) | (~is_over_16bits & n_bits_ahead);
  n_bits_ahead = (is_over_24bits & 24) | (~is_over_24bits & n_bits_ahead);

  byte_after_shift = x_to_be_computed >> n_bits_ahead;

  has_bit  = ((byte_after_shift << 31) >> 31); //true=11..1111 false=00...00
  n_bits_shifted = (has_bit & 1) | (~has_bit & n_bits_shifted);
  has_bit  = (((byte_after_shift >> 1) << 31) >> 31);
  n_bits_shifted = (has_bit & 2) | (~has_bit & n_bits_shifted);
  has_bit  = (((byte_after_shift >> 2) << 31) >> 31);
  n_bits_shifted = (has_bit & 3) | (~has_bit & n_bits_shifted);
  has_bit  = (((byte_after_shift >> 3) << 31) >> 31);
  n_bits_shifted = (has_bit & 4) | (~has_bit & n_bits_shifted);
  has_bit  = (((byte_after_shift >> 4) << 31) >> 31);
  n_bits_shifted = (has_bit & 5) | (~has_bit & n_bits_shifted);
  has_bit  = (((byte_after_shift >> 5) << 31) >> 31);
  n_bits_shifted = (has_bit & 6) | (~has_bit & n_bits_shifted);
  has_bit  = (((byte_after_shift >> 6) << 31) >> 31);
  n_bits_shifted = (has_bit & 7) | (~has_bit & n_bits_shifted);
  has_bit  = (((byte_after_shift >> 7) << 31) >> 31);
  n_bits_shifted = (has_bit & 8) | (~has_bit & n_bits_shifted);

  n_bits_required_for_x = n_bits_ahead + n_bits_shifted + 1;

  return n_bits_required_for_x;
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
  int exp_mask = 0x7F800000; // 0111 1111 1000 0000 00000000 00000000
  int frac_mask = 0x7FFFFF;  // 0000 0000 0111 1111 11111111 11111111
  unsigned exp_bits = 0;
  unsigned frac_bits = 0;
  unsigned frac_bit_1 = 0;
  unsigned result = 0;

  exp_bits = uf & exp_mask;
  frac_bits = uf & frac_mask;
  
  if(exp_bits == 0){ // denormalized  frac>>1 + rounding
    frac_bit_1 = frac_bits & 1;
    uf -= frac_bits;
    frac_bits = frac_bits >> 1;

    if(frac_bit_1 == 1) // rounding
      if(frac_bits & 1) // need +1
        frac_bits += 1;
    
    result = uf + frac_bits;

  }else if(exp_bits == exp_mask){ // special
    result = uf;

  }else{ // normalized  exp-1
    uf -= exp_bits;
    exp_bits = (exp_bits >> 23) & 0xFF;
    exp_bits -= 1;

    if(exp_bits == 0){ // 
      uf -= frac_bits;
      frac_bits = frac_bits + (1<<23);
      frac_bit_1 = frac_bits & 1;
      frac_bits = frac_bits >> 1;

      if(frac_bit_1 == 1) // rounding
        if(frac_bits & 1) // need +1
          frac_bits += 1;
    
      result = uf + frac_bits;
      return result;
    }

    exp_bits = exp_bits << 23;
    result = uf + exp_bits;
  }

  return result;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
  unsigned exp_mask = 0x7F800000; // 0111 1111 1000 0000 00000000 00000000
  unsigned frac_mask = 0x7FFFFF;  // 0000 0000 0111 1111 11111111 11111111
  unsigned exp_value = 0; 
  unsigned sign_bit_mask = 0x80000000;
  unsigned sign_bit = 0;
  unsigned exp_bits = 0;
  unsigned frac_bits = 0;
  int result = 0;
  sign_bit = uf & sign_bit_mask; // 1: 100..00  0: 00...00
  exp_bits = uf & exp_mask;
  frac_bits = uf & frac_mask;

  exp_bits =  exp_bits >> 23;
  exp_value = exp_bits - 127;

  if(exp_bits == 0xFF){
    return 0x80000000u;
  }else if(exp_value > 30){
    return 0x80000000u;
  }else if(exp_value <  0){
    return 0;
  }else{
    frac_bits = frac_bits >> (23 - exp_value);
    frac_bits = frac_bits +  (1 << exp_value);
    if(sign_bit)
      frac_bits = ~frac_bits + 1;
    result = (int)frac_bits;
    return result;
  }
}
