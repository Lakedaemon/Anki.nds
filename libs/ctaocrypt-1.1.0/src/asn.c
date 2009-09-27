/* asn.c
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


#include "asn.h"
#include "sha.h"
#include "md5.h"
#include "error.h"
#include <time.h> 
#include <assert.h>

#ifdef _MSC_VER
    /* 4996 warning to use MS extensions e.g., strcpy_s instead of strncpy */
    #pragma warning(disable: 4996)
#endif


enum {
    FALSE = 0,
    TRUE  = 1
};


enum {
    ISSUER  = 0,
    SUBJECT = 1,

    BEFORE  = 0,
    AFTER   = 1
};


#ifdef _WIN32_WCE
/* no time() or gmtime() even though in time.h header?? */

#include <windows.h>

#define YEAR0          1900
#define EPOCH_YEAR     1970
#define SECS_DAY       (24L * 60L * 60L)
#define LEAPYEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year) (LEAPYEAR(year) ? 366 : 365)


const int _ytab[2][12] =
{
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


time_t time(time_t* timer)
{
    SYSTEMTIME     sysTime;
    FILETIME       fTime;
    ULARGE_INTEGER intTime;
    time_t         localTime;

    if (timer == NULL)
        timer = &localTime;

    GetSystemTime(&sysTime);
    SystemTimeToFileTime(&sysTime, &fTime);
    
    memcpy(&intTime, &fTime, sizeof(FILETIME));
    /* subtract EPOCH */
    intTime.QuadPart -= 0x19db1ded53e8000;
    /* to secs */
    intTime.QuadPart /= 10000000;
    *timer = (time_t)intTime.QuadPart;

    return *timer;
}


struct tm* gmtime(const time_t* timer)
{
    static struct tm st_time;
    struct tm* ret = &st_time;
    time_t time = *timer;
    unsigned long dayclock, dayno;
    int year = EPOCH_YEAR;

    dayclock = (unsigned long)time % SECS_DAY;
    dayno    = (unsigned long)time / SECS_DAY;

    ret->tm_sec  =  dayclock % 60;
    ret->tm_min  = (dayclock % 3600) / 60;
    ret->tm_hour =  dayclock / 3600;
    ret->tm_wday = (dayno + 4) % 7;        /* day 0 a Thursday */

    while(dayno >= (unsigned long)YEARSIZE(year)) {
        dayno -= YEARSIZE(year);
        year++;
    }

    ret->tm_year = year - YEAR0;
    ret->tm_yday = dayno;
    ret->tm_mon  = 0;

    while(dayno >= (unsigned long)_ytab[LEAPYEAR(year)][ret->tm_mon]) {
        dayno -= _ytab[LEAPYEAR(year)][ret->tm_mon];
        ret->tm_mon++;
    }

    ret->tm_mday  = ++dayno;
    ret->tm_isdst = 0;

    return ret;
}

#endif /* _WIN32_WCE */


int GetLength(const byte* input, word32* inOutIdx, int* len)
{
    int     length = 0;
    word32  i = *inOutIdx;

    byte b = input[i++];
    if (b >= ASN_LONG_LENGTH) {        
        word32 bytes = b & 0x7F;

        while (bytes--) {
            b = input[i++];
            length = (length << 8) | b;
        }
    }
    else
        length = b;
    
    *inOutIdx = i;
    *len      = length;

    return length;
}


int GetSequence(const byte* input, word32* inOutIdx, int* len)
{
    int    length = -1;
    word32 idx    = *inOutIdx;

    if (input[idx++] != (ASN_SEQUENCE | ASN_CONSTRUCTED) ||
            GetLength(input, &idx, &length) < 0)
        return ASN_PARSE_E;

    *len      = length;
    *inOutIdx = idx;

    return length;
}


int GetSet(const byte* input, word32* inOutIdx, int* len)
{
    int    length = -1;
    word32 idx    = *inOutIdx;

    if (input[idx++] != (ASN_SET | ASN_CONSTRUCTED) ||
            GetLength(input, &idx, &length) < 0)
        return ASN_PARSE_E;

    *len      = length;
    *inOutIdx = idx;

    return length;
}


/* winodws header clash for WinCE using GetVersion */
int GetMyVersion(const byte* input, word32* inOutIdx, int* version)
{
    word32 idx = *inOutIdx;

    if (input[idx++] != ASN_INTEGER)
        return ASN_PARSE_E;

    if (input[idx++] != 0x01)
        return ASN_VERSION_E;

    *version  = input[idx++];
    *inOutIdx = idx;

    return *version;
}


/* May not have one, not an error */
int GetExplicitVersion(const byte* input, word32* inOutIdx, int* version)
{
    word32 idx = *inOutIdx;

    if (input[idx++] == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED)) {
        *inOutIdx = ++idx;  /* eat header */
        return GetMyVersion(input, inOutIdx, version);
    }

    /* go back as is */
    *version = 0;

    return 0;
}


