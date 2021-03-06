/*
 * Copyright 2013-2015 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#ifndef KSI_SIGNATURE_H_
#define KSI_SIGNATURE_H_

#include "types.h"
#include "verification.h"
#include "verify_deprecated.h"
#include "hashchain.h"

#ifdef __cplusplus
extern "C" {
#endif
	/**
	 * \addtogroup signature KSI Signature
	 * At the highest level of abstraction, a keyless signature consists of a hash chain linking the
	 * signed document to the root hash value of the aggregation tree, followed by another hash chain
	 * linking the root hash value of the aggregation tree to the published trust anchor.
	 * @{
	 */
#include "signature_helper.h"

	/**
	 * Free the signature object.
	 * \param[in]	signature		Signature object.
	 */
	void KSI_Signature_free(KSI_Signature *signature);

	/**
	 * Creates a clone of the signature object.
	 * \param[in]		sig			Signature to be cloned.
	 * \param[out]		clone		Pointer to the receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an error code).
	 */
	int KSI_Signature_clone(const KSI_Signature *sig, KSI_Signature **clone);

	/**
	 * Parses a KSI signature from raw buffer and verifies it with the provided policy and context.
	 * The raw buffer may be freed after this function finishes.
	 * To reserialize the signature use #KSI_Signature_serialize.
	 *
	 * \param[in]		ctx			KSI context.
	 * \param[in]		raw			Pointer to the raw signature.
	 * \param[in]		raw_len		Length of the raw signature.
	 * \param[in]		policy		Verification policy.
	 * \param[in]		context		Verification context.
	 * \param[out]		sig			Pointer to the receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 */
	int KSI_Signature_parseWithPolicy(KSI_CTX *ctx, const unsigned char *raw, size_t raw_len, const KSI_Policy *policy, KSI_VerificationContext *context, KSI_Signature **sig);

#define KSI_Signature_parse(ctx, raw, raw_len, sig) KSI_Signature_parseWithPolicy(ctx, raw, raw_len, KSI_VERIFICATION_POLICY_INTERNAL, NULL, sig)

	/**
	 * This function serializes the signature object into raw data. To deserialize it again
	 * use #KSI_Signature_parse.
	 * \param[in]		sig			Signature object.
	 * \param[out]		raw			Pointer to the pointer to output buffer.
	 * \param[out]		raw_len		Pointer to the length of the buffer variable.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 *
	 * \note The output memory buffer belongs to the caller and needs to be freed
	 * by the caller using #KSI_free.
	 */
	int KSI_Signature_serialize(const KSI_Signature *sig, unsigned char **raw, size_t *raw_len);

	/**
	 * This function signs the given root hash value (\c rootHash) with the aggregation level (\c rootLevel)
	 * of a locally aggregated hash tree. This function requires access to a working aggregaton and fails if
	 * it is not accessible. The signature is verified with the provided policy and context.
	 * \param[in]		ctx			KSI context.
	 * \param[in]		rootHash	Root value of the hash tree.
	 * \param[in]		rootLevel	Level of the root node (0 =< x <= 0xff).
	 * \param[in]		policy		Verification policy.
	 * \param[in]		context		Verification context.
	 * \param[out]		signature	Pointer to the receiving pointer.
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 * \see #KSI_createSignature, KSI_Signature_create, KSI_Signature_free.
	 */
	int KSI_Signature_signAggregatedWithPolicy(KSI_CTX *ctx, KSI_DataHash *rootHash, KSI_uint64_t rootLevel, const KSI_Policy *policy, KSI_VerificationContext *context, KSI_Signature **signature);

#define KSI_Signature_signAggregated(ctx, rootHash, rootLevel, signature) KSI_Signature_signAggregatedWithPolicy(ctx, rootHash, rootLevel, KSI_VERIFICATION_POLICY_INTERNAL, NULL, signature)

	/**
	 * This function creates a new signature using the aggrehation hash chain as the input. The aggregation hash chain will
	 * be included in the signature itself.
	 * \param[in]		ctx			KSI context.
	 * \param[in]		level		The level of the input hash of the aggregation hash chain.
	 * \param[in]		chn			Aggregation hash chain.
	 * \param[out]		signature	Pointer to the receiving pointer.
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an error code).
	 * \note The function does not consume the aggregation hash chain - the caller must free the resource.
	 */
	int KSI_Signature_signAggregationChain(KSI_CTX *ctx, int level, KSI_AggregationHashChain *chn, KSI_Signature **signature);

	/**
	 * This function extends the signature to the given publication \c pubRec. If \c pubRec is \c NULL the signature is
	 * extended to the head of the calendar database. This function requires access to a working KSI extender or it will
	 * fail with an error. The extended signature is verified with the provided policy and context.
	 * \param[in]		signature	KSI signature to be extended.
	 * \param[in]		ctx			KSI context.
	 * \param[in]		pubRec		Publication record.
	 * \param[in]		policy		Verification policy.
	 * \param[in]		context		Verification context.
	 * \param[out]		extended	Pointer to the receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 *
	 * \note The output signature is independent of the input signature and needs to be freed using #KSI_Signature_free.
	 */
	int KSI_Signature_extendWithPolicy(const KSI_Signature *signature, KSI_CTX *ctx, const KSI_PublicationRecord *pubRec, const KSI_Policy *policy, KSI_VerificationContext *context, KSI_Signature **extended);

