/* integer.c
 *
 * Copyright (C) 2006-2009 Sawtooth Consulting Ltd.
 *
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/*
 * Based on public domain LibTomMath 0.38 by Tom St Denis, tomstdenis@iahu.ca,
 * http://math.libtomcrypt.com
 */


/* optionally use fast math instead */
#ifdef USE_FAST_MATH
    #include "tfm.c"
#else

#include "integer.h"
#include <stdlib.h>


/* handle up to 6 inits */
int mp_init_multi(mp_int* a, mp_int* b, mp_int* c, mp_int* d, mp_int* e,
                  mp_int* f)
{
    int res = MP_OKAY;

    if (a && ((res = mp_init(a)) != MP_OKAY))
        return res;

    if (b && ((res = mp_init(b)) != MP_OKAY)) {
        mp_clear(a);
        return res;
    }

    if (c && ((res = mp_init(c)) != MP_OKAY)) {
        mp_clear(a); mp_clear(b);
        return res;
    }

    if (d && ((res = mp_init(d)) != MP_OKAY)) {
        mp_clear(a); mp_clear(b); mp_clear(c);
        return res;
    }

    if (e && ((res = mp_init(e)) != MP_OKAY)) {
        mp_clear(a); mp_clear(b); mp_clear(c); mp_clear(d);
        return res;
    }

    if (f && ((res = mp_init(f)) != MP_OKAY)) {
        mp_clear(a); mp_clear(b); mp_clear(c); mp_clear(d); mp_clear(e);
        return res;
    }

    return res;
}


/* init a new mp_int */
int mp_init (mp_int * a)
{
  int i;

  /* allocate memory required and clear it */
  a->dp = OPT_CAST(mp_digit) XMALLOC (sizeof (mp_digit) * MP_PREC, 0);
  if (a->dp == NULL) {
    return MP_MEM;
  }

  /* set the digits to zero */
  for (i = 0; i < MP_PREC; i++) {
      a->dp[i] = 0;
  }

  /* set the used to zero, allocated digits to the default precision
   * and sign to positive */
  a->used  = 0;
  a->alloc = MP_PREC;
  a->sign  = MP_ZPOS;

  return MP_OKAY;
}


/* clear one (frees)  */
void
mp_clear (mp_int * a)
{
  int i;

  /* only do anything if a hasn't been freed previously */
  if (a->dp != NULL) {
    /* first zero the digits */
    for (i = 0; i < a->used; i++) {
        a->dp[i] = 0;
    }

    /* free ram */
    XFREE(a->dp, 0);

    /* reset members to make debugging easier */
    a->dp    = NULL;
    a->alloc = a->used = 0;
    a->sign  = MP_ZPOS;
  }
}


/* get the size for an unsigned equivalent */
int mp_unsigned_bin_size (mp_int * a)
{
  int     size = mp_count_bits (a);
  return (size / 8 + ((size & 7) != 0 ? 1 : 0));
}


/* returns the number of bits in an int */
int
mp_count_bits (mp_int * a)
{
  int     r;
  mp_digit q;

  /* shortcut */
  if (a->used == 0) {
    return 0;
  }

  /* get number of digits and add that */
  r = (a->used - 1) * DIGIT_BIT;
  
  /* take the last digit and count the bits in it */
  q = a->dp[a->used - 1];
  while (q > ((mp_digit) 0)) {
    ++r;
    q >>= ((mp_digit) 1);
  }
  return r;
}


/* store in unsigned [big endian] format */
int mp_to_unsigned_bin (mp_int * a, unsigned char *b)
{
  int     x, res;
  mp_int  t;

  if ((res = mp_init_copy (&t, a)) != MP_OKAY) {
    return res;
  }

  x = 0;
  while (mp_iszero (&t) == 0) {
#ifndef MP_8BIT
      b[x++] = (unsigned char) (t.dp[0] & 255);
#else
      b[x++] = (unsigned char) (t.dp[0] | ((t.dp[1] & 0x01) << 7));
#endif
    if ((res = mp_div_2d (&t, 8, &t, NULL)) != MP_OKAY) {
      mp_clear (&t);
      return res;
    }
  }
  bn_reverse (b, x);
  mp_clear (&t);
  return MP_OKAY;
}


/* creates "a" then copies b into it */
int mp_init_copy (mp_int * a, mp_int * b)
{
  int     res;

  if ((res = mp_init (a)) != MP_OKAY) {
    return res;
  }
  return mp_copy (b, a);
}


/* copy, b = a */
int
mp_copy (mp_int * a, mp_int * b)
{
  int     res, n;

  /* if dst == src do nothing */
  if (a == b) {
    return MP_OKAY;
  }

  /* grow dest */
  if (b->alloc < a->used) {
     if ((res = mp_grow (b, a->used)) != MP_OKAY) {
        return res;
     }
  }

  /* zero b and copy the parameters over */
  {
    register mp_digit *tmpa, *tmpb;

    /* pointer aliases */

    /* source */
    tmpa = a->dp;

    /* destination */
    tmpb = b->dp;

    /* copy all the digits */
    for (n = 0; n < a->used; n++) {
      *tmpb++ = *tmpa++;
    }

    /* clear high digits */
    for (; n < b->used; n++) {
      *tmpb++ = 0;
    }
  }

  /* copy used count and sign */
  b->used = a->used;
  b->sign = a->sign;
  return MP_OKAY;
}


/* grow as required */
int mp_grow (mp_int * a, int size)
{
  int     i;
  mp_digit *tmp;

  /* if the alloc size is smaller alloc more ram */
  if (a->alloc < size) {
    /* ensure there are always at least MP_PREC digits extra on top */
    size += (MP_PREC * 2) - (size % MP_PREC);

    /* reallocate the array a->dp
     *
     * We store the return in a temporary variable
     * in case the operation failed we don't want
     * to overwrite the dp member of a.
     */
    tmp = OPT_CAST(mp_digit) XREALLOC (a->dp, sizeof (mp_digit) * size, 0);
    if (tmp == NULL) {
      /* reallocation failed but "a" is still valid [can be freed] */
      return MP_MEM;
    }

    /* reallocation succeeded so set a->dp */
    a->dp = tmp;

    /* zero excess digits */
    i        = a->alloc;
    a->alloc = size;
    for (; i < a->alloc; i++) {
      a->dp[i] = 0;
    }
  }
  return MP_OKAY;
}


/* reverse an array, used for radix code */
void
bn_reverse (unsigned char *s, int len)
{
  int     ix, iy;
  unsigned char t;

  ix = 0;
  iy = len - 1;
  while (ix < iy) {
    t     = s[ix];
    s[ix] = s[iy];
    s[iy] = t;
    ++ix;
    --iy;
  }
}


/* shift right by a certain bit count (store quotient in c, optional
   remainder in d) */
int mp_div_2d (mp_int * a, int b, mp_int * c, mp_int * d)
{
  mp_digit D, r, rr;
  int     x, res;
  mp_int  t;


  /* if the shift count is <= 0 then we do no work */
  if (b <= 0) {
    res = mp_copy (a, c);
    if (d != NULL) {
      mp_zero (d);
    }
    return res;
  }

  if ((res = mp_init (&t)) != MP_OKAY) {
    return res;
  }

  /* get the remainder */
  if (d != NULL) {
    if ((res = mp_mod_2d (a, b, &t)) != MP_OKAY) {
      mp_clear (&t);
      return res;
    }
  }

  /* copy */
  if ((res = mp_copy (a, c)) != MP_OKAY) {
    mp_clear (&t);
    return res;
  }

  /* shift by as many digits in the bit count */
  if (b >= (int)DIGIT_BIT) {
    mp_rshd (c, b / DIGIT_BIT);
  }

  /* shift any bit count < DIGIT_BIT */
  D = (mp_digit) (b % DIGIT_BIT);
  if (D != 0) {
    register mp_digit *tmpc, mask, shift;

    /* mask */
    mask = (((mp_digit)1) << D) - 1;

    /* shift for lsb */
    shift = DIGIT_BIT - D;

    /* alias */
    tmpc = c->dp + (c->used - 1);

    /* carry */
    r = 0;
    for (x = c->used - 1; x >= 0; x--) {
      /* get the lower  bits of this word in a temp */
      rr = *tmpc & mask;

      /* shift the current word and mix in the carry bits from the previous
         word */
      *tmpc = (*tmpc >> D) | (r << shift);
      --tmpc;

      /* set the carry to the carry bits of the current word found above */
      r = rr;
    }
  }
  mp_clamp (c);
  if (d != NULL) {
    mp_exch (&t, d);
  }
  mp_clear (&t);
  return MP_OKAY;
}


/* set to zero */
void mp_zero (mp_int * a)
{
  int       n;
  mp_digit *tmp;

  a->sign = MP_ZPOS;
  a->used = 0;

  tmp = a->dp;
  for (n = 0; n < a->alloc; n++) {
     *tmp++ = 0;
  }
}


/* trim unused digits 
 *
 * This is used to ensure that leading zero digits are
 * trimed and the leading "used" digit will be non-zero
 * Typically very fast.  Also fixes the sign if there
 * are no more leading digits
 */
void
mp_clamp (mp_int * a)
{
  /* decrease used while the most significant digit is
   * zero.
   */
  while (a->used > 0 && a->dp[a->used - 1] == 0) {
    --(a->used);
  }

  /* reset the sign flag if used == 0 */
  if (a->used == 0) {
    a->sign = MP_ZPOS;
  }
}


/* swap the elements of two integers, for cases where you can't simply swap the 
 * mp_int pointers around
 */
void
mp_exch (mp_int * a, mp_int * b)
{
  mp_int  t;

  t  = *a;
  *a = *b;
  *b = t;
}


/* shift right a certain amount of digits */
void mp_rshd (mp_int * a, int b)
{
  int     x;

  /* if b <= 0 then ignore it */
  if (b <= 0) {
    return;
  }

  /* if b > used then simply zero it and return */
  if (a->used <= b) {
    mp_zero (a);
    return;
  }

  {
    register mp_digit *bottom, *top;

    /* shift the digits down */

    /* bottom */
    bottom = a->dp;

    /* top [offset into digits] */
    top = a->dp + b;

    /* this is implemented as a sliding window where 
     * the window is b-digits long and digits from 
     * the top of the window are copied to the bottom
     *
     * e.g.

     b-2 | b-1 | b0 | b1 | b2 | ... | bb |   ---->
                 /\                   |      ---->
                  \-------------------/      ---->
     */
    for (x = 0; x < (a->used - b); x++) {
      *bottom++ = *top++;
    }

    /* zero the top digits */
    for (; x < a->used; x++) {
      *bottom++ = 0;
    }
  }
  
  /* remove excess digits */
  a->used -= b;
}


/* calc a value mod 2**b */
int
mp_mod_2d (mp_int * a, int b, mp_int * c)
{
  int     x, res;

  /* if b is <= 0 then zero the int */
  if (b <= 0) {
    mp_zero (c);
    return MP_OKAY;
  }

  /* if the modulus is larger than the value than return */
  if (b >= (int) (a->used * DIGIT_BIT)) {
    res = mp_copy (a, c);
    return res;
  }

  /* copy */
  if ((res = mp_copy (a, c)) != MP_OKAY) {
    return res;
  }

  /* zero digits above the last digit of the modulus */
  for (x = (b / DIGIT_BIT) + ((b % DIGIT_BIT) == 0 ? 0 : 1); x < c->used; x++) {
    c->dp[x] = 0;
  }
  /* clear the digit that is not completely outside/inside the modulus */
  c->dp[b / DIGIT_BIT] &= (mp_digit) ((((mp_digit) 1) <<
              (((mp_digit) b) % DIGIT_BIT)) - ((mp_digit) 1));
  mp_clamp (c);
  return MP_OKAY;
}


/* reads a unsigned char array, assumes the msb is stored first [big endian] */
int mp_read_unsigned_bin (mp_int * a, const unsigned char *b, int c)
{
  int     res;

  /* make sure there are at least two digits */
  if (a->alloc < 2) {
     if ((res = mp_grow(a, 2)) != MP_OKAY) {
        return res;
     }
  }

  /* zero the int */
  mp_zero (a);

  /* read the bytes in */
  while (c-- > 0) {
    if ((res = mp_mul_2d (a, 8, a)) != MP_OKAY) {
      return res;
    }

#ifndef MP_8BIT
      a->dp[0] |= *b++;
      a->used += 1;
#else
      a->dp[0] = (*b & MP_MASK);
      a->dp[1] |= ((*b++ >> 7U) & 1);
      a->used += 2;
#endif
  }
  mp_clamp (a);
  return MP_OKAY;
}