int GetInt(mp_int* mpi, const byte* input, word32* inOutIdx )
{
    word32 i = *inOutIdx;
    byte   b = input[i++];
    int    length;

    if (b != ASN_INTEGER)
        return ASN_PARSE_E;

    if (GetLength(input, &i, &length) < 0)
        return ASN_PARSE_E;

    if ( (b = input[i++]) == 0x00)
        length--;
    else
        i--;

    mp_init(mpi); 
    if (mp_read_unsigned_bin(mpi, (byte*)input + i, length) != 0) {
        mp_clear(mpi);
        return ASN_GETINT_E;
    }

    *inOutIdx = i + length;
    return 0;
}


int RsaPrivateKeyDecode(const byte* input, word32* inOutIdx, RsaKey* key,
                        word32 inSz)
{
    word32 begin = *inOutIdx;
    int    version, length;

    if (GetSequence(input, inOutIdx, &length) < 0)
        return ASN_PARSE_E;

    if ((word32)length > (inSz - (*inOutIdx - begin)))
        return ASN_INPUT_E;

    if (GetMyVersion(input, inOutIdx, &version) < 0)
        return ASN_PARSE_E;

    key->type = RSA_PRIVATE;

    if (GetInt(&key->n,  input, inOutIdx) < 0 ||
        GetInt(&key->e,  input, inOutIdx) < 0 ||
        GetInt(&key->d,  input, inOutIdx) < 0 ||
        GetInt(&key->p,  input, inOutIdx) < 0 ||
        GetInt(&key->q,  input, inOutIdx) < 0 ||
        GetInt(&key->dP, input, inOutIdx) < 0 ||
        GetInt(&key->dQ, input, inOutIdx) < 0 ||
        GetInt(&key->u,  input, inOutIdx) < 0 )  return ASN_RSA_KEY_E;

    return 0;
}


int RsaPublicKeyDecode(const byte* input, word32* inOutIdx, RsaKey* key,
                       word32 inSz)
{
    word32 begin = *inOutIdx;
    int    length;
    byte   b;

    if (GetSequence(input, inOutIdx, &length) < 0)
        return ASN_PARSE_E;

    if ((word32)length > (inSz - (*inOutIdx - begin)))
        return ASN_INPUT_E;

    key->type = RSA_PUBLIC;
    b = input[*inOutIdx];

#ifdef OPENSSL_EXTRA    
    if (b != ASN_INTEGER) {
        /* not from decoded cert, will have algo id, skip past */
        if (GetSequence(input, inOutIdx, &length) < 0)
            return ASN_PARSE_E;
        
        b = input[(*inOutIdx)++];
        if (b != ASN_OBJECT_ID) 
            return ASN_OBJECT_ID_E;
        
        if (GetLength(input, inOutIdx, &length) < 0)
            return ASN_PARSE_E;
        
        *inOutIdx += length;   /* skip past */
        
        /* could have NULL tag and 0 terminator, but may not */
        b = input[(*inOutIdx)++];
        
        if (b == ASN_TAG_NULL) {
            b = input[(*inOutIdx)++];
            if (b != 0) 
                return ASN_EXPECT_0_E;
        }
        else
        /* go back, didn't have it */
            (*inOutIdx)--;
        
        /* should have bit tag length and seq next */
        b = input[(*inOutIdx)++];
        if (b != ASN_BIT_STRING)
            return ASN_BITSTR_E;
        
        if (GetLength(input, inOutIdx, &length) < 0)
            return ASN_PARSE_E;
        
        /* could have 0 */
        b = input[(*inOutIdx)++];
        if (b != 0)
            (*inOutIdx)--;
        
        if (GetSequence(input, inOutIdx, &length) < 0)
            return ASN_PARSE_E;
    }
#endif /* OPENSSL_EXTRA */

    if (GetInt(&key->n,  input, inOutIdx) < 0 ||
        GetInt(&key->e,  input, inOutIdx) < 0 )  return ASN_RSA_KEY_E;

    return 0;
}


#ifndef NO_DH

int DhKeyDecode(const byte* input, word32* inOutIdx, DhKey* key, word32 inSz)
{
    word32 begin = *inOutIdx;
    int    length;

    if (GetSequence(input, inOutIdx, &length) < 0)
        return ASN_PARSE_E;

    if ((word32)length > (inSz - (*inOutIdx - begin)))
        return ASN_INPUT_E;

    if (GetInt(&key->p,  input, inOutIdx) < 0 ||
        GetInt(&key->g,  input, inOutIdx) < 0 )  return ASN_DH_KEY_E;

    return 0;
}

int DhSetKey(DhKey* key, const byte* p, word32 pSz, const byte* g, word32 gSz)
{
    /* may have leading 0 */
    if (p[0] == 0) {
        pSz--; p++;
    }

    if (g[0] == 0) {
        gSz--; g++;
    }

    mp_init(&key->p);
    if (mp_read_unsigned_bin(&key->p, p, pSz) != 0) {
        mp_clear(&key->p);
        return ASN_DH_KEY_E;
    }

    mp_init(&key->g);
    if (mp_read_unsigned_bin(&key->g, g, gSz) != 0) {
        mp_clear(&key->p);
        return ASN_DH_KEY_E;
    }

    return 0;
}


