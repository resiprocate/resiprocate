#if !defined(Srtp2Helper_hxx)
#define Srtp2Helper_hxx

// grep 'not declared' /tmp/errs.log | cut -f5- -d: | sort -u | sed -e 's/ was not declared in this scope; did you mean//' | tr -d "'?" | while read ; do echo "#define$REPLY" ; done

#define err_status_t srtp_err_status_t
#define err_status_ok srtp_err_status_ok
#define crypto_policy_t srtp_crypto_policy_t
#define crypto_policy_set_aes_cm_256_hmac_sha1_80 srtp_crypto_policy_set_aes_cm_256_hmac_sha1_80
#define crypto_policy_set_aes_cm_128_hmac_sha1_32 srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32
#define crypto_policy_set_aes_cm_128_null_auth srtp_crypto_policy_set_aes_cm_128_null_auth
#define crypto_policy_set_null_cipher_hmac_sha1_80 srtp_crypto_policy_set_null_cipher_hmac_sha1_80
#define crypto_policy_set_aes_cm_128_hmac_sha1_80 srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80
#define crypto_policy_set_aes_cm_256_hmac_sha1_32 srtp_crypto_policy_set_aes_cm_256_hmac_sha1_32
#define crypto_policy_set_from_profile_for_rtp srtp_crypto_policy_set_from_profile_for_rtp
#define crypto_policy_set_from_profile_for_rtcp srtp_crypto_policy_set_from_profile_for_rtcp
#define ssrc_t srtp_ssrc_t
#define err_status_algo_fail srtp_err_status_algo_fail
#define err_status_alloc_fail srtp_err_status_alloc_fail
#define err_status_auth_fail srtp_err_status_auth_fail
#define err_status_bad_param srtp_err_status_bad_param
#define err_status_cant_check srtp_err_status_cant_check
#define err_status_cipher_fail srtp_err_status_cipher_fail
#define err_status_dealloc_fail srtp_err_status_dealloc_fail
#define err_status_encode_err srtp_err_status_encode_err
#define err_status_fail srtp_err_status_fail
#define err_status_init_fail srtp_err_status_init_fail
#define err_status_key_expired srtp_err_status_key_expired
#define err_status_no_ctx srtp_err_status_no_ctx
#define err_status_nonce_bad srtp_err_status_nonce_bad
#define err_status_no_such_op srtp_err_status_no_such_op
#define err_status_parse_err srtp_err_status_parse_err
#define err_status_pfkey_err srtp_err_status_pfkey_err
#define err_status_read_fail srtp_err_status_read_fail
#define err_status_replay_fail srtp_err_status_replay_fail
#define err_status_replay_old srtp_err_status_replay_old
#define err_status_semaphore_err srtp_err_status_semaphore_err
#define err_status_signal_err srtp_err_status_signal_err
#define err_status_socket_err srtp_err_status_socket_err
#define err_status_terminus srtp_err_status_terminus
#define err_status_write_fail srtp_err_status_write_fail

#endif

/* ====================================================================
 *
 * Copyright 2020 Daniel Pocock https://danielpocock.com  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 */
