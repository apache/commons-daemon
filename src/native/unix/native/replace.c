/* ========================================================================= *
 *                                                                           *
 *                 The Apache Software License,  Version 1.1                 *
 *                                                                           *
 *          Copyright (c) 1999-2003 The Apache Software Foundation.          *
 *                           All rights reserved.                            *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * Redistribution and use in source and binary forms,  with or without modi- *
 * fication, are permitted provided that the following conditions are met:   *
 *                                                                           *
 * 1. Redistributions of source code  must retain the above copyright notice *
 *    notice, this list of conditions and the following disclaimer.          *
 *                                                                           *
 * 2. Redistributions  in binary  form  must  reproduce the  above copyright *
 *    notice,  this list of conditions  and the following  disclaimer in the *
 *    documentation and/or other materials provided with the distribution.   *
 *                                                                           *
 * 3. The end-user documentation  included with the redistribution,  if any, *
 *    must include the following acknowlegement:                             *
 *                                                                           *
 *       "This product includes  software developed  by the Apache  Software *
 *        Foundation <http://www.apache.org/>."                              *
 *                                                                           *
 *    Alternately, this acknowlegement may appear in the software itself, if *
 *    and wherever such third-party acknowlegements normally appear.         *
 *                                                                           *
 * 4. The names  "The  Jakarta  Project",  "WebApp",  and  "Apache  Software *
 *    Foundation"  must not be used  to endorse or promote  products derived *
 *    from this  software without  prior  written  permission.  For  written *
 *    permission, please contact <apache@apache.org>.                        *
 *                                                                           *
 * 5. Products derived from this software may not be called "Apache" nor may *
 *    "Apache" appear in their names without prior written permission of the *
 *    Apache Software Foundation.                                            *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES *
 * INCLUDING, BUT NOT LIMITED TO,  THE IMPLIED WARRANTIES OF MERCHANTABILITY *
 * AND FITNESS FOR  A PARTICULAR PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL *
 * THE APACHE  SOFTWARE  FOUNDATION OR  ITS CONTRIBUTORS  BE LIABLE  FOR ANY *
 * DIRECT,  INDIRECT,   INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL *
 * DAMAGES (INCLUDING,  BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS *
 * OR SERVICES;  LOSS OF USE,  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION) *
 * HOWEVER CAUSED AND  ON ANY  THEORY  OF  LIABILITY,  WHETHER IN  CONTRACT, *
 * STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN *
 * ANY  WAY  OUT OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF  ADVISED  OF THE *
 * POSSIBILITY OF SUCH DAMAGE.                                               *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * This software  consists of voluntary  contributions made  by many indivi- *
 * duals on behalf of the  Apache Software Foundation.  For more information *
 * on the Apache Software Foundation, please see <http://www.apache.org/>.   *
 *                                                                           *
 * ========================================================================= */

/* @version $Id: replace.c,v 1.1 2003/09/04 23:28:20 yoavs Exp $ */
#include "jsvc.h"

/* Replace all occurrences of a string in another */
int replace(char *new, int len, char *old, char *mch, char *rpl) {
    char *tmp;
    int count;
    int shift;
    int nlen;
    int olen;
    int mlen;
    int rlen;
    int x;

    /* The new buffer is NULL, fail */
    if (new==NULL) return(-1);
    /* The length of the buffer is less than zero, fail */
    if (len<0) return(-2);
    /* The old buffer is NULL, fail */
    if (old==NULL) return(-3);

    /* The string to be matched is NULL or empty, simply copy */
    if ((mch==NULL)||(strlen(mch)==0)) {
        olen=strlen(old);
        if (len<=olen) return(olen+1);
        strcpy(new,old);
        return(0);
    }

    /* The string to be replaced is NULL, assume it's an empty string */
    if (rpl==NULL) rpl="";

    /* Evaluate some lengths */
    olen=strlen(old);
    mlen=strlen(mch);
    rlen=strlen(rpl);

    /* Calculate how many times the mch string appears in old */
    tmp=old;
    count=0;
    while((tmp=strstr(tmp,mch))!=NULL) {
        count++;
        tmp+=mlen;
    }

    /* We have no matches, simply copy */
    if (count==0) {
        olen=strlen(old);
        if (len<=olen) return(olen+1);
        strcpy(new,old);
        return(0);
    }

    /* Calculate how big the buffer must be to hold the translation
       and of how many bytes we need to shift the data */
    shift=rlen-mlen;
    nlen=olen+(shift*count);
    /* printf("Count=%d Shift= %d OLen=%d NLen=%d\n",count,shift,olen,nlen); */

    /* Check if we have enough size in the buffer */
    if (nlen>=len) return(nlen+1);

    /* Copy over the old buffer in the new one (save memory) */
    strcpy(new,old);

    /* Start replacing */
    tmp=new;
    while((tmp=strstr(tmp,mch))!=NULL) {
        /* If shift is > 0 we need to move data from right to left */
        if (shift>0) {
            for (x=(strlen(tmp)+shift);x>shift;x--) {
                /*
                printf("src %c(%d) dst %c(%d)\n",
                        tmp[x-shift],tmp[x-shift],tmp[x],tmp[x]);
                 */
                tmp[x]=tmp[x-shift];
            }
        /* If shift is < 0 we need to move data from left to right */
        } else if (shift<0) {
            for (x=mlen;x<strlen(tmp)-shift;x++) {
                /*
                   printf("src %c(%d) dst %c(%d)\n",
                          tmp[x],tmp[x],tmp[x+shift],tmp[x+shift]);
                 */
                tmp[x+shift]=tmp[x];
            }
        }
        /* If shift is = 0 we don't have to shift data */
        strncpy(tmp,rpl,rlen);
        tmp+=rlen;
        /* printf("\"%s\"\n",tmp); */
    }
    return(0);
}