#endif /* NO_DH */


#ifndef NO_DSA

int DsaPublicKeyDecode(const byte* input, word32* inOutIdx, DsaKey* key,
                        word32 inSz)
{
    word32 begin = *inOutIdx;
    int    length;

    if (GetSequence(input, inOutIdx, &length) < 0)
        return ASN_PARSE_E;

    if ((word32)length > (inSz - (*inOutIdx - begin)))
        return ASN_INPUT_E;

    if (GetInt(&key->p,  input, inOutIdx) < 0 ||
        GetInt(&key->q,  input, inOutIdx) < 0 ||
        GetInt(&key->g,  input, inOutIdx) < 0 ||
        GetInt(&key->y,  input, inOutIdx) < 0 )  return ASN_DH_KEY_E;

    key->type = DSA_PUBLIC;
    return 0;
}


int DsaPrivateKeyDecode(const byte* input, word32* inOutIdx, DsaKey* key,
                        word32 inSz)
{
    word32 begin = *inOutIdx;
    int    length, version;

    if (GetSequence(input, inOutIdx, &length) < 0)
        return ASN_PARSE_E;

    if ((word32)length > (inSz - (*inOutIdx - begin)))
        return ASN_INPUT_E;

    if (GetMyVersion(input, inOutIdx, &version) < 0)
        return ASN_PARSE_E;

    if (GetInt(&key->p,  input, inOutIdx) < 0 ||
        GetInt(&key->q,  input, inOutIdx) < 0 ||
        GetInt(&key->g,  input, inOutIdx) < 0 ||
        GetInt(&key->y,  input, inOutIdx) < 0 ||
        GetInt(&key->x,  input, inOutIdx) < 0 )  return ASN_DH_KEY_E;

    key->type = DSA_PRIVATE;
    return 0;
}

#endif /* NO_DSA */


void InitDecodedCert(DecodedCert* cert, byte* source, void* heap)
{
    cert->publicKey       = 0;
    cert->pubKeyStored    = 0;
    cert->signature       = 0;
    cert->signatureStored = 0;
    cert->issuerCN        = 0;
    cert->issuerCNLen     = 0;
    cert->subjectCN       = 0;
    cert->subjectCNLen    = 0;
    cert->source          = source;  /* don't own */
    cert->srcIdx          = 0;
    cert->heap            = heap;
}


void FreeDecodedCert(DecodedCert* cert)
{
    if (cert->subjectCNLen == 0)
        XFREE(cert->subjectCN, cert->heap);
    if (cert->issuerCNLen == 0)
        XFREE(cert->issuerCN, cert->heap);
    if (cert->signatureStored)
        XFREE(cert->signature, cert->heap);
    if (cert->pubKeyStored == 1)
        XFREE(cert->publicKey, cert->heap);
}


static int GetCertHeader(DecodedCert* cert, word32 inSz)
{
    int    ret = 0, version, len;
    word32 begin = cert->srcIdx;
    mp_int mpi;

    if (GetSequence(cert->source, &cert->srcIdx, &len) < 0)
        return ASN_PARSE_E;

    if ((word32)len > (inSz - (cert->srcIdx - begin))) return ASN_INPUT_E;

    cert->certBegin = cert->srcIdx;

    GetSequence(cert->source, &cert->srcIdx, &len);
    cert->sigIndex = len + cert->srcIdx;

    if (GetExplicitVersion(cert->source, &cert->srcIdx, &version) < 0)
        return ASN_PARSE_E;

    if (GetInt(&mpi, cert->source, &cert->srcIdx) < 0) 
        ret = ASN_PARSE_E;

    mp_clear(&mpi);
    return ret;
}


static int GetAlgoId(DecodedCert* cert, word32* oid)
{
    int    length;
    byte   b;
    *oid = 0;

    if (GetSequence(cert->source, &cert->srcIdx, &length) < 0)
        return ASN_PARSE_E;
    
    b = cert->source[cert->srcIdx++];
    if (b != ASN_OBJECT_ID) 
        return ASN_OBJECT_ID_E;

    if (GetLength(cert->source, &cert->srcIdx, &length) < 0)
        return ASN_PARSE_E;
    
    while(length--)
        *oid += cert->source[cert->srcIdx++];
        /* just sum it up for now */

    /* could have NULL tag and 0 terminator, but may not */
    b = cert->source[cert->srcIdx++];

    if (b == ASN_TAG_NULL) {
        b = cert->source[cert->srcIdx++];
        if (b != 0) 
            return ASN_EXPECT_0_E;
    }
    else
        /* go back, didn't have it */
        cert->srcIdx--;

    return 0;
}


