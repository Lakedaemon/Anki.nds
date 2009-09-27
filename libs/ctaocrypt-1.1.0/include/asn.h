/* asn.h
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


#ifndef CTAO_CRYPT_ASN_H
#define CTAO_CRYPT_ASN_H

#include "types.h"
#include "rsa.h"
#include "dh.h"
#include "dsa.h"
#include "sha.h"


#ifdef __cplusplus
    extern "C" {
#endif

/* ASN Tags   */
enum ASN_Tags {        
    ASN_INTEGER           = 0x02,
    ASN_BIT_STRING        = 0x03,
    ASN_OCTET_STRING      = 0x04,
    ASN_TAG_NULL          = 0x05,
    ASN_OBJECT_ID         = 0x06,
    ASN_SEQUENCE          = 0x10,
    ASN_SET               = 0x11,
    ASN_UTC_TIME          = 0x17,
    ASN_GENERALIZED_TIME  = 0x18,
    ASN_LONG_LENGTH       = 0x80
};


enum  ASN_Flags{
    ASN_CONSTRUCTED       = 0x20,
    ASN_CONTEXT_SPECIFIC  = 0x80
};

enum DN_Tags {
    ASN_COMMON_NAME   = 0x03,   /* CN */
    ASN_SUR_NAME      = 0x04,   /* SN */
    ASN_COUNTRY_NAME  = 0x06,   /* C  */
    ASN_LOCALITY_NAME = 0x07,   /* L  */
    ASN_STATE_NAME    = 0x08,   /* ST */
    ASN_ORG_NAME      = 0x0a,   /* O  */
    ASN_ORGUNIT_NAME  = 0x0b    /* OU */
};

enum Misc_ASN { 
    ASN_NAME_MAX        = 256,    
    SHA_SIZE            =  20,
    MIN_DATE_SIZE       =  13,
    MAX_DATE_SIZE       =  15,
    MAX_ENCODED_SIG_SZ  = 512,
    MAX_SIG_SZ          = 256,
    MAX_ALGO_SZ         =  16,
    MAX_SEQ_SZ          =   5,     /* enum(seq | con) + length(4) */  
    MAX_ENCODED_DIG_SZ  =  25,     /* sha + enum(bit or octet) + legnth(4) */
    MAX_LENGTH_SZ       =   4 
};

enum Sig_Sum  {
    SHAwDSA = 517,
    MD2wRSA = 646,
    MD5wRSA = 648,
    SHAwRSA = 649
};

enum Hash_Sum  {
    MD2h  = 646,
    MD5h  = 649,
    SHAh  =  88
};

enum Key_Sum {
    DSAk = 515,
    RSAk = 645
};


/* Certificate file Type */
enum CertType {
    CERT_TYPE       = 0, 
    PRIVATEKEY_TYPE,
    CA_TYPE
};


enum VerifyType {
    NO_VERIFY = 0,
    VERIFY    = 1
};


typedef struct DecodedCert {
    byte*   publicKey;
    word32  pubKeySize;
    int     pubKeyStored;
    word32  certBegin;               /* offset to start of cert          */
    word32  sigIndex;                /* offset to start of signature     */
    word32  sigLength;               /* length of signature              */
    word32  signatureOID;            /* sum of algorithm object id       */
    word32  keyOID;                  /* sum of key algo  object id       */
    byte    subjectHash[SHA_SIZE];   /* hash of all Names                */
    byte    issuerHash[SHA_SIZE];    /* hash of all Names                */
    byte*   signature;
    int     signatureStored;
    char*   issuerCN;                /* CommonName                       */
    int     issuerCNLen;
    char*   subjectCN;               /* CommonName                       */
    int     subjectCNLen;
    char    issuer[ASN_NAME_MAX];    /* full name including common name  */
    char    subject[ASN_NAME_MAX];   /* full name including common name  */
    int     verify;                  /* Default to yes, but could be off */
    byte*   source;                  /* byte buffer holder cert, NOT owner */
    word32  srcIdx;                  /* current offset into buffer       */
    void*   heap;                    /* for user memory overrides        */
} DecodedCert;


typedef struct Signer Signer;

/* CA Signers */
struct Signer {
    byte*   publicKey;
    word32  pubKeySize;
    char*   name;                    /* common name */
    byte    hash[SHA_DIGEST_SIZE];   /* sha hash of names in certificate */
    Signer* next;
};


void InitDecodedCert(DecodedCert*, byte*, void*);
void FreeDecodedCert(DecodedCert*);
int  ParseCert(DecodedCert*, word32, int type, int verify, Signer* signer);
int  ParseCertRelative(DecodedCert*, word32, int type, int verify,
                       Signer* signer);

word32 EncodeSignature(byte* out, const byte* digest, word32 digSz,int hashOID);

Signer* MakeSigner(void*);
void    FreeSigners(Signer*, void*);


int RsaPrivateKeyDecode(const byte* input, word32* inOutIdx, RsaKey*, word32);
int RsaPublicKeyDecode(const byte* input, word32* inOutIdx, RsaKey*, word32);

#ifndef NO_DH
int DhKeyDecode(const byte* input, word32* inOutIdx, DhKey* key, word32);
int DhSetKey(DhKey* key, const byte* p, word32 pSz, const byte* g, word32 gSz);
#endif

#ifndef NO_DSA
int DsaPublicKeyDecode(const byte* input, word32* inOutIdx, DsaKey*, word32);
int DsaPrivateKeyDecode(const byte* input, word32* inOutIdx, DsaKey*, word32);
#endif


#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* CTAO_CRYPT_ASN_H */