/* shift left by a certain bit count */
int mp_mul_2d (mp_int * a, int b, mp_int * c)
{
  mp_digit d;
  int      res;

  /* copy */
  if (a != c) {
     if ((res = mp_copy (a, c)) != MP_OKAY) {
       return res;
     }
  }

  if (c->alloc < (int)(c->used + b/DIGIT_BIT + 1)) {
     if ((res = mp_grow (c, c->used + b / DIGIT_BIT + 1)) != MP_OKAY) {
       return res;
     }
  }

  /* shift by as many digits in the bit count */
  if (b >= (int)DIGIT_BIT) {
    if ((res = mp_lshd (c, b / DIGIT_BIT)) != MP_OKAY) {
      return res;
    }
  }

  /* shift any bit count < DIGIT_BIT */
  d = (mp_digit) (b % DIGIT_BIT);
  if (d != 0) {
    register mp_digit *tmpc, shift, mask, r, rr;
    register int x;

    /* bitmask for carries */
    mask = (((mp_digit)1) << d) - 1;

    /* shift for msbs */
    shift = DIGIT_BIT - d;

    /* alias */
    tmpc = c->dp;

    /* carry */
    r    = 0;
    for (x = 0; x < c->used; x++) {
      /* get the higher bits of the current word */
      rr = (*tmpc >> shift) & mask;

      /* shift the current word and OR in the carry */
      *tmpc = ((*tmpc << d) | r) & MP_MASK;
      ++tmpc;

      /* set the carry to the carry bits of the current word */
      r = rr;
    }
    
    /* set final carry */
    if (r != 0) {
       c->dp[(c->used)++] = r;
    }
  }
  mp_clamp (c);
  return MP_OKAY;
}


/* shift left a certain amount of digits */
int mp_lshd (mp_int * a, int b)
{
  int     x, res;

  /* if its less than zero return */
  if (b <= 0) {
    return MP_OKAY;
  }

  /* grow to fit the new digits */
  if (a->alloc < a->used + b) {
     if ((res = mp_grow (a, a->used + b)) != MP_OKAY) {
       return res;
     }
  }

  {
    register mp_digit *top, *bottom;

    /* increment the used by the shift amount then copy upwards */
    a->used += b;

    /* top */
    top = a->dp + a->used - 1;

    /* base */
    bottom = a->dp + a->used - 1 - b;

    /* much like mp_rshd this is implemented using a sliding window
     * except the window goes the otherway around.  Copying from
     * the bottom to the top.  see bn_mp_rshd.c for more info.
     */
    for (x = a->used - 1; x >= b; x--) {
      *top-- = *bottom--;
    }

    /* zero the lower digits */
    top = a->dp;
    for (x = 0; x < b; x++) {
      *top++ = 0;
    }
  }
  return MP_OKAY;
}


/* this is a shell function that calls either the normal or Montgomery
 * exptmod functions.  Originally the call to the montgomery code was
 * embedded in the normal function but that wasted alot of stack space
 * for nothing (since 99% of the time the Montgomery code would be called)
 */
int mp_exptmod (mp_int * G, mp_int * X, mp_int * P, mp_int * Y)
{
  int dr;

  /* modulus P must be positive */
  if (P->sign == MP_NEG) {
     return MP_VAL;
  }

  /* if exponent X is negative we have to recurse */
  if (X->sign == MP_NEG) {
#ifdef BN_MP_INVMOD_C
     mp_int tmpG, tmpX;
     int err;

     /* first compute 1/G mod P */
     if ((err = mp_init(&tmpG)) != MP_OKAY) {
        return err;
     }
     if ((err = mp_invmod(G, P, &tmpG)) != MP_OKAY) {
        mp_clear(&tmpG);
        return err;
     }

     /* now get |X| */
     if ((err = mp_init(&tmpX)) != MP_OKAY) {
        mp_clear(&tmpG);
        return err;
     }
     if ((err = mp_abs(X, &tmpX)) != MP_OKAY) {
        mp_clear(&tmpG);
        mp_clear(&tmpX);
        return err;
     }

     /* and now compute (1/G)**|X| instead of G**X [X < 0] */
     err = mp_exptmod(&tmpG, &tmpX, P, Y);
     mp_clear(&tmpG);
     mp_clear(&tmpX);
     return err;
#else 
     /* no invmod */
     return MP_VAL;
#endif
  }

/* modified diminished radix reduction */
#if defined(BN_MP_REDUCE_IS_2K_L_C) && defined(BN_MP_REDUCE_2K_L_C) && \
  defined(BN_S_MP_EXPTMOD_C)
  if (mp_reduce_is_2k_l(P) == MP_YES) {
     return s_mp_exptmod(G, X, P, Y, 1);
  }
#endif

#ifdef BN_MP_DR_IS_MODULUS_C
  /* is it a DR modulus? */
  dr = mp_dr_is_modulus(P);
#else
  /* default to no */
  dr = 0;
#endif

#ifdef BN_MP_REDUCE_IS_2K_C
  /* if not, is it a unrestricted DR modulus? */
  if (dr == 0) {
     dr = mp_reduce_is_2k(P) << 1;
  }
#endif
    
  /* if the modulus is odd or dr != 0 use the montgomery method */
#ifdef BN_MP_EXPTMOD_FAST_C
  if (mp_isodd (P) == 1 || dr !=  0) {
    return mp_exptmod_fast (G, X, P, Y, dr);
  } else {
#endif
#ifdef BN_S_MP_EXPTMOD_C
    /* otherwise use the generic Barrett reduction technique */
    return s_mp_exptmod (G, X, P, Y, 0);
#else
    /* no exptmod for evens */
    return MP_VAL;
#endif
#ifdef BN_MP_EXPTMOD_FAST_C
  }
#endif
}


/* b = |a| 
 *
 * Simple function copies the input and fixes the sign to positive
 */
int
mp_abs (mp_int * a, mp_int * b)
{
  int     res;

  /* copy a to b */
  if (a != b) {
     if ((res = mp_copy (a, b)) != MP_OKAY) {
       return res;
     }
  }

  /* force the sign of b to positive */
  b->sign = MP_ZPOS;

  return MP_OKAY;
}


/* hac 14.61, pp608 */
int mp_invmod (mp_int * a, mp_int * b, mp_int * c)
{
  /* b cannot be negative */
  if (b->sign == MP_NEG || mp_iszero(b) == 1) {
    return MP_VAL;
  }

#ifdef BN_FAST_MP_INVMOD_C
  /* if the modulus is odd we can use a faster routine instead */
  if (mp_isodd (b) == 1) {
    return fast_mp_invmod (a, b, c);
  }
#endif

#ifdef BN_MP_INVMOD_SLOW_C
  return mp_invmod_slow(a, b, c);
#endif

  return MP_VAL;
}


/* computes the modular inverse via binary extended euclidean algorithm, 
 * that is c = 1/a mod b 
 *
 * Based on slow invmod except this is optimized for the case where b is 
 * odd as per HAC Note 14.64 on pp. 610
 */
int fast_mp_invmod (mp_int * a, mp_int * b, mp_int * c)
{
  mp_int  x, y, u, v, B, D;
  int     res, neg;

  /* 2. [modified] b must be odd   */
  if (mp_iseven (b) == 1) {
    return MP_VAL;
  }

  /* init all our temps */
  if ((res = mp_init_multi(&x, &y, &u, &v, &B, &D)) != MP_OKAY) {
     return res;
  }

  /* x == modulus, y == value to invert */
  if ((res = mp_copy (b, &x)) != MP_OKAY) {
    goto LBL_ERR;
  }

  /* we need y = |a| */
  if ((res = mp_mod (a, b, &y)) != MP_OKAY) {
    goto LBL_ERR;
  }

  /* 3. u=x, v=y, A=1, B=0, C=0,D=1 */
  if ((res = mp_copy (&x, &u)) != MP_OKAY) {
    goto LBL_ERR;
  }
  if ((res = mp_copy (&y, &v)) != MP_OKAY) {
    goto LBL_ERR;
  }
  mp_set (&D, 1);

top:
  /* 4.  while u is even do */
  while (mp_iseven (&u) == 1) {
    /* 4.1 u = u/2 */
    if ((res = mp_div_2 (&u, &u)) != MP_OKAY) {
      goto LBL_ERR;
    }
    /* 4.2 if B is odd then */
    if (mp_isodd (&B) == 1) {
      if ((res = mp_sub (&B, &x, &B)) != MP_OKAY) {
        goto LBL_ERR;
      }
    }
    /* B = B/2 */
    if ((res = mp_div_2 (&B, &B)) != MP_OKAY) {
      goto LBL_ERR;
    }
  }

  /* 5.  while v is even do */
  while (mp_iseven (&v) == 1) {
    /* 5.1 v = v/2 */
    if ((res = mp_div_2 (&v, &v)) != MP_OKAY) {
      goto LBL_ERR;
    }
    /* 5.2 if D is odd then */
    if (mp_isodd (&D) == 1) {
      /* D = (D-x)/2 */
      if ((res = mp_sub (&D, &x, &D)) != MP_OKAY) {
        goto LBL_ERR;
      }
    }
    /* D = D/2 */
    if ((res = mp_div_2 (&D, &D)) != MP_OKAY) {
      goto LBL_ERR;
    }
  }

  /* 6.  if u >= v then */
  if (mp_cmp (&u, &v) != MP_LT) {
    /* u = u - v, B = B - D */
    if ((res = mp_sub (&u, &v, &u)) != MP_OKAY) {
      goto LBL_ERR;
    }

    if ((res = mp_sub (&B, &D, &B)) != MP_OKAY) {
      goto LBL_ERR;
    }
  } else {
    /* v - v - u, D = D - B */
    if ((res = mp_sub (&v, &u, &v)) != MP_OKAY) {
      goto LBL_ERR;
    }

    if ((res = mp_sub (&D, &B, &D)) != MP_OKAY) {
      goto LBL_ERR;
    }
  }

  /* if not zero goto step 4 */
  if (mp_iszero (&u) == 0) {
    goto top;
  }

  /* now a = C, b = D, gcd == g*v */

  /* if v != 1 then there is no inverse */
  if (mp_cmp_d (&v, 1) != MP_EQ) {
    res = MP_VAL;
    goto LBL_ERR;
  }

  /* b is now the inverse */
  neg = a->sign;
  while (D.sign == MP_NEG) {
    if ((res = mp_add (&D, b, &D)) != MP_OKAY) {
      goto LBL_ERR;
    }
  }
  mp_exch (&D, c);
  c->sign = neg;
  res = MP_OKAY;

LBL_ERR:mp_clear(&x);
        mp_clear(&y);
        mp_clear(&u);
        mp_clear(&v);
        mp_clear(&B);
        mp_clear(&D);
  return res;
}