static int StoreKey(DecodedCert* cert)
{
    int    length;
    word32 read = cert->srcIdx;

    if (GetSequence(cert->source, &cert->srcIdx, &length) < 0)
        return ASN_PARSE_E;
   
    read = cert->srcIdx - read;
    length += read;

    while (read--)
       cert->srcIdx--;

    cert->pubKeySize = length;
    cert->publicKey = cert->source + cert->srcIdx;
    cert->srcIdx += length;

    return 0;
}


static int GetKey(DecodedCert* cert)
{
    int length;

    if (GetSequence(cert->source, &cert->srcIdx, &length) < 0)
        return ASN_PARSE_E;
    
    if (GetAlgoId(cert, &cert->keyOID) < 0)
        return ASN_PARSE_E;

    if (cert->keyOID == RSAk) {
        byte b = cert->source[cert->srcIdx++];
        if (b != ASN_BIT_STRING)
            return ASN_BITSTR_E;

        b = cert->source[cert->srcIdx++];  /* length, future */
        b = cert->source[cert->srcIdx++];
        
        while (b != 0)
            b = cert->source[cert->srcIdx++];
    }
    else if (cert->keyOID == DSAk )
        ;   /* do nothing */
    else
        return ASN_UNKNOWN_OID_E;
    
    return StoreKey(cert);
}


/* process NAME, either issuer or subject */
static int GetName(DecodedCert* cert, int nameType)
{
    Sha    sha;
    int    length;  /* length of all distinguished names */
    int    dummy;
    char* full = (nameType == ISSUER) ? cert->issuer : cert->subject;
    word32 idx = 0;

    InitSha(&sha);

    if (GetSequence(cert->source, &cert->srcIdx, &length) < 0)
        return ASN_PARSE_E;

    length += cert->srcIdx;

    while (cert->srcIdx < (word32)length) {
        byte   b;
        byte   joint[2];
        int    oidSz;

        if (GetSet(cert->source, &cert->srcIdx, &dummy) < 0)
            return ASN_PARSE_E;

        if (GetSequence(cert->source, &cert->srcIdx, &dummy) < 0)
            return ASN_PARSE_E;

        b = cert->source[cert->srcIdx++];
        if (b != ASN_OBJECT_ID) 
            return ASN_OBJECT_ID_E;

        if (GetLength(cert->source, &cert->srcIdx, &oidSz) < 0)
            return ASN_PARSE_E;

        memcpy(joint, &cert->source[cert->srcIdx], sizeof(joint));

        /* v1 name types */
        if (joint[0] == 0x55 && joint[1] == 0x04) {
            byte   id;
            byte   copy = FALSE;
            int    strLen;

            cert->srcIdx += 2;
            id = cert->source[cert->srcIdx++]; 
            b  = cert->source[cert->srcIdx++];    /* strType */

            if (GetLength(cert->source, &cert->srcIdx, &strLen) < 0)
                return ASN_PARSE_E;

            if (id == ASN_COMMON_NAME) {
                if (nameType == ISSUER) {
                    cert->issuerCN = (char *)&cert->source[cert->srcIdx];
                    cert->issuerCNLen = strLen;
                } else {
                    cert->subjectCN = (char *)&cert->source[cert->srcIdx];
                    cert->subjectCNLen = strLen;
                }

                memcpy(&full[idx], "/CN=", 4);
                idx += 4;
                copy = TRUE;
            }
            else if (id == ASN_SUR_NAME) {
                memcpy(&full[idx], "/SN=", 4);
                idx += 4;
                copy = TRUE;
            }
            else if (id == ASN_COUNTRY_NAME) {
                memcpy(&full[idx], "/C=", 3);
                idx += 3;
                copy = TRUE;
            }
            else if (id == ASN_LOCALITY_NAME) {
                memcpy(&full[idx], "/L=", 3);
                idx += 3;
                copy = TRUE;
            }
            else if (id == ASN_STATE_NAME) {
                memcpy(&full[idx], "/ST=", 4);
                idx += 4;
                copy = TRUE;
            }
            else if (id == ASN_ORG_NAME) {
                memcpy(&full[idx], "/O=", 3);
                idx += 3;
                copy = TRUE;
            }
            else if (id == ASN_ORGUNIT_NAME) {
                memcpy(&full[idx], "/OU=", 4);
                idx += 4;
                copy = TRUE;
            }

            if (copy) {
                memcpy(&full[idx], &cert->source[cert->srcIdx], strLen);
                idx += strLen;
            }

            ShaUpdate(&sha, &cert->source[cert->srcIdx], strLen);
            cert->srcIdx += strLen;
        }
        else {
            /* skip */
            byte email = FALSE;
            int  adv;

            if (joint[0] == 0x2a && joint[1] == 0x86)  /* email id hdr */
                email = TRUE;

            cert->srcIdx += oidSz + 1;

            if (GetLength(cert->source, &cert->srcIdx, &adv) < 0)
                return ASN_PARSE_E;

            if (email) {
                memcpy(&full[idx], "/emailAddress=", 14);
                idx += 14;

                memcpy(&full[idx], &cert->source[cert->srcIdx], adv);
                idx += adv;
            }

            cert->srcIdx += adv;
        }
    }
    full[idx++] = 0;

    if (nameType == ISSUER)
        ShaFinal(&sha, cert->issuerHash);
    else
        ShaFinal(&sha, cert->subjectHash);

    return 0;
}


