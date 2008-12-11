/*
 * Copyright (c) 2008  Kungliga Tekniska Högskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "krb5/gsskrb5_locl.h"

#include <roken.h>

static OM_uint32
iov_allocate(OM_uint32 *minor_status, gss_iov_buffer_desc *iov, size_t iov_count)
{
    unsigned int i;
    
    for (i = 0; i < iov_count; i++) {
	if (iov[i].flags & GSS_IOV_BUFFER_FLAG_ALLOCATE) {
	    void *ptr = malloc(iov[i].buffer.length);
	    if (ptr == NULL)
		abort();
	    memcpy(ptr, iov[i].buffer.value, iov[i].buffer.length);
	    iov[i].buffer.value = ptr;
	    iov[i].flags |= GSS_IOV_BUFFER_FLAG_ALLOCATED;
	}
    }
    return GSS_S_COMPLETE;
}

static OM_uint32
iov_map(OM_uint32 *minor_status, size_t iov_count,
	const gss_iov_buffer_desc *iov,
	krb5_crypto_iov *data)
{
    unsigned int i;

    for (i = 0; i < iov_count; i++) {
	switch(iov[i].type) {
	case GSS_IOV_BUFFER_TYPE_EMPTY:
	    data[i].flags = KRB5_CRYPTO_TYPE_EMPTY;
	    break;
	case GSS_IOV_BUFFER_TYPE_DATA:
	    if (iov[i].flags & GSS_IOV_BUFFER_FLAG_SIGN_ONLY)
		data[i].flags = KRB5_CRYPTO_TYPE_SIGN_ONLY;
	    else
		data[i].flags = KRB5_CRYPTO_TYPE_DATA;
	    break;
	case GSS_IOV_BUFFER_TYPE_HEADER:
	    data[i].flags = KRB5_CRYPTO_TYPE_HEADER;
	    break;
	case GSS_IOV_BUFFER_TYPE_TRAILER:
	    data[i].flags = KRB5_CRYPTO_TYPE_TRAILER;
	    break;
	case GSS_IOV_BUFFER_TYPE_PADDING:
	    data[i].flags = KRB5_CRYPTO_TYPE_PADDING;
	    break;
	case GSS_IOV_BUFFER_TYPE_STREAM:
	    abort();
	    break;
	default:
	    *minor_status = EINVAL;
	    return GSS_S_FAILURE;
	}
	data[i].data.data = iov[i].buffer.value;
	data[i].data.length = iov[i].buffer.length;
    }
    return GSS_S_COMPLETE;
}

OM_uint32 GSSAPI_LIB_FUNCTION
_gk_wrap_iov(OM_uint32 * minor_status,
	     gss_ctx_id_t  context_handle,
	     int conf_req_flag,
	     gss_qop_t qop_req,
	     int * conf_state,
	     size_t iov_count,
	     gss_iov_buffer_desc *iov)
{
    gsskrb5_ctx ctx = (gsskrb5_ctx) context_handle;
    krb5_crypto crypto = NULL;
    krb5_context context;
    OM_uint32 major_status, junk;
    krb5_crypto_iov *data;
    unsigned usage;

    GSSAPI_KRB5_INIT (&context);

    major_status = iov_allocate(minor_status, iov, iov_count);
    if (major_status != GSS_S_COMPLETE)
	return major_status;

    data = calloc(iov_count, sizeof(data[0]));
    if (data == NULL) {
	gss_release_iov_buffer(&junk, iov_count, iov);
	*minor_status = ENOMEM;
	return GSS_S_FAILURE;
    }

    major_status = iov_map(minor_status, iov_count, iov, data);
    if (major_status != GSS_S_COMPLETE) {
	gss_release_iov_buffer(&junk, iov_count, iov);
	free(data);
	return major_status;
    }

    if (ctx->more_flags & LOCAL) {
	usage = KRB5_KU_USAGE_ACCEPTOR_SIGN;
    } else {
	usage = KRB5_KU_USAGE_INITIATOR_SIGN;
    }

    *minor_status = krb5_encrypt_iov_ivec(context, crypto, usage,
					  data, iov_count, NULL);
    free(data);
    if (major_status != GSS_S_COMPLETE) {
	gss_release_iov_buffer(&junk, iov_count, iov);
	return major_status;
    }

    *minor_status = 0;
    return GSS_S_COMPLETE;
}

OM_uint32 GSSAPI_LIB_FUNCTION
_gk_unwrap_iov(OM_uint32 *minor_status,
	       gss_ctx_id_t context_handle,
	       int *conf_state,
	       gss_qop_t *qop_state,
	       size_t iov_count,
	       gss_iov_buffer_desc *iov)
{
    gsskrb5_ctx ctx = (gsskrb5_ctx) context_handle;
    krb5_crypto crypto = NULL;
    krb5_context context;
    OM_uint32 major_status, junk;
    krb5_crypto_iov *data;
    unsigned usage;

    GSSAPI_KRB5_INIT (&context);

    major_status = iov_allocate(minor_status, iov, iov_count);
    if (major_status != GSS_S_COMPLETE)
	return major_status;

    data = calloc(iov_count, sizeof(data[0]));
    if (data == NULL) {
	gss_release_iov_buffer(&junk, iov_count, iov);
	*minor_status = ENOMEM;
	return GSS_S_FAILURE;
    }

    major_status = iov_map(minor_status, iov_count, iov, data);
    if (major_status != GSS_S_COMPLETE) {
	gss_release_iov_buffer(&junk, iov_count, iov);
	free(data);
	return major_status;
    }

    if (ctx->more_flags & LOCAL) {
	usage = KRB5_KU_USAGE_INITIATOR_SIGN;
    } else {
	usage = KRB5_KU_USAGE_ACCEPTOR_SIGN;
    }

    *minor_status = krb5_decrypt_iov_ivec(context, crypto, usage,
					  data, iov_count, NULL);
    free(data);
    if (major_status != GSS_S_COMPLETE) {
	gss_release_iov_buffer(&junk, iov_count, iov);
	return major_status;
    }

    *minor_status = 0;
    return GSS_S_COMPLETE;
}

OM_uint32  GSSAPI_LIB_FUNCTION
_gk_wrap_iov_length(OM_uint32 * minor_status,
		    gss_ctx_id_t context_handle,
		    int conf_req_flag,
		    gss_qop_t qop_req,
		    size_t iov_count,
		    gss_iov_buffer_desc *iov)
{
    krb5_context context;
    krb5_crypto crypto = NULL;
    unsigned int i;
    size_t size;
    size_t *padding = NULL;

    GSSAPI_KRB5_INIT (&context);
    *minor_status = 0;

    for (size = 0, i = 0; i < iov_count; i++) {
	switch(iov[i].type) {
	case GSS_IOV_BUFFER_TYPE_EMPTY:
	    break;
	case GSS_IOV_BUFFER_TYPE_DATA:
	    size += iov[i].buffer.length;
	    break;
	case GSS_IOV_BUFFER_TYPE_HEADER:
	    iov[i].buffer.length =
	      krb5_crypto_length(context, crypto, KRB5_CRYPTO_TYPE_HEADER);
	    size += iov[i].buffer.length;
	    break;
	case GSS_IOV_BUFFER_TYPE_TRAILER:
	    iov[i].buffer.length =
	      krb5_crypto_length(context, crypto, KRB5_CRYPTO_TYPE_TRAILER);
	    size += iov[i].buffer.length;
	    break;
	case GSS_IOV_BUFFER_TYPE_PADDING:
	    if (padding != NULL) {
		*minor_status = 0;
		return GSS_S_FAILURE;
	    }
	    padding = &iov[i].buffer.length;
	    break;
	case GSS_IOV_BUFFER_TYPE_STREAM:
	    size += iov[i].buffer.length;
	    break;
	default:
	    *minor_status = EINVAL;
	    return GSS_S_FAILURE;
	}
    }
    if (padding) {
	size_t pad = krb5_crypto_length(context, crypto, KRB5_CRYPTO_TYPE_PADDING);
	if (pad > 1) {
	    *padding = pad - (size % pad);
	    if (*padding == pad)
		*padding = 0;
	} else
	    *padding = 0;
    }

    return GSS_S_COMPLETE;
}