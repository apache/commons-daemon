/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
/*
 * Copyright (c) 1994
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Andrew Hume of AT&T Bell Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "apxwin.h"
#include "private.h"

typedef UINT    word;       /* "word" used for optimal copy speed */
#define wsize   sizeof(UINT)
#define wmask   (wsize - 1)

LPVOID AplFillMemory(PVOID Destination, SIZE_T Length, BYTE Fill)
{ 

    SIZE_T t;
    UINT   c;
    LPBYTE dst;

    dst = Destination;
    /*
     * If not enough words, just fill bytes.  A length >= 2 words
     * guarantees that at least one of them is `complete' after
     * any necessary alignment.  For instance:
     *
     *  |-----------|-----------|-----------|
     *  |00|01|02|03|04|05|06|07|08|09|0A|00|
     *            ^---------------------^
     *       dst         dst+length-1
     *
     * but we use a minimum of 3 here since the overhead of the code
     * to do word writes is substantial.
     */
    if (Length < 3 * wsize) {
        while (Length != 0) {
            *dst++ = Fill;
            --Length;
        }
        return (Destination);
    }

    if ((c = Fill) != 0) {  /* Fill the word. */
        c = (c << 8) | c;   /* u_int is 16 bits. */
        c = (c << 16) | c;  /* u_int is 32 bits. */
#ifdef WIN64
        c = (c << 32) | c;  /* u_int is 64 bits. */
#endif
    }
    /* Align destination by filling in bytes. */
    if ((t = (SIZE_T)dst & wmask) != 0) {
        t = wsize - t;
        Length -= t;
        do {
            *dst++ = Fill;
        } while (--t != 0);
    }

    /* Fill words.  Length was >= 2*words so we know t >= 1 here. */
    t = Length / wsize;
    do {
        *(UINT *)dst = c;
        dst += wsize;
    } while (--t != 0);

    /* Mop up trailing bytes, if any. */
    t = Length & wmask;
    if (t != 0)
        do {
            *dst++ = Fill;
        } while (--t != 0);
    return (Destination); 
}

void AplZeroMemory(PVOID Destination, SIZE_T Length)
{ 

    SIZE_T t;
    LPBYTE dst;

    dst = Destination;
    /*
     * If not enough words, just fill bytes.  A length >= 2 words
     * guarantees that at least one of them is `complete' after
     * any necessary alignment.  For instance:
     *
     *  |-----------|-----------|-----------|
     *  |00|01|02|03|04|05|06|07|08|09|0A|00|
     *            ^---------------------^
     *       dst         dst+length-1
     *
     * but we use a minimum of 3 here since the overhead of the code
     * to do word writes is substantial.
     */
    if (Length < 3 * wsize) {
        while (Length != 0) {
            *dst++ = 0;
            --Length;
        }
        return;
    }

    /* Align destination by filling in bytes. */
    if ((t = (SIZE_T)dst & wmask) != 0) {
        t = wsize - t;
        Length -= t;
        do {
            *dst++ = 0;
        } while (--t != 0);
    }

    /* Fill words.  Length was >= 2*words so we know t >= 1 here. */
    t = Length / wsize;
    do {
        *(UINT *)dst = 0;
        dst += wsize;
    } while (--t != 0);

    /* Mop up trailing bytes, if any. */
    t = Length & wmask;
    if (t != 0)
        do {
            *dst++ = 0;
        } while (--t != 0);
}

LPVOID AplCopyMemory(PVOID Destination, const VOID* Source, SIZE_T Length)
{
    char *dst = Destination;
    const char *src = Source;
    SIZE_T t;

    if (Length == 0 || dst == src)      /* nothing to do */
        goto done;

    /*
     * Macros: loop-t-times; and loop-t-times, t>0
     */
#define TLOOP(s) if (t) TLOOP1(s)
#define TLOOP1(s) do { s; } while (--t)

    if ((SIZE_T)dst < (SIZE_T)src) {
        /*
         * Copy forward.
         */
        t = (int)(SIZE_T)src;   /* only need low bits */
        if ((t | (int)(SIZE_T)dst) & wmask) {
            /*
             * Try to align operands.  This cannot be done
             * unless the low bits match.
             */
            if ((t ^ (int)(SIZE_T)dst) & wmask || Length < wsize)
                t = Length;
            else
                t = wsize - (t & wmask);
            Length -= t;
            TLOOP1(*dst++ = *src++);
        }
        /*
         * Copy whole words, then mop up any trailing bytes.
         */
        t = Length / wsize;
        TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
        t = Length & wmask;
        TLOOP(*dst++ = *src++);
    } else {
        /*
         * Copy backwards.  Otherwise essentially the same.
         * Alignment works as before, except that it takes
         * (t&wmask) bytes to align, not wsize-(t&wmask).
         */
        src += Length;
        dst += Length;
        t = (int)(SIZE_T)src;
        if ((t | (int)(SIZE_T)dst) & wmask) {
            if ((t ^ (int)(SIZE_T)dst) & wmask || Length <= wsize)
                t = Length;
            else
                t &= wmask;
            Length -= t;
            TLOOP1(*--dst = *--src);
        }
        t = Length / wsize;
        TLOOP(src -= wsize; dst -= wsize; *(word *)dst = *(word *)src);
        t = Length & wmask;
        TLOOP(*--dst = *--src);
    }
done:
    return (Destination);
} 


INT
AplMemCmp(LPCVOID lpA, LPCVOID lpB, SIZE_T nBytes)
{
    if (nBytes != 0) {
        const BYTE *p1 = lpA, *p2 = lpB;

        do {
            if (*p1++ != *p2++)
                return (*--p1 - *--p2);
        } while (--nBytes != 0);
    }
    return 0;
} 

/*
 * Find the first occurrence of lpFind in lpMem.
 * dwLen:   The length of lpFind
 * dwSize:  The length of lpMem
 */
LPBYTE
ApcMemSearch(LPCVOID lpMem, LPCVOID lpFind, SIZE_T dwLen, SIZE_T dwSize)
{
    BYTE   c, sc;
    SIZE_T cnt = 0;
    const BYTE *s = lpMem, *find = lpFind;

    if ((c = *find++) != 0) {
        do {
            do {
                sc = *s++;
                if (cnt++ > dwSize)
                    return NULL;
            } while (sc != c);
        } while (AplMemCmp(s, find, dwLen - 1) != 0);
        s--;
    }
    return (LPBYTE)s;
} 

LPSTR
AplRindexA(LPCSTR lpStr, int ch)
{
    LPSTR save;

    for (save = NULL;; ++lpStr) {
        if (*lpStr == ch)
            save = (LPSTR)lpStr;
        if (!*lpStr)
            return save;
    } 
    /* NOTREACHED */
} 