/* to the minute */
static int DateGreaterThan(const struct tm* a, const struct tm* b)
{
    if (a->tm_year > b->tm_year)
        return 1;

    if (a->tm_year == b->tm_year && a->tm_mon > b->tm_mon)
        return 1;
    
    if (a->tm_year == b->tm_year && a->tm_mon == b->tm_mon &&
           a->tm_mday > b->tm_mday)
        return 1;

    if (a->tm_year == b->tm_year && a->tm_mon == b->tm_mon &&
        a->tm_mday == b->tm_mday && a->tm_hour > b->tm_hour)
        return 1;

    if (a->tm_year == b->tm_year && a->tm_mon == b->tm_mon &&
        a->tm_mday == b->tm_mday && a->tm_hour == b->tm_hour &&
        a->tm_min > b->tm_min)
        return 1;

    return 0; /* false */
}


static INLINE int DateLessThan(const struct tm* a, const struct tm* b)
{
    return !DateGreaterThan(a,b);
}


/* like atoi but only use first byte */
static INLINE word32 btoi(byte b)
{
    return b - 0x30;
}


/* two byte date/time, add to value */
static INLINE void GetTime(int* value, const byte* date, int* idx)
{
    int i = *idx;

    *value += btoi(date[i++]) * 10;
    *value += btoi(date[i++]);

    *idx = i;
}


/* Make sure before and after dates are valid */
static int ValidateDate(const byte* date, byte format, int dateType)
{
    time_t ltime = time(0);
    struct tm  certTime;
    struct tm* localTime;
    int    i = 0;

    memset(&certTime, 0, sizeof(certTime));

    if (format == ASN_UTC_TIME) {
        if (btoi(date[0]) >= 5)
            certTime.tm_year = 1900;
        else
            certTime.tm_year = 2000;
    }
    else  { /* format == GENERALIZED_TIME */
        certTime.tm_year += btoi(date[i++]) * 1000;
        certTime.tm_year += btoi(date[i++]) * 100;
    }

    GetTime(&certTime.tm_year, date, &i); certTime.tm_year -= 1900; /* adjust */
    GetTime(&certTime.tm_mon,  date, &i); certTime.tm_mon  -= 1;    /* adjust */
    GetTime(&certTime.tm_mday, date, &i);
    GetTime(&certTime.tm_hour, date, &i); 
    GetTime(&certTime.tm_min,  date, &i); 
    GetTime(&certTime.tm_sec,  date, &i); 

    assert(date[i] == 'Z');     /* only Zulu supported for this profile */

    localTime = gmtime(&ltime);

    if (dateType == BEFORE) {
        if (DateLessThan(localTime, &certTime))
            return 0;
    }
    else
        if (DateGreaterThan(localTime, &certTime))
            return 0;

    return 1;
}


static int GetDate(DecodedCert* cert, int dateType)
{
    int    length;
    byte   date[MAX_DATE_SIZE];
    byte   b = cert->source[cert->srcIdx++];

    if (b != ASN_UTC_TIME && b != ASN_GENERALIZED_TIME)
        return ASN_TIME_E;

    if (GetLength(cert->source, &cert->srcIdx, &length) < 0)
        return ASN_PARSE_E;

    if (length > MAX_DATE_SIZE || length < MIN_DATE_SIZE)
        return ASN_DATE_SZ_E;

    memcpy(date, &cert->source[cert->srcIdx], length);
    cert->srcIdx += length;

    if (!ValidateDate(date, b, dateType)) {
        if (dateType == BEFORE)
            return ASN_BEFORE_DATE_E;
        else
            return ASN_AFTER_DATE_E;
    }

    return 0;
}


static int GetValidity(DecodedCert* cert, int verify)
{
    int length;

    if (GetSequence(cert->source, &cert->srcIdx, &length) < 0)
        return ASN_PARSE_E;

    if (GetDate(cert, BEFORE) < 0 && verify)
        return ASN_BEFORE_DATE_E;
    
    if (GetDate(cert, AFTER) < 0 && verify)
        return ASN_AFTER_DATE_E;
    
    return 0;
}


static int DecodeToKey(DecodedCert* cert, word32 inSz, int verify)
{
    int ret;

    if ( (ret = GetCertHeader(cert, inSz)) < 0)
        return ret;

    if ( (ret = GetAlgoId(cert, &cert->signatureOID)) < 0)
        return ret;

    if ( (ret = GetName(cert, ISSUER)) < 0)
        return ret;

    if ( (ret = GetValidity(cert, verify)) < 0)
        return ret;

    if ( (ret = GetName(cert, SUBJECT)) < 0)
        return ret;

    if ( (ret = GetKey(cert)) < 0)
        return ret;

    return ret;
}