/* hac 14.61, pp608 */
int mp_invmod_slow (mp_int * a, mp_int * b, mp_int * c)
{
  mp_int  x, y, u, v, A, B, C, D;
  int     res;

  /* b cannot be negative */
  if (b->sign == MP_NEG || mp_iszero(b) == 1) {
    return MP_VAL;
  }

  /* init temps */
  if ((res = mp_init_multi(&x, &y, &u, &v, 
                           &A, &B)) != MP_OKAY) {
     return res;
  }

  /* init rest of tmps temps */
  if ((res = mp_init_multi(&C, &D, 0, 0, 0, 0)) != MP_OKAY) {
     return res;
  }

  /* x = a, y = b */
  if ((res = mp_mod(a, b, &x)) != MP_OKAY) {
      goto LBL_ERR;
  }
  if ((res = mp_copy (b, &y)) != MP_OKAY) {
    goto LBL_ERR;
  }

  /* 2. [modified] if x,y are both even then return an error! */
  if (mp_iseven (&x) == 1 && mp_iseven (&y) == 1) {
    res = MP_VAL;
    goto LBL_ERR;
  }

  /* 3. u=x, v=y, A=1, B=0, C=0,D=1 */
  if ((res = mp_copy (&x, &u)) != MP_OKAY) {
    goto LBL_ERR;
  }
  if ((res = mp_copy (&y, &v)) != MP_OKAY) {
    goto LBL_ERR;
  }
  mp_set (&A, 1);
  mp_set (&D, 1);

top:
  /* 4.  while u is even do */
  while (mp_iseven (&u) == 1) {
    /* 4.1 u = u/2 */
    if ((res = mp_div_2 (&u, &u)) != MP_OKAY) {
      goto LBL_ERR;
    }
    /* 4.2 if A or B is odd then */
    if (mp_isodd (&A) == 1 || mp_isodd (&B) == 1) {
      /* A = (A+y)/2, B = (B-x)/2 */
      if ((res = mp_add (&A, &y, &A)) != MP_OKAY) {
         goto LBL_ERR;
      }
      if ((res = mp_sub (&B, &x, &B)) != MP_OKAY) {
         goto LBL_ERR;
      }
    }
    /* A = A/2, B = B/2 */
    if ((res = mp_div_2 (&A, &A)) != MP_OKAY) {
      goto LBL_ERR;
    }
    if ((res = mp_div_2 (&B, &B)) != MP_OKAY) {
      goto LBL_ERR;
    }
  }

  /* 5.  while v is even do */
  while (mp_iseven (&v) == 1) {
    /* 5.1 v = v/2 */
    if ((res = mp_div_2 (&v, &v)) != MP_OKAY) {
      goto LBL_ERR;
    }
    /* 5.2 if C or D is odd then */
    if (mp_isodd (&C) == 1 || mp_isodd (&D) == 1) {
      /* C = (C+y)/2, D = (D-x)/2 */
      if ((res = mp_add (&C, &y, &C)) != MP_OKAY) {
         goto LBL_ERR;
      }
      if ((res = mp_sub (&D, &x, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
    }
    /* C = C/2, D = D/2 */
    if ((res = mp_div_2 (&C, &C)) != MP_OKAY) {
      goto LBL_ERR;
    }
    if ((res = mp_div_2 (&D, &D)) != MP_OKAY) {
      goto LBL_ERR;
    }
  }

  /* 6.  if u >= v then */
  if (mp_cmp (&u, &v) != MP_LT) {
    /* u = u - v, A = A - C, B = B - D */
    if ((res = mp_sub (&u, &v, &u)) != MP_OKAY) {
      goto LBL_ERR;
    }

    if ((res = mp_sub (&A, &C, &A)) != MP_OKAY) {
      goto LBL_ERR;
    }

    if ((res = mp_sub (&B, &D, &B)) != MP_OKAY) {
      goto LBL_ERR;
    }
  } else {
    /* v - v - u, C = C - A, D = D - B */
    if ((res = mp_sub (&v, &u, &v)) != MP_OKAY) {
      goto LBL_ERR;
    }

    if ((res = mp_sub (&C, &A, &C)) != MP_OKAY) {
      goto LBL_ERR;
    }

    if ((res = mp_sub (&D, &B, &D)) != MP_OKAY) {
      goto LBL_ERR;
    }
  }

  /* if not zero goto step 4 */
  if (mp_iszero (&u) == 0)
    goto top;

  /* now a = C, b = D, gcd == g*v */

  /* if v != 1 then there is no inverse */
  if (mp_cmp_d (&v, 1) != MP_EQ) {
    res = MP_VAL;
    goto LBL_ERR;
  }

  /* if its too low */
  while (mp_cmp_d(&C, 0) == MP_LT) {
      if ((res = mp_add(&C, b, &C)) != MP_OKAY) {
         goto LBL_ERR;
      }
  }
  
  /* too big */
  while (mp_cmp_mag(&C, b) != MP_LT) {
      if ((res = mp_sub(&C, b, &C)) != MP_OKAY) {
         goto LBL_ERR;
      }
  }
  
  /* C is now the inverse */
  mp_exch (&C, c);
  res = MP_OKAY;
LBL_ERR:mp_clear(&x);
        mp_clear(&y);
        mp_clear(&u);
        mp_clear(&v);
        mp_clear(&A);
        mp_clear(&B);
        mp_clear(&C);
        mp_clear(&D);
  return res;
}


/* compare maginitude of two ints (unsigned) */
int mp_cmp_mag (mp_int * a, mp_int * b)
{
  int     n;
  mp_digit *tmpa, *tmpb;

  /* compare based on # of non-zero digits */
  if (a->used > b->used) {
    return MP_GT;
  }
  
  if (a->used < b->used) {
    return MP_LT;
  }

  /* alias for a */
  tmpa = a->dp + (a->used - 1);

  /* alias for b */
  tmpb = b->dp + (a->used - 1);

  /* compare based on digits  */
  for (n = 0; n < a->used; ++n, --tmpa, --tmpb) {
    if (*tmpa > *tmpb) {
      return MP_GT;
    }

    if (*tmpa < *tmpb) {
      return MP_LT;
    }
  }
  return MP_EQ;
}


/* compare two ints (signed)*/
int
mp_cmp (mp_int * a, mp_int * b)
{
  /* compare based on sign */
  if (a->sign != b->sign) {
     if (a->sign == MP_NEG) {
        return MP_LT;
     } else {
        return MP_GT;
     }
  }
  
  /* compare digits */
  if (a->sign == MP_NEG) {
     /* if negative compare opposite direction */
     return mp_cmp_mag(b, a);
  } else {
     return mp_cmp_mag(a, b);
  }
}


/* compare a digit */
int mp_cmp_d(mp_int * a, mp_digit b)
{
  /* compare based on sign */
  if (a->sign == MP_NEG) {
    return MP_LT;
  }

  /* compare based on magnitude */
  if (a->used > 1) {
    return MP_GT;
  }

  /* compare the only digit of a to b */
  if (a->dp[0] > b) {
    return MP_GT;
  } else if (a->dp[0] < b) {
    return MP_LT;
  } else {
    return MP_EQ;
  }
}


/* set to a digit */
void mp_set (mp_int * a, mp_digit b)
{
  mp_zero (a);
  a->dp[0] = b & MP_MASK;
  a->used  = (a->dp[0] != 0) ? 1 : 0;
}


/* c = a mod b, 0 <= c < b */
int
mp_mod (mp_int * a, mp_int * b, mp_int * c)
{
  mp_int  t;
  int     res;

  if ((res = mp_init (&t)) != MP_OKAY) {
    return res;
  }

  if ((res = mp_div (a, b, NULL, &t)) != MP_OKAY) {
    mp_clear (&t);
    return res;
  }

  if (t.sign != b->sign) {
    res = mp_add (b, &t, c);
  } else {
    res = MP_OKAY;
    mp_exch (&t, c);
  }

  mp_clear (&t);
  return res;
}


/* slower bit-bang division... also smaller */
int mp_div(mp_int * a, mp_int * b, mp_int * c, mp_int * d)
{
   mp_int ta, tb, tq, q;
   int    res, n, n2;

  /* is divisor zero ? */
  if (mp_iszero (b) == 1) {
    return MP_VAL;
  }

  /* if a < b then q=0, r = a */
  if (mp_cmp_mag (a, b) == MP_LT) {
    if (d != NULL) {
      res = mp_copy (a, d);
    } else {
      res = MP_OKAY;
    }
    if (c != NULL) {
      mp_zero (c);
    }
    return res;
  }
	
  /* init our temps */
  if ((res = mp_init_multi(&ta, &tb, &tq, &q, 0, 0) != MP_OKAY)) {
     return res;
  }


  mp_set(&tq, 1);
  n = mp_count_bits(a) - mp_count_bits(b);
  if (((res = mp_abs(a, &ta)) != MP_OKAY) ||
      ((res = mp_abs(b, &tb)) != MP_OKAY) || 
      ((res = mp_mul_2d(&tb, n, &tb)) != MP_OKAY) ||
      ((res = mp_mul_2d(&tq, n, &tq)) != MP_OKAY)) {
      goto LBL_ERR;
  }

  while (n-- >= 0) {
     if (mp_cmp(&tb, &ta) != MP_GT) {
        if (((res = mp_sub(&ta, &tb, &ta)) != MP_OKAY) ||
            ((res = mp_add(&q, &tq, &q)) != MP_OKAY)) {
           goto LBL_ERR;
        }
     }
     if (((res = mp_div_2d(&tb, 1, &tb, NULL)) != MP_OKAY) ||
         ((res = mp_div_2d(&tq, 1, &tq, NULL)) != MP_OKAY)) {
           goto LBL_ERR;
     }
  }

  /* now q == quotient and ta == remainder */
  n  = a->sign;
  n2 = (a->sign == b->sign ? MP_ZPOS : MP_NEG);
  if (c != NULL) {
     mp_exch(c, &q);
     c->sign  = (mp_iszero(c) == MP_YES) ? MP_ZPOS : n2;
  }
  if (d != NULL) {
     mp_exch(d, &ta);
     d->sign = (mp_iszero(d) == MP_YES) ? MP_ZPOS : n;
  }
LBL_ERR:
   mp_clear(&ta);
   mp_clear(&tb);
   mp_clear(&tq);
   mp_clear(&q);
   return res;
}


/* b = a/2 */
int mp_div_2(mp_int * a, mp_int * b)
{
  int     x, res, oldused;

  /* copy */
  if (b->alloc < a->used) {
    if ((res = mp_grow (b, a->used)) != MP_OKAY) {
      return res;
    }
  }

  oldused = b->used;
  b->used = a->used;
  {
    register mp_digit r, rr, *tmpa, *tmpb;

    /* source alias */
    tmpa = a->dp + b->used - 1;

    /* dest alias */
    tmpb = b->dp + b->used - 1;

    /* carry */
    r = 0;
    for (x = b->used - 1; x >= 0; x--) {
      /* get the carry for the next iteration */
      rr = *tmpa & 1;

      /* shift the current digit, add in carry and store */
      *tmpb-- = (*tmpa-- >> 1) | (r << (DIGIT_BIT - 1));

      /* forward carry to next iteration */
      r = rr;
    }

    /* zero excess digits */
    tmpb = b->dp + b->used;
    for (x = b->used; x < oldused; x++) {
      *tmpb++ = 0;
    }
  }
  b->sign = a->sign;
  mp_clamp (b);
  return MP_OKAY;
}


/* high level addition (handles signs) */
int mp_add (mp_int * a, mp_int * b, mp_int * c)
{
  int     sa, sb, res;

  /* get sign of both inputs */
  sa = a->sign;
  sb = b->sign;

  /* handle two cases, not four */
  if (sa == sb) {
    /* both positive or both negative */
    /* add their magnitudes, copy the sign */
    c->sign = sa;
    res = s_mp_add (a, b, c);
  } else {
    /* one positive, the other negative */
    /* subtract the one with the greater magnitude from */
    /* the one of the lesser magnitude.  The result gets */
    /* the sign of the one with the greater magnitude. */
    if (mp_cmp_mag (a, b) == MP_LT) {
      c->sign = sb;
      res = s_mp_sub (b, a, c);
    } else {
      c->sign = sa;
      res = s_mp_sub (a, b, c);
    }
  }
  return res;
}


/* low level addition, based on HAC pp.594, Algorithm 14.7 */
int
s_mp_add (mp_int * a, mp_int * b, mp_int * c)
{
  mp_int *x;
  int     olduse, res, min, max;

  /* find sizes, we let |a| <= |b| which means we have to sort
   * them.  "x" will point to the input with the most digits
   */
  if (a->used > b->used) {
    min = b->used;
    max = a->used;
    x = a;
  } else {
    min = a->used;
    max = b->used;
    x = b;
  }

  /* init result */
  if (c->alloc < max + 1) {
    if ((res = mp_grow (c, max + 1)) != MP_OKAY) {
      return res;
    }
  }

  /* get old used digit count and set new one */
  olduse = c->used;
  c->used = max + 1;

  {
    register mp_digit u, *tmpa, *tmpb, *tmpc;
    register int i;

    /* alias for digit pointers */

    /* first input */
    tmpa = a->dp;

    /* second input */
    tmpb = b->dp;

    /* destination */
    tmpc = c->dp;

    /* zero the carry */
    u = 0;
    for (i = 0; i < min; i++) {
      /* Compute the sum at one digit, T[i] = A[i] + B[i] + U */
      *tmpc = *tmpa++ + *tmpb++ + u;

      /* U = carry bit of T[i] */
      u = *tmpc >> ((mp_digit)DIGIT_BIT);

      /* take away carry bit from T[i] */
      *tmpc++ &= MP_MASK;
    }

    /* now copy higher words if any, that is in A+B 
     * if A or B has more digits add those in 
     */
    if (min != max) {
      for (; i < max; i++) {
        /* T[i] = X[i] + U */
        *tmpc = x->dp[i] + u;

        /* U = carry bit of T[i] */
        u = *tmpc >> ((mp_digit)DIGIT_BIT);

        /* take away carry bit from T[i] */
        *tmpc++ &= MP_MASK;
      }
    }

    /* add carry */
    *tmpc++ = u;

    /* clear digits above oldused */
    for (i = c->used; i < olduse; i++) {
      *tmpc++ = 0;
    }
  }

  mp_clamp (c);
  return MP_OKAY;
}


/* low level subtraction (assumes |a| > |b|), HAC pp.595 Algorithm 14.9 */
int
s_mp_sub (mp_int * a, mp_int * b, mp_int * c)
{
  int     olduse, res, min, max;

  /* find sizes */
  min = b->used;
  max = a->used;

  /* init result */
  if (c->alloc < max) {
    if ((res = mp_grow (c, max)) != MP_OKAY) {
      return res;
    }
  }
  olduse = c->used;
  c->used = max;

  {
    register mp_digit u, *tmpa, *tmpb, *tmpc;
    register int i;

    /* alias for digit pointers */
    tmpa = a->dp;
    tmpb = b->dp;
    tmpc = c->dp;

    /* set carry to zero */
    u = 0;
    for (i = 0; i < min; i++) {
      /* T[i] = A[i] - B[i] - U */
      *tmpc = *tmpa++ - *tmpb++ - u;

      /* U = carry bit of T[i]
       * Note this saves performing an AND operation since
       * if a carry does occur it will propagate all the way to the
       * MSB.  As a result a single shift is enough to get the carry
       */
      u = *tmpc >> ((mp_digit)(CHAR_BIT * sizeof (mp_digit) - 1));

      /* Clear carry from T[i] */
      *tmpc++ &= MP_MASK;
    }

    /* now copy higher words if any, e.g. if A has more digits than B  */
    for (; i < max; i++) {
      /* T[i] = A[i] - U */
      *tmpc = *tmpa++ - u;

      /* U = carry bit of T[i] */
      u = *tmpc >> ((mp_digit)(CHAR_BIT * sizeof (mp_digit) - 1));

      /* Clear carry from T[i] */
      *tmpc++ &= MP_MASK;
    }

    /* clear digits above used (since we may not have grown result above) */
    for (i = c->used; i < olduse; i++) {
      *tmpc++ = 0;
    }
  }

  mp_clamp (c);
  return MP_OKAY;
}


/* high level subtraction (handles signs) */
int
mp_sub (mp_int * a, mp_int * b, mp_int * c)
{
  int     sa, sb, res;

  sa = a->sign;
  sb = b->sign;

  if (sa != sb) {
    /* subtract a negative from a positive, OR */
    /* subtract a positive from a negative. */
    /* In either case, ADD their magnitudes, */
    /* and use the sign of the first number. */
    c->sign = sa;
    res = s_mp_add (a, b, c);
  } else {
    /* subtract a positive from a positive, OR */
    /* subtract a negative from a negative. */
    /* First, take the difference between their */
    /* magnitudes, then... */
    if (mp_cmp_mag (a, b) != MP_LT) {
      /* Copy the sign from the first */
      c->sign = sa;
      /* The first has a larger or equal magnitude */
      res = s_mp_sub (a, b, c);
    } else {
      /* The result has the *opposite* sign from */
      /* the first number. */
      c->sign = (sa == MP_ZPOS) ? MP_NEG : MP_ZPOS;
      /* The second has a larger magnitude */
      res = s_mp_sub (b, a, c);
    }
  }
  return res;
}


/* determines if reduce_2k_l can be used */
int mp_reduce_is_2k_l(mp_int *a)
{
   int ix, iy;
   
   if (a->used == 0) {
      return MP_NO;
   } else if (a->used == 1) {
      return MP_YES;
   } else if (a->used > 1) {
      /* if more than half of the digits are -1 we're sold */
      for (iy = ix = 0; ix < a->used; ix++) {
          if (a->dp[ix] == MP_MASK) {
              ++iy;
          }
      }
      return (iy >= (a->used/2)) ? MP_YES : MP_NO;
      
   }
   return MP_NO;
}


/* determines if mp_reduce_2k can be used */
int mp_reduce_is_2k(mp_int *a)
{
   int ix, iy, iw;
   mp_digit iz;
   
   if (a->used == 0) {
      return MP_NO;
   } else if (a->used == 1) {
      return MP_YES;
   } else if (a->used > 1) {
      iy = mp_count_bits(a);
      iz = 1;
      iw = 1;
    
      /* Test every bit from the second digit up, must be 1 */
      for (ix = DIGIT_BIT; ix < iy; ix++) {
          if ((a->dp[iw] & iz) == 0) {
             return MP_NO;
          }
          iz <<= 1;
          if (iz > (mp_digit)MP_MASK) {
             ++iw;
             iz = 1;
          }
      }
   }
   return MP_YES;
}


/* determines if a number is a valid DR modulus */
int mp_dr_is_modulus(mp_int *a)
{
   int ix;

   /* must be at least two digits */
   if (a->used < 2) {
      return 0;
   }

   /* must be of the form b**k - a [a <= b] so all
    * but the first digit must be equal to -1 (mod b).
    */
   for (ix = 1; ix < a->used; ix++) {
       if (a->dp[ix] != MP_MASK) {
          return 0;
       }
   }
   return 1;
}


/* computes Y == G**X mod P, HAC pp.616, Algorithm 14.85
 *
 * Uses a left-to-right k-ary sliding window to compute the modular
 * exponentiation.
 * The value of k changes based on the size of the exponent.
 *
 * Uses Montgomery or Diminished Radix reduction [whichever appropriate]
 */

#ifdef MP_LOW_MEM
   #define TAB_SIZE 32
#else
   #define TAB_SIZE 256
#endif

int mp_exptmod_fast (mp_int * G, mp_int * X, mp_int * P, mp_int * Y,
                     int redmode)
{
  mp_int  M[TAB_SIZE], res;
  mp_digit buf, mp;
  int     err, bitbuf, bitcpy, bitcnt, mode, digidx, x, y, winsize;

  /* use a pointer to the reduction algorithm.  This allows us to use
   * one of many reduction algorithms without modding the guts of
   * the code with if statements everywhere.
   */
  int     (*redux)(mp_int*,mp_int*,mp_digit);

  /* find window size */
  x = mp_count_bits (X);
  if (x <= 7) {
    winsize = 2;
  } else if (x <= 36) {
    winsize = 3;
  } else if (x <= 140) {
    winsize = 4;
  } else if (x <= 450) {
    winsize = 5;
  } else if (x <= 1303) {
    winsize = 6;
  } else if (x <= 3529) {
    winsize = 7;
  } else {
    winsize = 8;
  }

#ifdef MP_LOW_MEM
  if (winsize > 5) {
     winsize = 5;
  }
#endif

  /* init M array */
  /* init first cell */
  if ((err = mp_init(&M[1])) != MP_OKAY) {
     return err;
  }

  /* now init the second half of the array */
  for (x = 1<<(winsize-1); x < (1 << winsize); x++) {
    if ((err = mp_init(&M[x])) != MP_OKAY) {
      for (y = 1<<(winsize-1); y < x; y++) {
        mp_clear (&M[y]);
      }
      mp_clear(&M[1]);
      return err;
    }
  }

  /* determine and setup reduction code */
  if (redmode == 0) {
#ifdef BN_MP_MONTGOMERY_SETUP_C     
     /* now setup montgomery  */
     if ((err = mp_montgomery_setup (P, &mp)) != MP_OKAY) {
        goto LBL_M;
     }
#else
     err = MP_VAL;
     goto LBL_M;
#endif

     /* automatically pick the comba one if available (saves quite a few
        calls/ifs) */
#ifdef BN_FAST_MP_MONTGOMERY_REDUCE_C
     if (((P->used * 2 + 1) < MP_WARRAY) &&
          P->used < (1 << ((CHAR_BIT * sizeof (mp_word)) - (2 * DIGIT_BIT)))) {
        redux = fast_mp_montgomery_reduce;
     } else 
#endif
     {
#ifdef BN_MP_MONTGOMERY_REDUCE_C
        /* use slower baseline Montgomery method */
        redux = mp_montgomery_reduce;
#else
        err = MP_VAL;
        goto LBL_M;
#endif
     }
  } else if (redmode == 1) {
#if defined(BN_MP_DR_SETUP_C) && defined(BN_MP_DR_REDUCE_C)
     /* setup DR reduction for moduli of the form B**k - b */
     mp_dr_setup(P, &mp);
     redux = mp_dr_reduce;
#else
     err = MP_VAL;
     goto LBL_M;
#endif
  } else {
#if defined(BN_MP_REDUCE_2K_SETUP_C) && defined(BN_MP_REDUCE_2K_C)
     /* setup DR reduction for moduli of the form 2**k - b */
     if ((err = mp_reduce_2k_setup(P, &mp)) != MP_OKAY) {
        goto LBL_M;
     }
     redux = mp_reduce_2k;
#else
     err = MP_VAL;
     goto LBL_M;
#endif
  }

  /* setup result */
  if ((err = mp_init (&res)) != MP_OKAY) {
    goto LBL_M;
  }

  /* create M table
   *

   *
   * The first half of the table is not computed though accept for M[0] and M[1]
   */

  if (redmode == 0) {
#ifdef BN_MP_MONTGOMERY_CALC_NORMALIZATION_C
     /* now we need R mod m */
     if ((err = mp_montgomery_calc_normalization (&res, P)) != MP_OKAY) {
       goto LBL_RES;
     }
#else 
     err = MP_VAL;
     goto LBL_RES;
#endif

     /* now set M[1] to G * R mod m */
     if ((err = mp_mulmod (G, &res, P, &M[1])) != MP_OKAY) {
       goto LBL_RES;
     }
  } else {
     mp_set(&res, 1);
     if ((err = mp_mod(G, P, &M[1])) != MP_OKAY) {
        goto LBL_RES;
     }
  }

  /* compute the value at M[1<<(winsize-1)] by squaring M[1] (winsize-1) times*/
  if ((err = mp_copy (&M[1], &M[1 << (winsize - 1)])) != MP_OKAY) {
    goto LBL_RES;
  }

  for (x = 0; x < (winsize - 1); x++) {
    if ((err = mp_sqr (&M[1 << (winsize - 1)], &M[1 << (winsize - 1)])) != MP_OKAY) {
      goto LBL_RES;
    }
    if ((err = redux (&M[1 << (winsize - 1)], P, mp)) != MP_OKAY) {
      goto LBL_RES;
    }
  }

  /* create upper table */
  for (x = (1 << (winsize - 1)) + 1; x < (1 << winsize); x++) {
    if ((err = mp_mul (&M[x - 1], &M[1], &M[x])) != MP_OKAY) {
      goto LBL_RES;
    }
    if ((err = redux (&M[x], P, mp)) != MP_OKAY) {
      goto LBL_RES;
    }
  }

  /* set initial mode and bit cnt */
  mode   = 0;
  bitcnt = 1;
  buf    = 0;
  digidx = X->used - 1;
  bitcpy = 0;
  bitbuf = 0;

  for (;;) {
    /* grab next digit as required */
    if (--bitcnt == 0) {
      /* if digidx == -1 we are out of digits so break */
      if (digidx == -1) {
        break;
      }
      /* read next digit and reset bitcnt */
      buf    = X->dp[digidx--];
      bitcnt = (int)DIGIT_BIT;
    }

    /* grab the next msb from the exponent */
    y     = (mp_digit)(buf >> (DIGIT_BIT - 1)) & 1;
    buf <<= (mp_digit)1;

    /* if the bit is zero and mode == 0 then we ignore it
     * These represent the leading zero bits before the first 1 bit
     * in the exponent.  Technically this opt is not required but it
     * does lower the # of trivial squaring/reductions used
     */
    if (mode == 0 && y == 0) {
      continue;
    }

    /* if the bit is zero and mode == 1 then we square */
    if (mode == 1 && y == 0) {
      if ((err = mp_sqr (&res, &res)) != MP_OKAY) {
        goto LBL_RES;
      }
      if ((err = redux (&res, P, mp)) != MP_OKAY) {
        goto LBL_RES;
      }
      continue;
    }

    /* else we add it to the window */
    bitbuf |= (y << (winsize - ++bitcpy));
    mode    = 2;

    if (bitcpy == winsize) {
      /* ok window is filled so square as required and multiply  */
      /* square first */
      for (x = 0; x < winsize; x++) {
        if ((err = mp_sqr (&res, &res)) != MP_OKAY) {
          goto LBL_RES;
        }
        if ((err = redux (&res, P, mp)) != MP_OKAY) {
          goto LBL_RES;
        }
      }

      /* then multiply */
      if ((err = mp_mul (&res, &M[bitbuf], &res)) != MP_OKAY) {
        goto LBL_RES;
      }
      if ((err = redux (&res, P, mp)) != MP_OKAY) {
        goto LBL_RES;
      }

      /* empty window and reset */
      bitcpy = 0;
      bitbuf = 0;
      mode   = 1;
    }
  }

  /* if bits remain then square/multiply */
  if (mode == 2 && bitcpy > 0) {
    /* square then multiply if the bit is set */
    for (x = 0; x < bitcpy; x++) {
      if ((err = mp_sqr (&res, &res)) != MP_OKAY) {
        goto LBL_RES;
      }
      if ((err = redux (&res, P, mp)) != MP_OKAY) {
        goto LBL_RES;
      }

      /* get next bit of the window */
      bitbuf <<= 1;
      if ((bitbuf & (1 << winsize)) != 0) {
        /* then multiply */
        if ((err = mp_mul (&res, &M[1], &res)) != MP_OKAY) {
          goto LBL_RES;
        }
        if ((err = redux (&res, P, mp)) != MP_OKAY) {
          goto LBL_RES;
        }
      }
    }
  }

  if (redmode == 0) {
     /* fixup result if Montgomery reduction is used
      * recall that any value in a Montgomery system is
      * actually multiplied by R mod n.  So we have
      * to reduce one more time to cancel out the factor
      * of R.
      */
     if ((err = redux(&res, P, mp)) != MP_OKAY) {
       goto LBL_RES;
     }
  }

  /* swap res with Y */
  mp_exch (&res, Y);
  err = MP_OKAY;
LBL_RES:mp_clear (&res);
LBL_M:
  mp_clear(&M[1]);
  for (x = 1<<(winsize-1); x < (1 << winsize); x++) {
    mp_clear (&M[x]);
  }
  return err;
}


/* setups the montgomery reduction stuff */
int
mp_montgomery_setup (mp_int * n, mp_digit * rho)
{
  mp_digit x, b;

/* fast inversion mod 2**k
 *
 * Based on the fact that
 *
 * XA = 1 (mod 2**n)  =>  (X(2-XA)) A = 1 (mod 2**2n)
 *                    =>  2*X*A - X*X*A*A = 1
 *                    =>  2*(1) - (1)     = 1
 */
  b = n->dp[0];

  if ((b & 1) == 0) {
    return MP_VAL;
  }

  x = (((b + 2) & 4) << 1) + b; /* here x*a==1 mod 2**4 */
  x *= 2 - b * x;               /* here x*a==1 mod 2**8 */
#if !defined(MP_8BIT)
  x *= 2 - b * x;               /* here x*a==1 mod 2**16 */
#endif
#if defined(MP_64BIT) || !(defined(MP_8BIT) || defined(MP_16BIT))
  x *= 2 - b * x;               /* here x*a==1 mod 2**32 */
#endif
#ifdef MP_64BIT
  x *= 2 - b * x;               /* here x*a==1 mod 2**64 */
#endif

  /* rho = -1/m mod b */
  /* TAO, switched mp_word casts to mp_digit to shut up compiler */
  *rho = (((mp_digit)1 << ((mp_digit) DIGIT_BIT)) - x) & MP_MASK;

  return MP_OKAY;
}


/* computes xR**-1 == x (mod N) via Montgomery Reduction
 *
 * This is an optimized implementation of montgomery_reduce
 * which uses the comba method to quickly calculate the columns of the
 * reduction.
 *
 * Based on Algorithm 14.32 on pp.601 of HAC.
*/
int fast_mp_montgomery_reduce (mp_int * x, mp_int * n, mp_digit rho)
{
  int     ix, res, olduse;
  mp_word W[MP_WARRAY];

  /* get old used count */
  olduse = x->used;

  /* grow a as required */
  if (x->alloc < n->used + 1) {
    if ((res = mp_grow (x, n->used + 1)) != MP_OKAY) {
      return res;
    }
  }

  /* first we have to get the digits of the input into
   * an array of double precision words W[...]
   */
  {
    register mp_word *_W;
    register mp_digit *tmpx;

    /* alias for the W[] array */
    _W   = W;

    /* alias for the digits of  x*/
    tmpx = x->dp;

    /* copy the digits of a into W[0..a->used-1] */
    for (ix = 0; ix < x->used; ix++) {
      *_W++ = *tmpx++;
    }

    /* zero the high words of W[a->used..m->used*2] */
    for (; ix < n->used * 2 + 1; ix++) {
      *_W++ = 0;
    }
  }

  /* now we proceed to zero successive digits
   * from the least significant upwards
   */
  for (ix = 0; ix < n->used; ix++) {
    /* mu = ai * m' mod b
     *
     * We avoid a double precision multiplication (which isn't required)
     * by casting the value down to a mp_digit.  Note this requires
     * that W[ix-1] have  the carry cleared (see after the inner loop)
     */
    register mp_digit mu;
    mu = (mp_digit) (((W[ix] & MP_MASK) * rho) & MP_MASK);

    /* a = a + mu * m * b**i
     *
     * This is computed in place and on the fly.  The multiplication
     * by b**i is handled by offseting which columns the results
     * are added to.
     *
     * Note the comba method normally doesn't handle carries in the
     * inner loop In this case we fix the carry from the previous
     * column since the Montgomery reduction requires digits of the
     * result (so far) [see above] to work.  This is
     * handled by fixing up one carry after the inner loop.  The
     * carry fixups are done in order so after these loops the
     * first m->used words of W[] have the carries fixed
     */
    {
      register int iy;
      register mp_digit *tmpn;
      register mp_word *_W;

      /* alias for the digits of the modulus */
      tmpn = n->dp;

      /* Alias for the columns set by an offset of ix */
      _W = W + ix;

      /* inner loop */
      for (iy = 0; iy < n->used; iy++) {
          *_W++ += ((mp_word)mu) * ((mp_word)*tmpn++);
      }
    }

    /* now fix carry for next digit, W[ix+1] */
    W[ix + 1] += W[ix] >> ((mp_word) DIGIT_BIT);
  }

  /* now we have to propagate the carries and
   * shift the words downward [all those least
   * significant digits we zeroed].
   */
  {
    register mp_digit *tmpx;
    register mp_word *_W, *_W1;

    /* nox fix rest of carries */

    /* alias for current word */
    _W1 = W + ix;

    /* alias for next word, where the carry goes */
    _W = W + ++ix;

    for (; ix <= n->used * 2 + 1; ix++) {
      *_W++ += *_W1++ >> ((mp_word) DIGIT_BIT);
    }

    /* copy out, A = A/b**n
     *
     * The result is A/b**n but instead of converting from an
     * array of mp_word to mp_digit than calling mp_rshd
     * we just copy them in the right order
     */

    /* alias for destination word */
    tmpx = x->dp;

    /* alias for shifted double precision result */
    _W = W + n->used;

    for (ix = 0; ix < n->used + 1; ix++) {
      *tmpx++ = (mp_digit)(*_W++ & ((mp_word) MP_MASK));
    }

    /* zero oldused digits, if the input a was larger than
     * m->used+1 we'll have to clear the digits
     */
    for (; ix < olduse; ix++) {
      *tmpx++ = 0;
    }
  }

  /* set the max used and clamp */
  x->used = n->used + 1;
  mp_clamp (x);

  /* if A >= m then A = A - m */
  if (mp_cmp_mag (x, n) != MP_LT) {
    return s_mp_sub (x, n, x);
  }
  return MP_OKAY;
}


/* computes xR**-1 == x (mod N) via Montgomery Reduction */
int
mp_montgomery_reduce (mp_int * x, mp_int * n, mp_digit rho)
{
  int     ix, res, digs;
  mp_digit mu;

  /* can the fast reduction [comba] method be used?
   *
   * Note that unlike in mul you're safely allowed *less*
   * than the available columns [255 per default] since carries
   * are fixed up in the inner loop.
   */
  digs = n->used * 2 + 1;
  if ((digs < MP_WARRAY) &&
      n->used <
      (1 << ((CHAR_BIT * sizeof (mp_word)) - (2 * DIGIT_BIT)))) {
    return fast_mp_montgomery_reduce (x, n, rho);
  }

  /* grow the input as required */
  if (x->alloc < digs) {
    if ((res = mp_grow (x, digs)) != MP_OKAY) {
      return res;
    }
  }
  x->used = digs;

  for (ix = 0; ix < n->used; ix++) {
    /* mu = ai * rho mod b
     *
     * The value of rho must be precalculated via
     * montgomery_setup() such that
     * it equals -1/n0 mod b this allows the
     * following inner loop to reduce the
     * input one digit at a time
     */
    mu = (mp_digit) (((mp_word)x->dp[ix]) * ((mp_word)rho) & MP_MASK);

    /* a = a + mu * m * b**i */
    {
      register int iy;
      register mp_digit *tmpn, *tmpx, u;
      register mp_word r;

      /* alias for digits of the modulus */
      tmpn = n->dp;

      /* alias for the digits of x [the input] */
      tmpx = x->dp + ix;

      /* set the carry to zero */
      u = 0;

      /* Multiply and add in place */
      for (iy = 0; iy < n->used; iy++) {
        /* compute product and sum */
        r       = ((mp_word)mu) * ((mp_word)*tmpn++) +
                  ((mp_word) u) + ((mp_word) * tmpx);

        /* get carry */
        u       = (mp_digit)(r >> ((mp_word) DIGIT_BIT));

        /* fix digit */
        *tmpx++ = (mp_digit)(r & ((mp_word) MP_MASK));
      }
      /* At this point the ix'th digit of x should be zero */


      /* propagate carries upwards as required*/
      while (u) {
        *tmpx   += u;
        u        = *tmpx >> DIGIT_BIT;
        *tmpx++ &= MP_MASK;
      }
    }
  }

  /* at this point the n.used'th least
   * significant digits of x are all zero
   * which means we can shift x to the
   * right by n.used digits and the
   * residue is unchanged.
   */

  /* x = x/b**n.used */
  mp_clamp(x);
  mp_rshd (x, n->used);

  /* if x >= n then x = x - n */
  if (mp_cmp_mag (x, n) != MP_LT) {
    return s_mp_sub (x, n, x);
  }

  return MP_OKAY;
}


/* determines the setup value */
void mp_dr_setup(mp_int *a, mp_digit *d)
{
   /* the casts are required if DIGIT_BIT is one less than
    * the number of bits in a mp_digit [e.g. DIGIT_BIT==31]
    */
   *d = (mp_digit)((((mp_word)1) << ((mp_word)DIGIT_BIT)) - 
        ((mp_word)a->dp[0]));
}


/* reduce "x" in place modulo "n" using the Diminished Radix algorithm.
 *
 * Based on algorithm from the paper
 *
 * "Generating Efficient Primes for Discrete Log Cryptosystems"
 *                 Chae Hoon Lim, Pil Joong Lee,
 *          POSTECH Information Research Laboratories
 *
 * The modulus must be of a special format [see manual]
 *
 * Has been modified to use algorithm 7.10 from the LTM book instead
 *
 * Input x must be in the range 0 <= x <= (n-1)**2
 */
int
mp_dr_reduce (mp_int * x, mp_int * n, mp_digit k)
{
  int      err, i, m;
  mp_word  r;
  mp_digit mu, *tmpx1, *tmpx2;

  /* m = digits in modulus */
  m = n->used;

  /* ensure that "x" has at least 2m digits */
  if (x->alloc < m + m) {
    if ((err = mp_grow (x, m + m)) != MP_OKAY) {
      return err;
    }
  }

/* top of loop, this is where the code resumes if
 * another reduction pass is required.
 */
top:
  /* aliases for digits */
  /* alias for lower half of x */
  tmpx1 = x->dp;

  /* alias for upper half of x, or x/B**m */
  tmpx2 = x->dp + m;

  /* set carry to zero */
  mu = 0;

  /* compute (x mod B**m) + k * [x/B**m] inline and inplace */
  for (i = 0; i < m; i++) {
      r         = ((mp_word)*tmpx2++) * ((mp_word)k) + *tmpx1 + mu;
      *tmpx1++  = (mp_digit)(r & MP_MASK);
      mu        = (mp_digit)(r >> ((mp_word)DIGIT_BIT));
  }

  /* set final carry */
  *tmpx1++ = mu;

  /* zero words above m */
  for (i = m + 1; i < x->used; i++) {
      *tmpx1++ = 0;
  }

  /* clamp, sub and return */
  mp_clamp (x);

  /* if x >= n then subtract and reduce again
   * Each successive "recursion" makes the input smaller and smaller.
   */
  if (mp_cmp_mag (x, n) != MP_LT) {
    s_mp_sub(x, n, x);
    goto top;
  }
  return MP_OKAY;
}


/* reduces a modulo n where n is of the form 2**p - d */
int mp_reduce_2k(mp_int *a, mp_int *n, mp_digit d)
{
   mp_int q;
   int    p, res;
   
   if ((res = mp_init(&q)) != MP_OKAY) {
      return res;
   }
   
   p = mp_count_bits(n);    
top:
   /* q = a/2**p, a = a mod 2**p */
   if ((res = mp_div_2d(a, p, &q, a)) != MP_OKAY) {
      goto ERR;
   }
   
   if (d != 1) {
      /* q = q * d */
      if ((res = mp_mul_d(&q, d, &q)) != MP_OKAY) { 
         goto ERR;
      }
   }
   
   /* a = a + q */
   if ((res = s_mp_add(a, &q, a)) != MP_OKAY) {
      goto ERR;
   }
   
   if (mp_cmp_mag(a, n) != MP_LT) {
      s_mp_sub(a, n, a);
      goto top;
   }
   
ERR:
   mp_clear(&q);
   return res;
}


/* determines the setup value */
int mp_reduce_2k_setup(mp_int *a, mp_digit *d)
{
   int res, p;
   mp_int tmp;
   
   if ((res = mp_init(&tmp)) != MP_OKAY) {
      return res;
   }
   
   p = mp_count_bits(a);
   if ((res = mp_2expt(&tmp, p)) != MP_OKAY) {
      mp_clear(&tmp);
      return res;
   }
   
   if ((res = s_mp_sub(&tmp, a, &tmp)) != MP_OKAY) {
      mp_clear(&tmp);
      return res;
   }
   
   *d = tmp.dp[0];
   mp_clear(&tmp);
   return MP_OKAY;
}


/* computes a = 2**b 
 *
 * Simple algorithm which zeroes the int, grows it then just sets one bit
 * as required.
 */
int
mp_2expt (mp_int * a, int b)
{
  int     res;

  /* zero a as per default */
  mp_zero (a);

  /* grow a to accomodate the single bit */
  if ((res = mp_grow (a, b / DIGIT_BIT + 1)) != MP_OKAY) {
    return res;
  }

  /* set the used count of where the bit will go */
  a->used = b / DIGIT_BIT + 1;

  /* put the single bit in its place */
  a->dp[b / DIGIT_BIT] = ((mp_digit)1) << (b % DIGIT_BIT);

  return MP_OKAY;
}


/* multiply by a digit */
int
mp_mul_d (mp_int * a, mp_digit b, mp_int * c)
{
  mp_digit u, *tmpa, *tmpc;
  mp_word  r;
  int      ix, res, olduse;

  /* make sure c is big enough to hold a*b */
  if (c->alloc < a->used + 1) {
    if ((res = mp_grow (c, a->used + 1)) != MP_OKAY) {
      return res;
    }
  }

  /* get the original destinations used count */
  olduse = c->used;

  /* set the sign */
  c->sign = a->sign;

  /* alias for a->dp [source] */
  tmpa = a->dp;

  /* alias for c->dp [dest] */
  tmpc = c->dp;

  /* zero carry */
  u = 0;

  /* compute columns */
  for (ix = 0; ix < a->used; ix++) {
    /* compute product and carry sum for this term */
    r       = ((mp_word) u) + ((mp_word)*tmpa++) * ((mp_word)b);

    /* mask off higher bits to get a single digit */
    *tmpc++ = (mp_digit) (r & ((mp_word) MP_MASK));

    /* send carry into next iteration */
    u       = (mp_digit) (r >> ((mp_word) DIGIT_BIT));
  }

  /* store final carry [if any] and increment ix offset  */
  *tmpc++ = u;
  ++ix;

  /* now zero digits above the top */
  while (ix++ < olduse) {
     *tmpc++ = 0;
  }

  /* set used count */
  c->used = a->used + 1;
  mp_clamp(c);

  return MP_OKAY;
}


/* d = a * b (mod c) */
int mp_mulmod (mp_int * a, mp_int * b, mp_int * c, mp_int * d)
{
  int     res;
  mp_int  t;

  if ((res = mp_init (&t)) != MP_OKAY) {
    return res;
  }

  if ((res = mp_mul (a, b, &t)) != MP_OKAY) {
    mp_clear (&t);
    return res;
  }
  res = mp_mod (&t, c, d);
  mp_clear (&t);
  return res;
}


/* computes b = a*a */
int
mp_sqr (mp_int * a, mp_int * b)
{
  int     res;

  {
#ifdef BN_FAST_S_MP_SQR_C
    /* can we use the fast comba multiplier? */
    if ((a->used * 2 + 1) < MP_WARRAY && 
         a->used < 
         (1 << (sizeof(mp_word) * CHAR_BIT - 2*DIGIT_BIT - 1))) {
      res = fast_s_mp_sqr (a, b);
    } else
#endif
#ifdef BN_S_MP_SQR_C
      res = s_mp_sqr (a, b);
#else
      res = MP_VAL;
#endif
  }
  b->sign = MP_ZPOS;
  return res;
}


/* high level multiplication (handles sign) */
int mp_mul (mp_int * a, mp_int * b, mp_int * c)
{
  int     res, neg;
  neg = (a->sign == b->sign) ? MP_ZPOS : MP_NEG;

  {
    /* can we use the fast multiplier?
     *
     * The fast multiplier can be used if the output will 
     * have less than MP_WARRAY digits and the number of 
     * digits won't affect carry propagation
     */
    int     digs = a->used + b->used + 1;

#ifdef BN_FAST_S_MP_MUL_DIGS_C
    if ((digs < MP_WARRAY) &&
        MIN(a->used, b->used) <= 
        (1 << ((CHAR_BIT * sizeof (mp_word)) - (2 * DIGIT_BIT)))) {
      res = fast_s_mp_mul_digs (a, b, c, digs);
    } else 
#endif
#ifdef BN_S_MP_MUL_DIGS_C
      res = s_mp_mul (a, b, c); /* uses s_mp_mul_digs */
#else
      res = MP_VAL;
#endif

  }
  c->sign = (c->used > 0) ? neg : MP_ZPOS;
  return res;
}


/* b = a*2 */
int mp_mul_2(mp_int * a, mp_int * b)
{
  int     x, res, oldused;

  /* grow to accomodate result */
  if (b->alloc < a->used + 1) {
    if ((res = mp_grow (b, a->used + 1)) != MP_OKAY) {
      return res;
    }
  }

  oldused = b->used;
  b->used = a->used;

  {
    register mp_digit r, rr, *tmpa, *tmpb;

    /* alias for source */
    tmpa = a->dp;
    
    /* alias for dest */
    tmpb = b->dp;

    /* carry */
    r = 0;
    for (x = 0; x < a->used; x++) {
    
      /* get what will be the *next* carry bit from the 
       * MSB of the current digit 
       */
      rr = *tmpa >> ((mp_digit)(DIGIT_BIT - 1));
      
      /* now shift up this digit, add in the carry [from the previous] */
      *tmpb++ = ((*tmpa++ << ((mp_digit)1)) | r) & MP_MASK;
      
      /* copy the carry that would be from the source 
       * digit into the next iteration 
       */
      r = rr;
    }

    /* new leading digit? */
    if (r != 0) {
      /* add a MSB which is always 1 at this point */
      *tmpb = 1;
      ++(b->used);
    }

    /* now zero any excess digits on the destination 
     * that we didn't write to 
     */
    tmpb = b->dp + b->used;
    for (x = b->used; x < oldused; x++) {
      *tmpb++ = 0;
    }
  }
  b->sign = a->sign;
  return MP_OKAY;
}


/* divide by three (based on routine from MPI and the GMP manual) */
int
mp_div_3 (mp_int * a, mp_int *c, mp_digit * d)
{
  mp_int   q;
  mp_word  w, t;
  mp_digit b;
  int      res, ix;
  
  /* b = 2**DIGIT_BIT / 3 */
  b = (((mp_word)1) << ((mp_word)DIGIT_BIT)) / ((mp_word)3);

  if ((res = mp_init_size(&q, a->used)) != MP_OKAY) {
     return res;
  }
  
  q.used = a->used;
  q.sign = a->sign;
  w = 0;
  for (ix = a->used - 1; ix >= 0; ix--) {
     w = (w << ((mp_word)DIGIT_BIT)) | ((mp_word)a->dp[ix]);

     if (w >= 3) {
        /* multiply w by [1/3] */
        t = (w * ((mp_word)b)) >> ((mp_word)DIGIT_BIT);

        /* now subtract 3 * [w/3] from w, to get the remainder */
        w -= t+t+t;

        /* fixup the remainder as required since
         * the optimization is not exact.
         */
        while (w >= 3) {
           t += 1;
           w -= 3;
        }
      } else {
        t = 0;
      }
      q.dp[ix] = (mp_digit)t;
  }

  /* [optional] store the remainder */
  if (d != NULL) {
     *d = (mp_digit)w;
  }

  /* [optional] store the quotient */
  if (c != NULL) {
     mp_clamp(&q);
     mp_exch(&q, c);
  }
  mp_clear(&q);
  
  return res;
}


/* init an mp_init for a given size */
int mp_init_size (mp_int * a, int size)
{
  int x;

  /* pad size so there are always extra digits */
  size += (MP_PREC * 2) - (size % MP_PREC);	
  
  /* alloc mem */
  a->dp = OPT_CAST(mp_digit) XMALLOC (sizeof (mp_digit) * size, 0);
  if (a->dp == NULL) {
    return MP_MEM;
  }

  /* set the members */
  a->used  = 0;
  a->alloc = size;
  a->sign  = MP_ZPOS;

  /* zero the digits */
  for (x = 0; x < size; x++) {
      a->dp[x] = 0;
  }

  return MP_OKAY;
}


/* the jist of squaring...
 * you do like mult except the offset of the tmpx [one that 
 * starts closer to zero] can't equal the offset of tmpy.  
 * So basically you set up iy like before then you min it with
 * (ty-tx) so that it never happens.  You double all those 
 * you add in the inner loop

After that loop you do the squares and add them in.
*/

int fast_s_mp_sqr (mp_int * a, mp_int * b)
{
  int       olduse, res, pa, ix, iz;
  mp_digit   W[MP_WARRAY], *tmpx;
  mp_word   W1;

  /* grow the destination as required */
  pa = a->used + a->used;
  if (b->alloc < pa) {
    if ((res = mp_grow (b, pa)) != MP_OKAY) {
      return res;
    }
  }

  /* number of output digits to produce */
  W1 = 0;
  for (ix = 0; ix < pa; ix++) { 
      int      tx, ty, iy;
      mp_word  _W;
      mp_digit *tmpy;

      /* clear counter */
      _W = 0;

      /* get offsets into the two bignums */
      ty = MIN(a->used-1, ix);
      tx = ix - ty;

      /* setup temp aliases */
      tmpx = a->dp + tx;
      tmpy = a->dp + ty;

      /* this is the number of times the loop will iterrate, essentially
         while (tx++ < a->used && ty-- >= 0) { ... }
       */
      iy = MIN(a->used-tx, ty+1);

      /* now for squaring tx can never equal ty 
       * we halve the distance since they approach at a rate of 2x
       * and we have to round because odd cases need to be executed
       */
      iy = MIN(iy, (ty-tx+1)>>1);

      /* execute loop */
      for (iz = 0; iz < iy; iz++) {
         _W += ((mp_word)*tmpx++)*((mp_word)*tmpy--);
      }

      /* double the inner product and add carry */
      _W = _W + _W + W1;

      /* even columns have the square term in them */
      if ((ix&1) == 0) {
         _W += ((mp_word)a->dp[ix>>1])*((mp_word)a->dp[ix>>1]);
      }

      /* store it */
      W[ix] = (mp_digit)(_W & MP_MASK);

      /* make next carry */
      W1 = _W >> ((mp_word)DIGIT_BIT);
  }

  /* setup dest */
  olduse  = b->used;
  b->used = a->used+a->used;

  {
    mp_digit *tmpb;
    tmpb = b->dp;
    for (ix = 0; ix < pa; ix++) {
      *tmpb++ = W[ix] & MP_MASK;
    }

    /* clear unused digits [that existed in the old copy of c] */
    for (; ix < olduse; ix++) {
      *tmpb++ = 0;
    }
  }
  mp_clamp (b);
  return MP_OKAY;
}


/* Fast (comba) multiplier
 *
 * This is the fast column-array [comba] multiplier.  It is 
 * designed to compute the columns of the product first 
 * then handle the carries afterwards.  This has the effect 
 * of making the nested loops that compute the columns very
 * simple and schedulable on super-scalar processors.
 *
 * This has been modified to produce a variable number of 
 * digits of output so if say only a half-product is required 
 * you don't have to compute the upper half (a feature 
 * required for fast Barrett reduction).
 *
 * Based on Algorithm 14.12 on pp.595 of HAC.
 *
 */
int fast_s_mp_mul_digs (mp_int * a, mp_int * b, mp_int * c, int digs)
{
  int     olduse, res, pa, ix, iz;
  mp_digit W[MP_WARRAY];
  register mp_word  _W;

  /* grow the destination as required */
  if (c->alloc < digs) {
    if ((res = mp_grow (c, digs)) != MP_OKAY) {
      return res;
    }
  }

  /* number of output digits to produce */
  pa = MIN(digs, a->used + b->used);

  /* clear the carry */
  _W = 0;
  for (ix = 0; ix < pa; ix++) { 
      int      tx, ty;
      int      iy;
      mp_digit *tmpx, *tmpy;

      /* get offsets into the two bignums */
      ty = MIN(b->used-1, ix);
      tx = ix - ty;

      /* setup temp aliases */
      tmpx = a->dp + tx;
      tmpy = b->dp + ty;

      /* this is the number of times the loop will iterrate, essentially 
         while (tx++ < a->used && ty-- >= 0) { ... }
       */
      iy = MIN(a->used-tx, ty+1);

      /* execute loop */
      for (iz = 0; iz < iy; ++iz) {
         _W += ((mp_word)*tmpx++)*((mp_word)*tmpy--);

      }

      /* store term */
      W[ix] = ((mp_digit)_W) & MP_MASK;

      /* make next carry */
      _W = _W >> ((mp_word)DIGIT_BIT);
 }

  /* setup dest */
  olduse  = c->used;
  c->used = pa;

  {
    register mp_digit *tmpc;
    tmpc = c->dp;
    for (ix = 0; ix < pa+1; ix++) {
      /* now extract the previous digit [below the carry] */
      *tmpc++ = W[ix];
    }

    /* clear unused digits [that existed in the old copy of c] */
    for (; ix < olduse; ix++) {
      *tmpc++ = 0;
    }
  }
  mp_clamp (c);
  return MP_OKAY;
}


/* low level squaring, b = a*a, HAC pp.596-597, Algorithm 14.16 */
int s_mp_sqr (mp_int * a, mp_int * b)
{
  mp_int  t;
  int     res, ix, iy, pa;
  mp_word r;
  mp_digit u, tmpx, *tmpt;

  pa = a->used;
  if ((res = mp_init_size (&t, 2*pa + 1)) != MP_OKAY) {
    return res;
  }

  /* default used is maximum possible size */
  t.used = 2*pa + 1;

  for (ix = 0; ix < pa; ix++) {
    /* first calculate the digit at 2*ix */
    /* calculate double precision result */
    r = ((mp_word) t.dp[2*ix]) +
        ((mp_word)a->dp[ix])*((mp_word)a->dp[ix]);

    /* store lower part in result */
    t.dp[ix+ix] = (mp_digit) (r & ((mp_word) MP_MASK));

    /* get the carry */
    u           = (mp_digit)(r >> ((mp_word) DIGIT_BIT));

    /* left hand side of A[ix] * A[iy] */
    tmpx        = a->dp[ix];

    /* alias for where to store the results */
    tmpt        = t.dp + (2*ix + 1);
    
    for (iy = ix + 1; iy < pa; iy++) {
      /* first calculate the product */
      r       = ((mp_word)tmpx) * ((mp_word)a->dp[iy]);

      /* now calculate the double precision result, note we use
       * addition instead of *2 since it's easier to optimize
       */
      r       = ((mp_word) *tmpt) + r + r + ((mp_word) u);

      /* store lower part */
      *tmpt++ = (mp_digit) (r & ((mp_word) MP_MASK));

      /* get carry */
      u       = (mp_digit)(r >> ((mp_word) DIGIT_BIT));
    }
    /* propagate upwards */
    while (u != ((mp_digit) 0)) {
      r       = ((mp_word) *tmpt) + ((mp_word) u);
      *tmpt++ = (mp_digit) (r & ((mp_word) MP_MASK));
      u       = (mp_digit)(r >> ((mp_word) DIGIT_BIT));
    }
  }

  mp_clamp (&t);
  mp_exch (&t, b);
  mp_clear (&t);
  return MP_OKAY;
}


/* multiplies |a| * |b| and only computes upto digs digits of result
 * HAC pp. 595, Algorithm 14.12  Modified so you can control how 
 * many digits of output are created.
 */
int s_mp_mul_digs (mp_int * a, mp_int * b, mp_int * c, int digs)
{
  mp_int  t;
  int     res, pa, pb, ix, iy;
  mp_digit u;
  mp_word r;
  mp_digit tmpx, *tmpt, *tmpy;

  /* can we use the fast multiplier? */
  if (((digs) < MP_WARRAY) &&
      MIN (a->used, b->used) < 
          (1 << ((CHAR_BIT * sizeof (mp_word)) - (2 * DIGIT_BIT)))) {
    return fast_s_mp_mul_digs (a, b, c, digs);
  }

  if ((res = mp_init_size (&t, digs)) != MP_OKAY) {
    return res;
  }
  t.used = digs;

  /* compute the digits of the product directly */
  pa = a->used;
  for (ix = 0; ix < pa; ix++) {
    /* set the carry to zero */
    u = 0;

    /* limit ourselves to making digs digits of output */
    pb = MIN (b->used, digs - ix);

    /* setup some aliases */
    /* copy of the digit from a used within the nested loop */
    tmpx = a->dp[ix];
    
    /* an alias for the destination shifted ix places */
    tmpt = t.dp + ix;
    
    /* an alias for the digits of b */
    tmpy = b->dp;

    /* compute the columns of the output and propagate the carry */
    for (iy = 0; iy < pb; iy++) {
      /* compute the column as a mp_word */
      r       = ((mp_word)*tmpt) +
                ((mp_word)tmpx) * ((mp_word)*tmpy++) +
                ((mp_word) u);

      /* the new column is the lower part of the result */
      *tmpt++ = (mp_digit) (r & ((mp_word) MP_MASK));

      /* get the carry word from the result */
      u       = (mp_digit) (r >> ((mp_word) DIGIT_BIT));
    }
    /* set carry if it is placed below digs */
    if (ix + iy < digs) {
      *tmpt = u;
    }
  }

  mp_clamp (&t);
  mp_exch (&t, c);

  mp_clear (&t);
  return MP_OKAY;
}


/*
 * shifts with subtractions when the result is greater than b.
 *
 * The method is slightly modified to shift B unconditionally upto just under
 * the leading bit of b.  This saves alot of multiple precision shifting.
 */
int mp_montgomery_calc_normalization (mp_int * a, mp_int * b)
{
  int     x, bits, res;

  /* how many bits of last digit does b use */
  bits = mp_count_bits (b) % DIGIT_BIT;

  if (b->used > 1) {
     if ((res = mp_2expt (a, (b->used - 1) * DIGIT_BIT + bits - 1)) != MP_OKAY) {
        return res;
     }
  } else {
     mp_set(a, 1);
     bits = 1;
  }


  /* now compute C = A * B mod b */
  for (x = bits - 1; x < (int)DIGIT_BIT; x++) {
    if ((res = mp_mul_2 (a, a)) != MP_OKAY) {
      return res;
    }
    if (mp_cmp_mag (a, b) != MP_LT) {
      if ((res = s_mp_sub (a, b, a)) != MP_OKAY) {
        return res;
      }
    }
  }

  return MP_OKAY;
}


#ifdef MP_LOW_MEM
   #define TAB_SIZE 32
#else
   #define TAB_SIZE 256
#endif

int s_mp_exptmod (mp_int * G, mp_int * X, mp_int * P, mp_int * Y, int redmode)
{
  mp_int  M[TAB_SIZE], res, mu;
  mp_digit buf;
  int     err, bitbuf, bitcpy, bitcnt, mode, digidx, x, y, winsize;
  int (*redux)(mp_int*,mp_int*,mp_int*);

  /* find window size */
  x = mp_count_bits (X);
  if (x <= 7) {
    winsize = 2;
  } else if (x <= 36) {
    winsize = 3;
  } else if (x <= 140) {
    winsize = 4;
  } else if (x <= 450) {
    winsize = 5;
  } else if (x <= 1303) {
    winsize = 6;
  } else if (x <= 3529) {
    winsize = 7;
  } else {
    winsize = 8;
  }

#ifdef MP_LOW_MEM
    if (winsize > 5) {
       winsize = 5;
    }
#endif

  /* init M array */
  /* init first cell */
  if ((err = mp_init(&M[1])) != MP_OKAY) {
     return err; 
  }

  /* now init the second half of the array */
  for (x = 1<<(winsize-1); x < (1 << winsize); x++) {
    if ((err = mp_init(&M[x])) != MP_OKAY) {
      for (y = 1<<(winsize-1); y < x; y++) {
        mp_clear (&M[y]);
      }
      mp_clear(&M[1]);
      return err;
    }
  }

  /* create mu, used for Barrett reduction */
  if ((err = mp_init (&mu)) != MP_OKAY) {
    goto LBL_M;
  }
  
  if (redmode == 0) {
     if ((err = mp_reduce_setup (&mu, P)) != MP_OKAY) {
        goto LBL_MU;
     }
     redux = mp_reduce;
  } else {
     if ((err = mp_reduce_2k_setup_l (P, &mu)) != MP_OKAY) {
        goto LBL_MU;
     }
     redux = mp_reduce_2k_l;
  }    

  /* create M table
   *
   * The M table contains powers of the base, 
   * e.g. M[x] = G**x mod P
   *
   * The first half of the table is not 
   * computed though accept for M[0] and M[1]
   */
  if ((err = mp_mod (G, P, &M[1])) != MP_OKAY) {
    goto LBL_MU;
  }

  /* compute the value at M[1<<(winsize-1)] by squaring 
   * M[1] (winsize-1) times 
   */
  if ((err = mp_copy (&M[1], &M[1 << (winsize - 1)])) != MP_OKAY) {
    goto LBL_MU;
  }

  for (x = 0; x < (winsize - 1); x++) {
    /* square it */
    if ((err = mp_sqr (&M[1 << (winsize - 1)], 
                       &M[1 << (winsize - 1)])) != MP_OKAY) {
      goto LBL_MU;
    }

    /* reduce modulo P */
    if ((err = redux (&M[1 << (winsize - 1)], P, &mu)) != MP_OKAY) {
      goto LBL_MU;
    }
  }

  /* create upper table, that is M[x] = M[x-1] * M[1] (mod P)
   * for x = (2**(winsize - 1) + 1) to (2**winsize - 1)
   */
  for (x = (1 << (winsize - 1)) + 1; x < (1 << winsize); x++) {
    if ((err = mp_mul (&M[x - 1], &M[1], &M[x])) != MP_OKAY) {
      goto LBL_MU;
    }
    if ((err = redux (&M[x], P, &mu)) != MP_OKAY) {
      goto LBL_MU;
    }
  }

  /* setup result */
  if ((err = mp_init (&res)) != MP_OKAY) {
    goto LBL_MU;
  }
  mp_set (&res, 1);

  /* set initial mode and bit cnt */
  mode   = 0;
  bitcnt = 1;
  buf    = 0;
  digidx = X->used - 1;
  bitcpy = 0;
  bitbuf = 0;

  for (;;) {
    /* grab next digit as required */
    if (--bitcnt == 0) {
      /* if digidx == -1 we are out of digits */
      if (digidx == -1) {
        break;
      }
      /* read next digit and reset the bitcnt */
      buf    = X->dp[digidx--];
      bitcnt = (int) DIGIT_BIT;
    }

    /* grab the next msb from the exponent */
    y     = (buf >> (mp_digit)(DIGIT_BIT - 1)) & 1;
    buf <<= (mp_digit)1;

    /* if the bit is zero and mode == 0 then we ignore it
     * These represent the leading zero bits before the first 1 bit
     * in the exponent.  Technically this opt is not required but it
     * does lower the # of trivial squaring/reductions used
     */
    if (mode == 0 && y == 0) {
      continue;
    }

    /* if the bit is zero and mode == 1 then we square */
    if (mode == 1 && y == 0) {
      if ((err = mp_sqr (&res, &res)) != MP_OKAY) {
        goto LBL_RES;
      }
      if ((err = redux (&res, P, &mu)) != MP_OKAY) {
        goto LBL_RES;
      }
      continue;
    }

    /* else we add it to the window */
    bitbuf |= (y << (winsize - ++bitcpy));
    mode    = 2;

    if (bitcpy == winsize) {
      /* ok window is filled so square as required and multiply  */
      /* square first */
      for (x = 0; x < winsize; x++) {
        if ((err = mp_sqr (&res, &res)) != MP_OKAY) {
          goto LBL_RES;
        }
        if ((err = redux (&res, P, &mu)) != MP_OKAY) {
          goto LBL_RES;
        }
      }

      /* then multiply */
      if ((err = mp_mul (&res, &M[bitbuf], &res)) != MP_OKAY) {
        goto LBL_RES;
      }
      if ((err = redux (&res, P, &mu)) != MP_OKAY) {
        goto LBL_RES;
      }

      /* empty window and reset */
      bitcpy = 0;
      bitbuf = 0;
      mode   = 1;
    }
  }

  /* if bits remain then square/multiply */
  if (mode == 2 && bitcpy > 0) {
    /* square then multiply if the bit is set */
    for (x = 0; x < bitcpy; x++) {
      if ((err = mp_sqr (&res, &res)) != MP_OKAY) {
        goto LBL_RES;
      }
      if ((err = redux (&res, P, &mu)) != MP_OKAY) {
        goto LBL_RES;
      }

      bitbuf <<= 1;
      if ((bitbuf & (1 << winsize)) != 0) {
        /* then multiply */
        if ((err = mp_mul (&res, &M[1], &res)) != MP_OKAY) {
          goto LBL_RES;
        }
        if ((err = redux (&res, P, &mu)) != MP_OKAY) {
          goto LBL_RES;
        }
      }
    }
  }

  mp_exch (&res, Y);
  err = MP_OKAY;
LBL_RES:mp_clear (&res);
LBL_MU:mp_clear (&mu);
LBL_M:
  mp_clear(&M[1]);
  for (x = 1<<(winsize-1); x < (1 << winsize); x++) {
    mp_clear (&M[x]);
  }
  return err;
}


/* pre-calculate the value required for Barrett reduction
 * For a given modulus "b" it calulates the value required in "a"
 */
int mp_reduce_setup (mp_int * a, mp_int * b)
{
  int     res;
  
  if ((res = mp_2expt (a, b->used * 2 * DIGIT_BIT)) != MP_OKAY) {
    return res;
  }
  return mp_div (a, b, a, NULL);
}


/* reduces x mod m, assumes 0 < x < m**2, mu is 
 * precomputed via mp_reduce_setup.
 * From HAC pp.604 Algorithm 14.42
 */
int mp_reduce (mp_int * x, mp_int * m, mp_int * mu)
{
  mp_int  q;
  int     res, um = m->used;

  /* q = x */
  if ((res = mp_init_copy (&q, x)) != MP_OKAY) {
    return res;
  }

  /* q1 = x / b**(k-1)  */
  mp_rshd (&q, um - 1);         

  /* according to HAC this optimization is ok */
  if (((unsigned long) um) > (((mp_digit)1) << (DIGIT_BIT - 1))) {
    if ((res = mp_mul (&q, mu, &q)) != MP_OKAY) {
      goto CLEANUP;
    }
  } else {
#ifdef BN_S_MP_MUL_HIGH_DIGS_C
    if ((res = s_mp_mul_high_digs (&q, mu, &q, um)) != MP_OKAY) {
      goto CLEANUP;
    }
#elif defined(BN_FAST_S_MP_MUL_HIGH_DIGS_C)
    if ((res = fast_s_mp_mul_high_digs (&q, mu, &q, um)) != MP_OKAY) {
      goto CLEANUP;
    }
#else 
    { 
      res = MP_VAL;
      goto CLEANUP;
    }
#endif
  }

  /* q3 = q2 / b**(k+1) */
  mp_rshd (&q, um + 1);         

  /* x = x mod b**(k+1), quick (no division) */
  if ((res = mp_mod_2d (x, DIGIT_BIT * (um + 1), x)) != MP_OKAY) {
    goto CLEANUP;
  }

  /* q = q * m mod b**(k+1), quick (no division) */
  if ((res = s_mp_mul_digs (&q, m, &q, um + 1)) != MP_OKAY) {
    goto CLEANUP;
  }

  /* x = x - q */
  if ((res = mp_sub (x, &q, x)) != MP_OKAY) {
    goto CLEANUP;
  }

  /* If x < 0, add b**(k+1) to it */
  if (mp_cmp_d (x, 0) == MP_LT) {
    mp_set (&q, 1);
    if ((res = mp_lshd (&q, um + 1)) != MP_OKAY)
      goto CLEANUP;
    if ((res = mp_add (x, &q, x)) != MP_OKAY)
      goto CLEANUP;
  }

  /* Back off if it's too big */
  while (mp_cmp (x, m) != MP_LT) {
    if ((res = s_mp_sub (x, m, x)) != MP_OKAY) {
      goto CLEANUP;
    }
  }
  
CLEANUP:
  mp_clear (&q);

  return res;
}


/* reduces a modulo n where n is of the form 2**p - d 
   This differs from reduce_2k since "d" can be larger
   than a single digit.
*/
int mp_reduce_2k_l(mp_int *a, mp_int *n, mp_int *d)
{
   mp_int q;
   int    p, res;
   
   if ((res = mp_init(&q)) != MP_OKAY) {
      return res;
   }
   
   p = mp_count_bits(n);    
top:
   /* q = a/2**p, a = a mod 2**p */
   if ((res = mp_div_2d(a, p, &q, a)) != MP_OKAY) {
      goto ERR;
   }
   
   /* q = q * d */
   if ((res = mp_mul(&q, d, &q)) != MP_OKAY) { 
      goto ERR;
   }
   
   /* a = a + q */
   if ((res = s_mp_add(a, &q, a)) != MP_OKAY) {
      goto ERR;
   }
   
   if (mp_cmp_mag(a, n) != MP_LT) {
      s_mp_sub(a, n, a);
      goto top;
   }
   
ERR:
   mp_clear(&q);
   return res;
}


/* determines the setup value */
int mp_reduce_2k_setup_l(mp_int *a, mp_int *d)
{
   int    res;
   mp_int tmp;
   
   if ((res = mp_init(&tmp)) != MP_OKAY) {
      return res;
   }
   
   if ((res = mp_2expt(&tmp, mp_count_bits(a))) != MP_OKAY) {
      goto ERR;
   }
   
   if ((res = s_mp_sub(&tmp, a, d)) != MP_OKAY) {
      goto ERR;
   }
   
ERR:
   mp_clear(&tmp);
   return res;
}


/* multiplies |a| * |b| and does not compute the lower digs digits
 * [meant to get the higher part of the product]
 */
int
s_mp_mul_high_digs (mp_int * a, mp_int * b, mp_int * c, int digs)
{
  mp_int  t;
  int     res, pa, pb, ix, iy;
  mp_digit u;
  mp_word r;
  mp_digit tmpx, *tmpt, *tmpy;

  /* can we use the fast multiplier? */
#ifdef BN_FAST_S_MP_MUL_HIGH_DIGS_C
  if (((a->used + b->used + 1) < MP_WARRAY)
      && MIN (a->used, b->used) < (1 << ((CHAR_BIT * sizeof (mp_word)) - (2 * DIGIT_BIT)))) {
    return fast_s_mp_mul_high_digs (a, b, c, digs);
  }
#endif

  if ((res = mp_init_size (&t, a->used + b->used + 1)) != MP_OKAY) {
    return res;
  }
  t.used = a->used + b->used + 1;

  pa = a->used;
  pb = b->used;
  for (ix = 0; ix < pa; ix++) {
    /* clear the carry */
    u = 0;

    /* left hand side of A[ix] * B[iy] */
    tmpx = a->dp[ix];

    /* alias to the address of where the digits will be stored */
    tmpt = &(t.dp[digs]);

    /* alias for where to read the right hand side from */
    tmpy = b->dp + (digs - ix);

    for (iy = digs - ix; iy < pb; iy++) {
      /* calculate the double precision result */
      r       = ((mp_word)*tmpt) +
                ((mp_word)tmpx) * ((mp_word)*tmpy++) +
                ((mp_word) u);

      /* get the lower part */
      *tmpt++ = (mp_digit) (r & ((mp_word) MP_MASK));

      /* carry the carry */
      u       = (mp_digit) (r >> ((mp_word) DIGIT_BIT));
    }
    *tmpt = u;
  }
  mp_clamp (&t);
  mp_exch (&t, c);
  mp_clear (&t);
  return MP_OKAY;
}


/* this is a modified version of fast_s_mul_digs that only produces
 * output digits *above* digs.  See the comments for fast_s_mul_digs
 * to see how it works.
 *
 * This is used in the Barrett reduction since for one of the multiplications
 * only the higher digits were needed.  This essentially halves the work.
 *
 * Based on Algorithm 14.12 on pp.595 of HAC.
 */
int fast_s_mp_mul_high_digs (mp_int * a, mp_int * b, mp_int * c, int digs)
{
  int     olduse, res, pa, ix, iz;
  mp_digit W[MP_WARRAY];
  mp_word  _W;

  /* grow the destination as required */
  pa = a->used + b->used;
  if (c->alloc < pa) {
    if ((res = mp_grow (c, pa)) != MP_OKAY) {
      return res;
    }
  }

  /* number of output digits to produce */
  pa = a->used + b->used;
  _W = 0;
  for (ix = digs; ix < pa; ix++) { 
      int      tx, ty, iy;
      mp_digit *tmpx, *tmpy;

      /* get offsets into the two bignums */
      ty = MIN(b->used-1, ix);
      tx = ix - ty;

      /* setup temp aliases */
      tmpx = a->dp + tx;
      tmpy = b->dp + ty;

      /* this is the number of times the loop will iterrate, essentially its 
         while (tx++ < a->used && ty-- >= 0) { ... }
       */
      iy = MIN(a->used-tx, ty+1);

      /* execute loop */
      for (iz = 0; iz < iy; iz++) {
         _W += ((mp_word)*tmpx++)*((mp_word)*tmpy--);
      }

      /* store term */
      W[ix] = ((mp_digit)_W) & MP_MASK;

      /* make next carry */
      _W = _W >> ((mp_word)DIGIT_BIT);
  }
  
  /* setup dest */
  olduse  = c->used;
  c->used = pa;

  {
    register mp_digit *tmpc;

    tmpc = c->dp + digs;
    for (ix = digs; ix <= pa; ix++) {
      /* now extract the previous digit [below the carry] */
      *tmpc++ = W[ix];
    }

    /* clear unused digits [that existed in the old copy of c] */
    for (; ix < olduse; ix++) {
      *tmpc++ = 0;
    }
  }
  mp_clamp (c);
  return MP_OKAY;
}



#endif /* USE_FAST_MATH */