#define KSI_Signature_extend(signature, ctx, pubRec, extended) KSI_Signature_extendWithPolicy(signature, ctx, pubRec, KSI_VERIFICATION_POLICY_INTERNAL, NULL, extended)

	/**
	 * Extends the signature to a given time \c to. If \c to is equal to \c NULL, the signature is extended to
	 * the head of the extender. The extended signature is verified with the provided policy and context.
	 * \param[in]		signature	KSI signature to be extended.
	 * \param[in]		ctx			KSI context.
	 * \param[in]		to			UTC time to extend to.
	 * \param[in]		policy		Verification policy.
	 * \param[in]		context		Verification context.
	 * \param[out]		extended 	Pointer to the receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 *
	 * \note Extending to a specific time will remove calendar auth record and publication record.
	 */
	int KSI_Signature_extendToWithPolicy(const KSI_Signature *signature, KSI_CTX *ctx, KSI_Integer *to, const KSI_Policy *policy, KSI_VerificationContext *context, KSI_Signature **extended);

#define KSI_Signature_extendTo(signature, ctx, to, extended) KSI_Signature_extendToWithPolicy(signature, ctx, to, KSI_VERIFICATION_POLICY_INTERNAL, NULL, extended)

	/**
	 * Access method for the signed document hash as a #KSI_DataHash object.
	 * \param[in]		sig			KSI signature.
	 * \param[out]		hsh			Pointer to receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 *
	 * \note The output hash \c hsh may not be freed by the caller.
	 */
	int KSI_Signature_getDocumentHash(const KSI_Signature *sig, KSI_DataHash ** hsh);

	/**
	 * Access method for the signing time. The \c signTime is expressed as
	 * the number of seconds since 1970-01-01 00:00:00 UTC.
	 * \param[in]		sig			KSI signature.
	 * \param[out]		signTime	Pointer to the receiving variable.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 */
	int KSI_Signature_getSigningTime(const KSI_Signature *sig, KSI_Integer **signTime);

	/**
	 * Function to get signer identity.
	 * \param[in]		sig			KSI signature.
	 * \param[out]		identity	Pointer to receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 *
	 * \note The output memory buffer belongs to the caller and needs to be freed
	 * by the caller using #KSI_free.
	 * \note This method is deprecated and will be removed in later versions, use
	 * #KSI_Signature_getAggregationHashChainIdentity
	 */
	KSI_FN_DEPRECATED(int KSI_Signature_getSignerIdentity(KSI_Signature *sig, char **identity));

	/**
	 * Function for getting a list of the identities present in all aggregation hash chains.
	 * The identities in the list are ordered - the higher-aggregator identity is before lower-aggregator identity.
	 *
	 * \param[in]		sig			KSI signature.
	 * \param[out]		identity	Pointer to receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 */
	int KSI_Signature_getAggregationHashChainIdentity(const KSI_Signature *sig, KSI_HashChainLinkIdentityList **identity);

	/**
	 * Accessor method for the published data. If the signature does not have a publication
	 * record the \c pubRec will be set to \c NULL.
	 * \param[in]		sig			KSI signature.
	 * \param[out]		pubRec		Pointer to receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 */
	int KSI_Signature_getPublicationRecord(const KSI_Signature *sig, KSI_PublicationRecord **pubRec);

	/**
	 * Accessor method for the calendar authentication record.
	 * \param[in]	sig		Signature
	 * \param[out]	calendarAuthRec		Pointer to the receiving pointer.
	 *
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 */
	int KSI_Signature_getCalendarAuthRec (const KSI_Signature *sig, KSI_CalendarAuthRec **calendarAuthRec);

	int KSI_createSignRequest(KSI_CTX *ctx, KSI_DataHash *hsh, int lvl, KSI_AggregationReq **request);
	int KSI_createExtendRequest(KSI_CTX *ctx, KSI_Integer *start, KSI_Integer *end, KSI_ExtendReq **request);

	/**
	 * Replaces the existing publication record of the signature.
	 * \param[in]	sig		KSI signature.
	 * \param[in]	pubRec	Publication record.
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 */
	int KSI_Signature_replacePublicationRecord(KSI_Signature *sig, KSI_PublicationRecord *pubRec);

	/**
	 * Function for getting publication information from an extended signature.
	 * \param [in]		sig			Extended signature including publication record.
	 * \param [out]		pubHsh		Publication hash.
	 * \param [out]		pubStr		Publication data converted into a base-32 encoded string.
	 * \param [out]		pubDate		Publicatoin date
	 * \param [out]		pubRefs		Publication references.
	 * \param [out]		repUrls		Publication URL repositories.
	 * \return status code (#KSI_OK, when operation succeeded, otherwise an error code).
	 *
	 * \note	The output memory has to be freed by the caller
	 * \see		#KSI_DataHash_free, #KSI_Utf8String_free, #KSI_Utf8StringList_free
	 */
	int KSI_Signature_getPublicationInfo(const KSI_Signature *sig, KSI_DataHash **pubHsh, KSI_Utf8String **pubStr, time_t *pubDate, KSI_LIST(KSI_Utf8String) **pubRefs, KSI_LIST(KSI_Utf8String) **repUrls);

	KSI_DEFINE_REF(KSI_Signature);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* KSI_SIGNATURE_H_ */