static int GetSignature(DecodedCert* cert)
{
    int    length;
    byte   b = cert->source[cert->srcIdx++];

    if (b != ASN_BIT_STRING)
        return ASN_BITSTR_E;

    if (GetLength(cert->source, &cert->srcIdx, &length) < 0)
        return ASN_PARSE_E;

    cert->sigLength = length;

    b = cert->source[cert->srcIdx++];
    if (b != 0x00)
        return ASN_EXPECT_0_E;

    cert->sigLength--;
    cert->signature = &cert->source[cert->srcIdx];
    cert->srcIdx += cert->sigLength;

    return 0;
}


static word32 SetDigest(const byte* digest, word32 digSz, byte* output)
{
    output[0] = ASN_OCTET_STRING;
    output[1] = digSz;
    memcpy(&output[2], digest, digSz);

    return digSz + 2;
} 


static word32 BytePrecision(word32 value)
{
    word32 i;
    for (i = sizeof(value); i; --i)
        if (value >> (i - 1) * 8)
            break;

    return i;
}


static word32 SetLength(word32 length, byte* output)
{
    word32 i = 0, j;

    if (length < ASN_LONG_LENGTH)
        output[i++] = length;
    else {
        output[i++] = BytePrecision(length) | ASN_LONG_LENGTH;
      
        for (j = BytePrecision(length); j; --j) {
            output[i] = length >> (j - 1) * 8;
            i++;
        }
    }

    return i;
}


static word32 SetSequence(word32 len, byte* output)
{
    output[0] = ASN_SEQUENCE | ASN_CONSTRUCTED;
    return SetLength(len, output + 1) + 1;
}


static word32 SetAlgoID(int algoOID, byte* output)
{
    /* adding TAG_NULL and 0 to end */
    static const byte shaAlgoID[] = { 0x2b, 0x0e, 0x03, 0x02, 0x1a,
                                      0x05, 0x00 };
    static const byte md5AlgoID[] = { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
                                      0x02, 0x05, 0x05, 0x00  };
    static const byte md2AlgoID[] = { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
                                      0x02, 0x02, 0x05, 0x00};

    int    algoSz = 0;
    word32 idSz, seqSz;
    const  byte* algoName = 0;
    byte ID_Length[MAX_LENGTH_SZ];
    byte seqArray[MAX_SEQ_SZ + 1];  /* add object_id to end */

    switch (algoOID) {
    case SHAh:
        algoSz = sizeof(shaAlgoID);
        algoName = shaAlgoID;
        break;

    case MD2h:
        algoSz = sizeof(md2AlgoID);
        algoName = md2AlgoID;
        break;

    case MD5h:
        algoSz = sizeof(md5AlgoID);
        algoName = md5AlgoID;
        break;

    default:
        return 0;  /* UNKOWN_HASH_E; */
    }


    idSz  = SetLength(algoSz - 2, ID_Length); /* don't include TAG_NULL/0 */
    seqSz = SetSequence(idSz + algoSz + 1, seqArray);
    seqArray[seqSz++] = ASN_OBJECT_ID;

    memcpy(output, seqArray, seqSz);
    memcpy(output + seqSz, ID_Length, idSz);
    memcpy(output + seqSz + idSz, algoName, algoSz);

    return seqSz + idSz + algoSz;

}


word32 EncodeSignature(byte* out, const byte* digest, word32 digSz, int hashOID)
{
    byte digArray[MAX_ENCODED_DIG_SZ];
    byte algoArray[MAX_ALGO_SZ];
    byte seqArray[MAX_SEQ_SZ];
    word32 encDigSz, algoSz, seqSz; 

    encDigSz = SetDigest(digest, digSz, digArray);
    algoSz   = SetAlgoID(hashOID, algoArray);
    seqSz    = SetSequence(encDigSz + algoSz, seqArray);

    memcpy(out, seqArray, seqSz);
    memcpy(out + seqSz, algoArray, algoSz);
    memcpy(out + seqSz + algoSz, digArray, encDigSz);

    return encDigSz + algoSz + seqSz;
}
                           

