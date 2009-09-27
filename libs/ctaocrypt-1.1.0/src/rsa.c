/* rsa.c
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



#include "rsa.h"
#include "random.h"
#include "error.h"
#include <assert.h>


enum {
    RSA_PUBLIC_ENCRYPT  = 0,
    RSA_PUBLIC_DECRYPT  = 1,
    RSA_PRIVATE_ENCRYPT = 2,
    RSA_PRIVATE_DECRYPT = 3,

    RSA_BLOCK_TYPE_1 = 1,
    RSA_BLOCK_TYPE_2 = 2,

    RSA_MIN_PAD_SZ   = 11      /* seperator + 0 + pad value + 8 pads */
};


void InitRsaKey(RsaKey* key, void* heap)
{
    key->type = -1;  /* haven't decdied yet */
    key->heap = heap;

/* TomsFastMath doesn't use memory allocation */
#ifndef USE_FAST_MATH
    key->n.dp = key->e.dp = 0;  /* public  alloc parts */

    key->d.dp = key->p.dp  = 0;  /* private alloc parts */
    key->q.dp = key->dP.dp = 0;  
    key->u.dp = key->dQ.dp = 0;
#endif
}


void FreeRsaKey(RsaKey* key)
{
/* TomsFastMath doesn't use memory allocation */
#ifndef USE_FAST_MATH
    if (key->type == RSA_PRIVATE) {
        mp_clear(&key->u);
        mp_clear(&key->dQ);
        mp_clear(&key->dP);
        mp_clear(&key->q);
        mp_clear(&key->p);
        mp_clear(&key->d);
    }
    mp_clear(&key->e);
    mp_clear(&key->n);
#endif
}

static void RsaPad(const byte* input, word32 inputLen, byte* pkcsBlock,
                   word32 pkcsBlockLen, byte padValue, RNG* rng)
{
    pkcsBlock[0] = 0x0;       /* set first byte to zero and advance */
    pkcsBlock++; pkcsBlockLen--;
    pkcsBlock[0] = padValue;  /* insert padValue */

    assert(padValue == RSA_BLOCK_TYPE_1 || padValue == RSA_BLOCK_TYPE_2);

    if (padValue == RSA_BLOCK_TYPE_1)
        /* pad with 0xff bytes */
        memset(&pkcsBlock[1], 0xFF, pkcsBlockLen - inputLen - 2);
    else {
        /* pad with non-zero random bytes */
        word32 padLen = pkcsBlockLen - inputLen - 1, i;
        RNG_GenerateBlock(rng, &pkcsBlock[1], padLen);

        /* remove zeros */
        for (i = 1; i < padLen; i++)
            if (pkcsBlock[i] == 0) pkcsBlock[i] = 0x01;
    }

    pkcsBlock[pkcsBlockLen-inputLen-1] = 0;     /* separator */
    memcpy(pkcsBlock+pkcsBlockLen-inputLen, input, inputLen);
}


static word32 RsaUnPad(const byte *pkcsBlock, unsigned int pkcsBlockLen,
                       byte **output, byte padValue)
{
    word32 maxOutputLen = (pkcsBlockLen > 10) ? (pkcsBlockLen - 10) : 0,
           invalid = 0,
           i = 1,
           outputLen;

    if (pkcsBlock[0] != 0x0) /* skip past zero */
        invalid = 1;
    pkcsBlock++; pkcsBlockLen--;

    /* Require block type padValue */
    invalid = (pkcsBlock[0] != padValue) || invalid;

    /* skip past the padding until we find the separator */
    while (i<pkcsBlockLen && pkcsBlock[i++]) { /* null body */
        }
    assert(i==pkcsBlockLen || pkcsBlock[i-1]==0);

    outputLen = pkcsBlockLen - i;
    invalid = (outputLen > maxOutputLen) || invalid;

    if (invalid)
        return 0;

    *output = (byte *)(pkcsBlock + i);
    return outputLen;
}