/* return true (1) for Confirmation */
static int ConfirmSignature(DecodedCert* cert, const byte* key, word32 keySz)
{
    byte digest[SHA_DIGEST_SIZE]; /* max size */
    int  hashType, digestSz, ret;

    if (cert->signatureOID == MD5wRSA) {
        Md5 md5;
        InitMd5(&md5);
        Md5Update(&md5, cert->source + cert->certBegin,
                  cert->sigIndex - cert->certBegin);
        Md5Final(&md5, digest);
        hashType = MD5h;
        digestSz = MD5_DIGEST_SIZE;
    }
    else if (cert->signatureOID == SHAwRSA || cert->signatureOID == SHAwDSA) {
        Sha sha;
        InitSha(&sha);
        ShaUpdate(&sha, cert->source + cert->certBegin,
                  cert->sigIndex - cert->certBegin);
        ShaFinal(&sha, digest);
        hashType = SHAh;
        digestSz = SHA_DIGEST_SIZE;
    }
    else
        return 0; /* ASN_SIG_HASH_E; */

    if (cert->keyOID == RSAk) {
        RsaKey pubKey;
        byte   encodedSig[MAX_ENCODED_SIG_SZ];
        byte   plain[MAX_ENCODED_SIG_SZ];
        word32 idx = 0;
        int    sigSz, verifySz;
        byte*  out;

        if (cert->sigLength > MAX_ENCODED_SIG_SZ)
            return 0; /* the key is too big */
            
        InitRsaKey(&pubKey, cert->heap);
        if (RsaPublicKeyDecode(key, &idx, &pubKey, keySz) < 0)
            ret = 0; /* ASN_KEY_DECODE_E; */

        else {
            memcpy(plain, cert->signature, cert->sigLength);
            if ( (verifySz = RsaSSL_VerifyInline(plain, cert->sigLength, &out,
                                           &pubKey)) < 0)
                ret = 0; /* ASN_VERIFY_E; */
            else {
                /* make sure we're right justified */
                sigSz = EncodeSignature(encodedSig, digest, digestSz, hashType);
                if (sigSz != verifySz || memcmp(out, encodedSig, sigSz) != 0)
                    ret = 0; /* ASN_VERIFY_MATCH_E; */
                else
                    ret = 1; /* match */
            }
        }
        FreeRsaKey(&pubKey);
        return ret;
    }
    else
        return 0; /* ASN_SIG_KEY_E; */

    return 0;  /* not confirmed */
}


int ParseCert(DecodedCert* cert, word32 inSz, int type, int verify,
              Signer* signers)
{
    int   ret;
    char* ptr;

    ret = ParseCertRelative(cert, inSz, type, verify, signers);
    if (ret < 0)
        return ret;

    if (cert->issuerCNLen > 0) {
        ptr = (char*) XMALLOC(cert->issuerCNLen + 1, cert->heap);
        if (ptr == NULL)
            return MEMORY_E;
        memcpy(ptr, cert->issuerCN, cert->issuerCNLen);
        ptr[cert->issuerCNLen] = '\0';
        cert->issuerCN = ptr;
        cert->issuerCNLen = 0;
    }

    if (cert->subjectCNLen > 0) {
        ptr = (char*) XMALLOC(cert->subjectCNLen + 1, cert->heap);
        if (ptr == NULL)
            return MEMORY_E;
        memcpy(ptr, cert->subjectCN, cert->subjectCNLen);
        ptr[cert->subjectCNLen] = '\0';
        cert->subjectCN = ptr;
        cert->subjectCNLen = 0;
    }

    if (cert->pubKeySize > 0) {
        ptr = (char*) XMALLOC(cert->pubKeySize, cert->heap);
        if (ptr == NULL)
            return MEMORY_E;
        memcpy(ptr, cert->publicKey, cert->pubKeySize);
        cert->publicKey = (byte *)ptr;
        cert->pubKeyStored = 1;
    }

    if (cert->sigLength > 0) {
        ptr = (char*) XMALLOC(cert->sigLength, cert->heap);
        if (ptr == NULL)
            return MEMORY_E;
        memcpy(ptr, cert->signature, cert->sigLength);
        cert->signature = (byte *)ptr;
        cert->signatureStored = 1;
    }

    return ret;
}


int ParseCertRelative(DecodedCert* cert, word32 inSz, int type, int verify,
              Signer* signers)
{
    word32 confirmOID;
    int    ret;
    int    confirm = 0;

    if ((ret = DecodeToKey(cert, inSz, verify)) < 0)
        return ret;

    if (cert->srcIdx != cert->sigIndex)
        cert->srcIdx =  cert->sigIndex;

    if ((ret = GetAlgoId(cert, &confirmOID)) < 0)
        return ret;

    if ((ret = GetSignature(cert)) < 0)
        return ret;

    if (confirmOID != cert->signatureOID)
        return ASN_SIG_OID_E;

    if (verify && type != CA_TYPE) {
        while (signers) {
            if (memcmp(cert->issuerHash, signers->hash, SHA_DIGEST_SIZE)
                       == 0) {
                /* other confirm */
                if (!ConfirmSignature(cert, signers->publicKey,
                                      signers->pubKeySize))
                    return ASN_SIG_CONFIRM_E;
                else {
                    confirm = 1;
                    break;
                }
            }
            signers = signers->next;
        }
        if (!confirm)
            return ASN_SIG_CONFIRM_E;
    }

    return 0;
}


Signer* MakeSigner(void* heap)
{
    Signer* signer = (Signer*) XMALLOC(sizeof(Signer), heap);
    if (signer) {
        signer->name      = 0;
        signer->publicKey = 0;
        signer->next      = 0;
    }

    return signer;
}


void FreeSigners(Signer* signer, void* heap)
{
    Signer* next = signer;

    while( (signer = next) ) {
        next = signer->next;
        XFREE(signer->name, heap);
        XFREE(signer->publicKey, heap);
        XFREE(signer, heap);
    }
}


void CTaoCryptErrorString(int error, char* buffer)
{
    const int max = MAX_ERROR_SZ;   /* shorthand */

#ifdef NO_ERROR_STRINGS

    strncpy(buffer, "no support for error strings built in", max);

#else

    switch (error) {

    case OPEN_RAN_E :        
        strncpy(buffer, "opening random device error", max);
        break;

    case READ_RAN_E :
        strncpy(buffer, "reading random device error", max);
        break;

    case WINCRYPT_E :
        strncpy(buffer, "windows crypt init error", max);
        break;

    case CRYPTGEN_E : 
        strncpy(buffer, "windows crypt generation error", max);
        break;

    case RAN_BLOCK_E : 
        strncpy(buffer, "random device read would block error", max);
        break;

    case MP_INIT_E :
        strncpy(buffer, "mp_init error state", max);
        break;

    case MP_READ_E :
        strncpy(buffer, "mp_read error state", max);
        break;

    case MP_EXPTMOD_E :
        strncpy(buffer, "mp_exptmod error state", max);
        break;

    case MP_TO_E :
        strncpy(buffer, "mp_to_xxx error state, can't convert", max);
        break;

    case MP_SUB_E :
        strncpy(buffer, "mp_sub error state, can't subtract", max);
        break;

    case MP_ADD_E :
        strncpy(buffer, "mp_add error state, can't add", max);
        break;

    case MP_MUL_E :
        strncpy(buffer, "mp_mul error state, can't multiply", max);
        break;

    case MP_MULMOD_E :
        strncpy(buffer, "mp_mulmod error state, can't multiply mod", max);
        break;

    case MP_MOD_E :
        strncpy(buffer, "mp_mod error state, can't mod", max);
        break;

    case MP_INVMOD_E :
        strncpy(buffer, "mp_invmod error state, can't inv mod", max);
        break; 
        
    case MEMORY_E :
        strncpy(buffer, "out of memory error", max);
        break;

    case RSA_WRONG_TYPE_E :
        strncpy(buffer, "RSA wrong block type for RSA function", max);
        break; 

    case RSA_BUFFER_E :
        strncpy(buffer, "RSA buffer error, output too small or input too big",
                max);
        break; 

    case ASN_PARSE_E :
        strncpy(buffer, "ASN parsing error, invalid input", max);
        break;

    case ASN_VERSION_E :
        strncpy(buffer, "ASN version error, invalid number", max);
        break;

    case ASN_GETINT_E :
        strncpy(buffer, "ASN get big int error, invalid data", max);
        break;

    case ASN_RSA_KEY_E :
        strncpy(buffer, "ASN key init error, invalid input", max);
        break;

    case ASN_OBJECT_ID_E :
        strncpy(buffer, "ASN object id error, invalid id", max);
        break;

    case ASN_TAG_NULL_E :
        strncpy(buffer, "ASN tag error, not null", max);
        break;

    case ASN_EXPECT_0_E :
        strncpy(buffer, "ASN expect error, not zero", max);
        break;

    case ASN_BITSTR_E :
        strncpy(buffer, "ASN bit string error, wrong id", max);
        break;

    case ASN_UNKNOWN_OID_E :
        strncpy(buffer, "ASN oid error, unknown sum id", max);
        break;

    case ASN_DATE_SZ_E :
        strncpy(buffer, "ASN date error, bad size", max);
        break;

    case ASN_BEFORE_DATE_E :
        strncpy(buffer, "ASN date error, current date before", max);
        break;

    case ASN_AFTER_DATE_E :
        strncpy(buffer, "ASN date error, current date after", max);
        break;

    case ASN_SIG_OID_E :
        strncpy(buffer, "ASN signature error, mismatched oid", max);
        break;

    case ASN_TIME_E :
        strncpy(buffer, "ASN time error, unkown time type", max);
        break;

    case ASN_INPUT_E :
        strncpy(buffer, "ASN input error, not enough data", max);
        break;

    case ASN_SIG_CONFIRM_E :
        strncpy(buffer, "ASN sig error, confirm failure", max);
        break;

    case ASN_SIG_HASH_E :
        strncpy(buffer, "ASN sig error, unsupported hash type", max);
        break;

    case ASN_SIG_KEY_E :
        strncpy(buffer, "ASN sig error, unsupported key type", max);
        break;

    case ASN_DH_KEY_E :
        strncpy(buffer, "ASN key init error, invalid input", max);
        break;

    default:
        strncpy(buffer, "unknown error number", max);

    }

#endif

}