static int RsaFunction(const byte* in, word32 inLen, byte* out, word32* outLen,
                       int type, RsaKey* key)
{
    #define ERROR_OUT(x) { ret = x; goto done;}

    mp_int tmp;
    int    ret = 0;
    word32 keyLen, len;

    if (mp_init(&tmp) != MP_OKAY)
        return MP_INIT_E;

    if (mp_read_unsigned_bin(&tmp, (byte*)in, inLen) != MP_OKAY)
        ERROR_OUT(MP_READ_E);

    if (type == RSA_PRIVATE_DECRYPT || type == RSA_PRIVATE_ENCRYPT) {
        #ifdef RSA_LOW_MEM      /* half as much memory but twice as slow */
            if (mp_exptmod(&tmp, &key->d, &key->n, &tmp) != MP_OKAY)
                ERROR_OUT(MP_EXPTMOD_E);
        #else
            #define INNER_ERROR_OUT(x) { ret = x; goto inner_done; }

            mp_int tmpa, tmpb;

            if (mp_init(&tmpa) != MP_OKAY)
                ERROR_OUT(MP_INIT_E);

            if (mp_init(&tmpb) != MP_OKAY) {
                mp_clear(&tmpa);
                ERROR_OUT(MP_INIT_E);
            }

            /* tmpa = tmp^dP mod p */
            if (mp_exptmod(&tmp, &key->dP, &key->p, &tmpa) != MP_OKAY)
                INNER_ERROR_OUT(MP_EXPTMOD_E);

            /* tmpb = tmp^dQ mod q */
            if (mp_exptmod(&tmp, &key->dQ, &key->q, &tmpb) != MP_OKAY)
                INNER_ERROR_OUT(MP_EXPTMOD_E);

            /* tmp = (tmpa - tmpb) * qInv (mod p) */
            if (mp_sub(&tmpa, &tmpb, &tmp) != MP_OKAY)
                INNER_ERROR_OUT(MP_SUB_E);

            if (mp_mulmod(&tmp, &key->u, &key->p, &tmp) != MP_OKAY)
                INNER_ERROR_OUT(MP_MULMOD_E);

            /* tmp = tmpb + q * tmp */
            if (mp_mul(&tmp, &key->q, &tmp) != MP_OKAY)
                INNER_ERROR_OUT(MP_MUL_E);

            if (mp_add(&tmp, &tmpb, &tmp) != MP_OKAY)
                INNER_ERROR_OUT(MP_ADD_E);

        inner_done:
            mp_clear(&tmpa);
            mp_clear(&tmpb);

            if (ret != 0) return ret;

        #endif   /* RSA_LOW_MEM */
    }
    else if (type == RSA_PUBLIC_ENCRYPT || type == RSA_PUBLIC_DECRYPT) {
        if (mp_exptmod(&tmp, &key->e, &key->n, &tmp) != MP_OKAY)
            ERROR_OUT(MP_EXPTMOD_E);
    }
    else
        ERROR_OUT(RSA_WRONG_TYPE_E);

    keyLen = mp_unsigned_bin_size(&key->n);
    if (keyLen > *outLen)
        ERROR_OUT(RSA_BUFFER_E);

    len = mp_unsigned_bin_size(&tmp);

    /* pad front w/ zeros to match key length */
    while (len < keyLen) {
        *out++ = 0x00;
        len++;
    }

    *outLen = keyLen;

    /* convert */
    if (mp_to_unsigned_bin(&tmp, out) != MP_OKAY)
        ERROR_OUT(MP_TO_E);
   
done: 
    mp_clear(&tmp);
    return ret;
}



int RsaPublicEncrypt(const byte* in, word32 inLen, byte* out, word32 outLen,
                     RsaKey* key, RNG* rng)
{
    int sz = mp_unsigned_bin_size(&key->n), ret;

    if (sz > (int)outLen)
        return RSA_BUFFER_E;

    if (inLen > (word32)(sz - RSA_MIN_PAD_SZ))
        return RSA_BUFFER_E;

    RsaPad(in, inLen, out, sz, RSA_BLOCK_TYPE_2, rng);

    if ((ret = RsaFunction(out, sz, out, &outLen, RSA_PUBLIC_ENCRYPT, key)) < 0)
        sz = ret;

    return sz;
}


int RsaPrivateDecryptInline(byte* in, word32 inLen, byte** out, RsaKey* key)
{
    int plainLen, ret;

    if ((ret = RsaFunction(in, inLen, in, &inLen, RSA_PRIVATE_DECRYPT, key))
            < 0) {
        return ret;
    }
 
    plainLen = RsaUnPad(in, inLen, out, RSA_BLOCK_TYPE_2);

    return plainLen;
}

int RsaPrivateDecrypt(const byte* in, word32 inLen, byte* out, word32 outLen,
                     RsaKey* key)
{
    int plainLen, ret;
    byte*  tmp;
    byte*  pad = 0;

    if ( !(tmp = (byte*)XMALLOC(inLen, key->heap)) )
        return MEMORY_E;

    memcpy(tmp, in, inLen);

    if ((ret = plainLen = RsaPrivateDecryptInline(tmp, inLen, &pad, key))
            < 0) {
        XFREE(tmp, key->heap);
        return ret;
    }
    memcpy(out, pad, plainLen);
    memset(tmp, 0x00, inLen); 

    XFREE(tmp, key->heap);
    return plainLen;
}


/* for Rsa Verify */
int RsaSSL_VerifyInline(byte* in, word32 inLen, byte** out, RsaKey* key)
{
    int plainLen, ret;

    if ((ret = RsaFunction(in, inLen, in, &inLen, RSA_PUBLIC_DECRYPT, key))
            < 0) {
        return ret;
    }
  
    plainLen = RsaUnPad(in, inLen, out, RSA_BLOCK_TYPE_1);

    return plainLen;
}

int RsaSSL_Verify(const byte* in, word32 inLen, byte* out, word32 outLen,
                     RsaKey* key)
{
    int plainLen, ret;
    byte*  tmp;
    byte*  pad = 0;

    if ( !(tmp = (byte*)XMALLOC(inLen, key->heap)) )
        return MEMORY_E;

    memcpy(tmp, in, inLen);

    if ((ret = plainLen = RsaSSL_VerifyInline(tmp, inLen, &pad, key))
            < 0) {
        XFREE(tmp, key->heap);
        return ret;
    }
  
    memcpy(out, pad, plainLen);
    memset(tmp, 0x00, inLen); 

    XFREE(tmp, key->heap);
    return plainLen;
}


/* for Rsa Sign */
int RsaSSL_Sign(const byte* in, word32 inLen, byte* out, word32 outLen,
                      RsaKey* key, RNG* rng)
{
    int sz = mp_unsigned_bin_size(&key->n), ret;

    if (sz > (int)outLen)
        return RSA_BUFFER_E;

    if (inLen > (word32)(sz - RSA_MIN_PAD_SZ))
        return RSA_BUFFER_E;

    RsaPad(in, inLen, out, sz, RSA_BLOCK_TYPE_1, rng);

    if ((ret = RsaFunction(out, sz, out, &outLen, RSA_PRIVATE_ENCRYPT,key)) < 0)
        sz = ret;
    
    return sz;
}


int RsaEncryptSize(RsaKey* key)
{
    return mp_unsigned_bin_size(&key->n);
}

