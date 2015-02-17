#include <limits.h>
#include "internal.h"

#include "tlv.h"
#include "tlv_template.h"
#include "hashchain.h"

#define KSI_CalAuthRecPKISignedData_new KSI_PKISignedData_new
#define KSI_CalAuthRecPKISignedData_free KSI_PKISignedData_free

#define KSI_AggrAuthRecPKISignedData_new KSI_PKISignedData_new
#define KSI_AggrAuthRecPKISignedData_free KSI_PKISignedData_free

#define KSI_Utf8StringNZ_new KSI_Utf8String_new
#define KSI_Utf8StringNZ_free KSI_Utf8String_free

#define FLAGSET(tmpl, flg) (((tmpl).flags & flg) != 0)

static int extractGenerator(KSI_CTX *ctx, void *payload, void *generatorCtx, const KSI_TlvTemplate *tmpl, int (*generator)(void *, KSI_TLV **), char *buf, size_t buf_len, const size_t buf_size);
static int extract(KSI_CTX *ctx, void *payload, KSI_TLV *tlv, const KSI_TlvTemplate *tmpl, char *buf, size_t buf_len, const size_t buf_size);

KSI_DEFINE_TLV_TEMPLATE(KSI_CalAuthRecPKISignedData)
	KSI_TLV_UTF8_STRING(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PKISignedData_getSigType, KSI_PKISignedData_setSigType, "sign_data")
	KSI_TLV_OCTET_STRING(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PKISignedData_getSignatureValue, KSI_PKISignedData_setSignatureValue, "signature")
	KSI_TLV_OCTET_STRING(0x03, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PKISignedData_getCertId, KSI_PKISignedData_setCertId, "cert_id")
	KSI_TLV_OBJECT(0x04, KSI_TLV_TMPL_FLG_NONE, KSI_PKISignedData_getCertRepositoryUri, KSI_PKISignedData_setCertRepositoryUri, KSI_Utf8StringNZ_fromTlv, KSI_Utf8StringNZ_toTlv, KSI_Utf8String_free, "cert_rep_uri")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_AggrAuthRecPKISignedData)
	KSI_TLV_UTF8_STRING(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PKISignedData_getSigType, KSI_PKISignedData_setSigType, "sig_type")
	KSI_TLV_OCTET_STRING(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PKISignedData_getSignatureValue, KSI_PKISignedData_setSignatureValue, "signed_data")
	KSI_TLV_OCTET_STRING(0x03, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PKISignedData_getCertId, KSI_PKISignedData_setCertId, "cert_id")
	KSI_TLV_OBJECT(0x04, KSI_TLV_TMPL_FLG_NONE, KSI_PKISignedData_getCertRepositoryUri, KSI_PKISignedData_setCertRepositoryUri, KSI_Utf8StringNZ_fromTlv, KSI_Utf8StringNZ_toTlv, KSI_Utf8String_free, "cert_rep_uri")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_PublicationsHeader)
	KSI_TLV_INTEGER(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PublicationsHeader_getVersion, KSI_PublicationsHeader_setVersion, "version")
	KSI_TLV_TIME_S(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PublicationsHeader_getTimeCreated, KSI_PublicationsHeader_setTimeCreated, "time_created")
	KSI_TLV_OBJECT(0x03, KSI_TLV_TMPL_FLG_NONE, KSI_PublicationsHeader_getRepositoryUri, KSI_PublicationsHeader_setRepositoryUri, KSI_Utf8StringNZ_fromTlv, KSI_Utf8StringNZ_toTlv, KSI_Utf8String_free, "rep_uri")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_CertificateRecord)
	KSI_TLV_OCTET_STRING(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_CertificateRecord_getCertId, KSI_CertificateRecord_setCertId, "cert_id")
	KSI_TLV_OBJECT(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_CertificateRecord_getCert, KSI_CertificateRecord_setCert, KSI_PKICertificate_fromTlv, KSI_PKICertificate_toTlv, KSI_PKICertificate_free, "cert")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_PublicationData)
	KSI_TLV_TIME_S(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PublicationData_getTime, KSI_PublicationData_setTime, "pub_time")
	KSI_TLV_IMPRINT(0x04, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PublicationData_getImprint, KSI_PublicationData_setImprint, "imprint")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_PublicationRecord)
	KSI_TLV_COMPOSITE(0x10, KSI_TLV_TMPL_FLG_MANDATORY, KSI_PublicationRecord_getPublishedData, KSI_PublicationRecord_setPublishedData, KSI_PublicationData, "pub_data")
	KSI_TLV_OBJECT_LIST(0x09, KSI_TLV_TMPL_FLG_NONE, KSI_PublicationRecord_getPublicationRefList, KSI_PublicationRecord_setPublicationRefList, KSI_Utf8StringNZ, "pub_ref")
	KSI_TLV_OBJECT_LIST(0x0a, KSI_TLV_TMPL_FLG_NONE, KSI_PublicationRecord_getRepositoryUriList, KSI_PublicationRecord_setRepositoryUriList, KSI_Utf8StringNZ, "uri")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_MetaData)
	KSI_TLV_UTF8_STRING(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_MetaData_getClientId, KSI_MetaData_setClientId, "client_id")
	KSI_TLV_OCTET_STRING(0x02, KSI_TLV_TMPL_FLG_NONE, KSI_MetaData_getMachineId, KSI_MetaData_setMachineId, "machine_id")
	KSI_TLV_INTEGER(0x03, KSI_TLV_TMPL_FLG_NONE, KSI_MetaData_getSequenceNr, KSI_MetaData_setSequenceNr, "seq_nr")
	KSI_TLV_TIME_US(0x04, KSI_TLV_TMPL_FLG_NONE, KSI_MetaData_getRequestTimeInMicros, KSI_MetaData_setRequestTimeInMicros, "req_time")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_HashChainLink)
	KSI_TLV_INTEGER(0x01, KSI_TLV_TMPL_FLG_NONE, KSI_HashChainLink_getLevelCorrection, KSI_HashChainLink_setLevelCorrection, "level_correction")
	KSI_TLV_IMPRINT(0x02, KSI_TLV_TMPL_FLG_MANTATORY_MOST_ONE_G0, KSI_HashChainLink_getImprint, KSI_HashChainLink_setImprint, "imprint")
	KSI_TLV_META_IMPRINT(0x03, KSI_TLV_TMPL_FLG_MANTATORY_MOST_ONE_G0, KSI_HashChainLink_getMetaHash, KSI_HashChainLink_setMetaHash, "meta_hash")
	KSI_TLV_OBJECT(0x04, KSI_TLV_TMPL_FLG_MANTATORY_MOST_ONE_G0, KSI_HashChainLink_getMetaData, KSI_HashChainLink_setMetaData, KSI_MetaData_fromTlv, KSI_MetaData_toTlv, KSI_MetaData_free, "meta_data")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_Header)
	KSI_TLV_OCTET_STRING(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_Header_getLoginId, KSI_Header_setLoginId, "login_id")
	KSI_TLV_INTEGER(0x02, KSI_TLV_TMPL_FLG_NONE, KSI_Header_getInstanceId, KSI_Header_setInstanceId, "instance_id")
	KSI_TLV_INTEGER(0x03, KSI_TLV_TMPL_FLG_NONE, KSI_Header_getMessageId, KSI_Header_setMessageId, "message_id")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_Config)
	KSI_TLV_INTEGER(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_Config_getMaxLevel, KSI_Config_setMaxLevel, "max_level")
	KSI_TLV_INTEGER(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_Config_getAggrAlgo, KSI_Config_setAggrAlgo, "aggr_algo")
	KSI_TLV_INTEGER(0x03, KSI_TLV_TMPL_FLG_MANDATORY, KSI_Config_getAggrPeriod, KSI_Config_setAggrPeriod, "aggr_period")
	KSI_TLV_UTF8_STRING_LIST(0x04, KSI_TLV_TMPL_FLG_MANDATORY, KSI_Config_getParentUri, KSI_Config_setParentUri, "parent_uri")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_AggregationHashChain)
	KSI_TLV_TIME_S(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationHashChain_getAggregationTime, KSI_AggregationHashChain_setAggregationTime, "aggr_time")
	KSI_TLV_INTEGER_LIST(0x03, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationHashChain_getChainIndex, KSI_AggregationHashChain_setChainIndex, "chain_index")
	KSI_TLV_OCTET_STRING(0x04, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationHashChain_getInputData, KSI_AggregationHashChain_setInputData, "input_data")
	KSI_TLV_IMPRINT(0x05, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationHashChain_getInputHash, KSI_AggregationHashChain_setInputHash, "input_hash")
	KSI_TLV_INTEGER(0x06, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationHashChain_getAggrHashId, KSI_AggregationHashChain_setAggrHashId, "hash_id")
	KSI_TLV_OBJECT_LIST(0x07, KSI_TLV_TMPL_FLG_LEAST_ONE_G0, KSI_AggregationHashChain_getChain, KSI_AggregationHashChain_setChain, KSI_HashChainLink, "aggr_chain")
	KSI_TLV_OBJECT_LIST(0x08, KSI_TLV_TMPL_FLG_LEAST_ONE_G0 | KSI_TLV_TMPL_FLG_NO_SERIALIZE, KSI_AggregationHashChain_getChain, KSI_AggregationHashChain_setChain, KSI_HashChainLink, "aggr_chain")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_AggregationAuthRec)
	KSI_TLV_TIME_S(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationAuthRec_getAggregationTime, KSI_AggregationAuthRec_setAggregationTime, "aggr_time")
	KSI_TLV_INTEGER_LIST(0x03, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationAuthRec_getChainIndex, KSI_AggregationAuthRec_setChainIndex, "chain_index")
	KSI_TLV_IMPRINT(0x05, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationAuthRec_getInputHash, KSI_AggregationAuthRec_setInputHash, "input_hash")
	KSI_TLV_COMPOSITE(0x0b, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationAuthRec_getSigData, KSI_AggregationAuthRec_setSigData, KSI_AggrAuthRecPKISignedData, "signed_data")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_CalendarAuthRec)
	KSI_TLV_COMPOSITE(0x10, KSI_TLV_TMPL_FLG_FORWARD | KSI_TLV_TMPL_FLG_MANDATORY | KSI_TLV_TMPL_FLG_MORE_DEFS, KSI_CalendarAuthRec_getPublishedData, KSI_CalendarAuthRec_setPublishedData, KSI_PublicationData, "pub_data")
	KSI_TLV_UNPROCESSED(0x10, KSI_CalendarAuthRec_setSignedData)
	KSI_TLV_COMPOSITE(0x0b, KSI_TLV_TMPL_FLG_MANDATORY, KSI_CalendarAuthRec_getSignatureData, KSI_CalendarAuthRec_setSignatureData, KSI_CalAuthRecPKISignedData, "signature")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_AggregationReq)
	KSI_TLV_INTEGER(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationReq_getRequestId, KSI_AggregationReq_setRequestId, "req_id")
	KSI_TLV_IMPRINT(0x02, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationReq_getRequestHash, KSI_AggregationReq_setRequestHash, "req_hash")
	KSI_TLV_INTEGER(0x03, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationReq_getRequestLevel, KSI_AggregationReq_setRequestLevel, "req_level")
	KSI_TLV_COMPOSITE(0x04, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationReq_getConfig, KSI_AggregationReq_setConfig, KSI_Config, "config")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_RequestAck)
	KSI_TLV_INTEGER(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_RequestAck_getAggregationPeriod, KSI_RequestAck_setAggregationPeriod, "aggr_period")
	KSI_TLV_INTEGER(0x02, KSI_TLV_TMPL_FLG_MANDATORY, KSI_RequestAck_getAggregationDelay, KSI_RequestAck_setAggregationDelay, "aggr_delay")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_CalendarHashChain)
	KSI_TLV_TIME_S(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_CalendarHashChain_getPublicationTime, KSI_CalendarHashChain_setPublicationTime, "pub_time")
	KSI_TLV_TIME_S(0x02, KSI_TLV_TMPL_FLG_NONE, KSI_CalendarHashChain_getAggregationTime, KSI_CalendarHashChain_setAggregationTime, "aggr_time")
	KSI_TLV_IMPRINT(0x05, KSI_TLV_TMPL_FLG_MANDATORY, KSI_CalendarHashChain_getInputHash, KSI_CalendarHashChain_setInputHash, "input_hash")
	KSI_TLV_OBJECT_LIST(0x07, KSI_TLV_TMPL_FLG_LEAST_ONE_G0, KSI_CalendarHashChain_getHashChain, KSI_CalendarHashChain_setHashChain, KSI_CalendarHashChainLink, "chain")
	KSI_TLV_OBJECT_LIST(0x08, KSI_TLV_TMPL_FLG_LEAST_ONE_G0 | KSI_TLV_TMPL_FLG_NO_SERIALIZE, KSI_CalendarHashChain_getHashChain, KSI_CalendarHashChain_setHashChain, KSI_CalendarHashChainLink, "chain")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_AggregationResp)
	KSI_TLV_INTEGER(0x01, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getRequestId, KSI_AggregationResp_setRequestId, "req_id")
	KSI_TLV_INTEGER(0x04, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getStatus, KSI_AggregationResp_setStatus, "status")
	KSI_TLV_UTF8_STRING(0x05, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getErrorMsg, KSI_AggregationResp_setErrorMsg, "err_message")
	KSI_TLV_COMPOSITE(0x10, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getConfig, KSI_AggregationResp_setConfig, KSI_Config, "config")
	KSI_TLV_COMPOSITE(0x11, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getRequestAck, KSI_AggregationResp_setRequestAck, KSI_RequestAck, "req_ack")
	KSI_TLV_COMPOSITE_LIST(0x0801, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getAggregationChainList, KSI_AggregationResp_setAggregationChainList, KSI_AggregationHashChain, "aggr_chain")
	KSI_TLV_COMPOSITE(0x0802, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getCalendarChain, KSI_AggregationResp_setCalendarChain, KSI_CalendarHashChain, "cal_chain")
	KSI_TLV_COMPOSITE(0x0804, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getAggregationAuthRec, KSI_AggregationResp_setAggregationAuthRec, KSI_AggregationAuthRec, "aggr_auth_rec") /* TODO! Future work. */
	KSI_TLV_COMPOSITE(0x0805, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationResp_getCalendarAuthRec, KSI_AggregationResp_setCalendarAuthRec, KSI_CalendarAuthRec, "cal_auth_rec")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_AggregationPdu)
	KSI_TLV_OBJECT(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_AggregationPdu_getHeader, KSI_AggregationPdu_setHeader, KSI_Header_fromTlv, KSI_Header_toTlv, KSI_Header_free, "header")
	KSI_TLV_OBJECT(0x201, KSI_TLV_TMPL_FLG_MANTATORY_MOST_ONE_G0, KSI_AggregationPdu_getRequest, KSI_AggregationPdu_setRequest, KSI_AggregationReq_fromTlv, KSI_AggregationReq_toTlv, KSI_AggregationReq_free, "aggr_req")
	KSI_TLV_OBJECT(0x202, KSI_TLV_TMPL_FLG_MANTATORY_MOST_ONE_G0, KSI_AggregationPdu_getResponse, KSI_AggregationPdu_setResponse, KSI_AggregationResp_fromTlv, KSI_AggregationResp_toTlv, KSI_AggregationResp_free, "aggr_resp")
	KSI_TLV_IMPRINT(0x1F, KSI_TLV_TMPL_FLG_NONE, KSI_AggregationPdu_getHmac, KSI_AggregationPdu_setHmac, "hmac")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_ExtendReq)
	KSI_TLV_INTEGER(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_ExtendReq_getRequestId, KSI_ExtendReq_setRequestId, "req_id")
	KSI_TLV_TIME_S(0x02, KSI_TLV_TMPL_FLG_NONE, KSI_ExtendReq_getAggregationTime, KSI_ExtendReq_setAggregationTime, "aggr_time")
	KSI_TLV_TIME_S(0x03, KSI_TLV_TMPL_FLG_NONE, KSI_ExtendReq_getPublicationTime, KSI_ExtendReq_setPublicationTime, "pub_time")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_ExtendResp)
	KSI_TLV_INTEGER(0x01, KSI_TLV_TMPL_FLG_NONE, KSI_ExtendResp_getRequestId, KSI_ExtendResp_setRequestId, "req_id")
	KSI_TLV_INTEGER(0x04, KSI_TLV_TMPL_FLG_NONE, KSI_ExtendResp_getStatus, KSI_ExtendResp_setStatus, "status")
	KSI_TLV_UTF8_STRING(0x05, KSI_TLV_TMPL_FLG_NONE, KSI_ExtendResp_getErrorMsg, KSI_ExtendResp_setErrorMsg, "err_message")
	KSI_TLV_TIME_S(0x10, KSI_TLV_TMPL_FLG_NONE, KSI_ExtendResp_getLastTime, KSI_ExtendResp_setLastTime, "last_time")
	KSI_TLV_COMPOSITE(0x802, KSI_TLV_TMPL_FLG_NONE, KSI_ExtendResp_getCalendarHashChain, KSI_ExtendResp_setCalendarHashChain, KSI_CalendarHashChain, "cal_hash_chain")
KSI_END_TLV_TEMPLATE

KSI_DEFINE_TLV_TEMPLATE(KSI_ExtendPdu)
	KSI_TLV_OBJECT(0x01, KSI_TLV_TMPL_FLG_MANDATORY, KSI_ExtendPdu_getHeader, KSI_ExtendPdu_setHeader, KSI_Header_fromTlv, KSI_Header_toTlv, KSI_Header_free, "header")
	KSI_TLV_OBJECT(0x301, KSI_TLV_TMPL_FLG_MANTATORY_MOST_ONE_G0, KSI_ExtendPdu_getRequest, KSI_ExtendPdu_setRequest, KSI_ExtendReq_fromTlv, KSI_ExtendReq_toTlv, KSI_ExtendReq_free, "ext_req")
	KSI_TLV_OBJECT(0x302, KSI_TLV_TMPL_FLG_MANTATORY_MOST_ONE_G0, KSI_ExtendPdu_getResponse, KSI_ExtendPdu_setResponse, KSI_ExtendResp_fromTlv, KSI_ExtendResp_toTlv, KSI_ExtendResp_free, "ext_resp")
	KSI_TLV_IMPRINT(0x1F, KSI_TLV_TMPL_FLG_NONE, KSI_ExtendPdu_getHmac, KSI_ExtendPdu_setHmac, "hmac")
KSI_END_TLV_TEMPLATE

static int storeObjectValue(KSI_CTX *ctx, const KSI_TlvTemplate *tmpl, void *payload, void *val) {
	KSI_ERR err;
	int res;
	void *list = NULL;
	void *listp = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, tmpl != NULL) goto cleanup;
	KSI_PRE(&err, payload != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	if (tmpl->listAppend != NULL) {
		res = tmpl->getValue(payload, &listp);
		if (res != KSI_OK) goto cleanup;
		
		if (listp == NULL) {
			/* Make sure we have required function pointers. */
			if (tmpl->listNew == NULL || tmpl->listFree == NULL) {
				KSI_FAIL(&err, KSI_INVALID_ARGUMENT, "Template does not have list constructor or destructor, but list itself does not exist.");
				goto cleanup;
			}
			res = tmpl->listNew(&list);
			KSI_CATCH(&err, res) goto cleanup;

			listp = list;
		}
		
		res = tmpl->listAppend(listp, (void *) val);
		KSI_CATCH(&err, res) goto cleanup;
		
		res = tmpl->setValue(payload, listp);
		KSI_CATCH(&err, res) goto cleanup;

		list = NULL;

	} else {
		/* Regular value - store with the setter. */
		res = tmpl->setValue(payload, (void *) val);
		KSI_CATCH(&err, res) goto cleanup;
	}

	KSI_SUCCESS(&err);

cleanup:

	KSI_nofree(listp);
	if (tmpl->listFree != NULL) tmpl->listFree(list);

	return KSI_RETURN(&err);
}

typedef struct TLVListIterator_st {
	KSI_LIST(KSI_TLV) *list;
	size_t idx;
} TLVListIterator;

static int TLVListIterator_next(TLVListIterator *iter, KSI_TLV **tlv) {
	int res = KSI_UNKNOWN_ERROR;
	KSI_TLV *next = NULL;

	if (iter == NULL) {
		res = KSI_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (iter->idx < KSI_TLVList_length(iter->list)) {
		res = KSI_TLVList_elementAt(iter->list, iter->idx, &next);
		if (res != KSI_OK) goto cleanup;

		iter->idx++;
	}

	*tlv = next;

	res = KSI_OK;

cleanup:

	return res;
}

static int extract(KSI_CTX *ctx, void *payload, KSI_TLV *tlv, const KSI_TlvTemplate *tmpl, char *buf, size_t buf_len, const size_t buf_size) {
	KSI_ERR err;
	int res;
	size_t len;

	TLVListIterator iter;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_TLV);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_TLV_getNestedList(tlv, &iter.list);
	KSI_CATCH(&err, res) goto cleanup;

	iter.idx = 0;

	len = KSI_snprintf(buf + buf_len, buf_size, "[0x%02x]", KSI_TLV_getTag(tlv));

	res = extractGenerator(ctx, payload, (void *)&iter, tmpl, (int (*)(void *, KSI_TLV **))TLVListIterator_next, buf, buf_len + len, buf_size);
	KSI_CATCH(&err, res) {
		KSI_LOG_debug(ctx, "Unable to parse TLV: %s", buf);
		goto cleanup;
	}

	KSI_SUCCESS(&err);

cleanup:

	return KSI_RETURN(&err);

}

int KSI_TlvTemplate_extract(KSI_CTX *ctx, void *payload, KSI_TLV *tlv, const KSI_TlvTemplate *tmpl) {
	int res = KSI_UNKNOWN_ERROR;
	char buf[0xffff];
	*buf = '\0';
	res = extract(ctx, payload, tlv, tmpl, buf, 0, sizeof(buf));
	if (res != KSI_OK) {
		KSI_LOG_logTlv(ctx, KSI_LOG_DEBUG, "Parsed tlv at failure", tlv);
	}
	return res;
}

int KSI_TlvTemplate_parse(KSI_CTX *ctx, unsigned char *raw, unsigned raw_len, const KSI_TlvTemplate *tmpl, void *payload) {
	KSI_ERR err;
	int res;
	KSI_TLV *tlv = NULL;
	char buf[0xfff];

	*buf = '\0';

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, raw != NULL) goto cleanup;
	KSI_PRE(&err, raw_len > 0) goto cleanup;
	KSI_PRE(&err, tmpl != NULL) goto cleanup;
	KSI_PRE(&err, payload != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	res = KSI_TLV_parseBlob2(ctx, raw, raw_len, 0, &tlv);
	KSI_CATCH(&err, res) goto cleanup;

	res = extract(ctx, payload, tlv, tmpl, buf, 0, sizeof(buf));
	KSI_CATCH(&err, res) goto cleanup;

	KSI_LOG_logTlv(ctx, KSI_LOG_DEBUG, "Parsed TLV", tlv);

	KSI_SUCCESS(&err);

cleanup:

	KSI_TLV_free(tlv);

	return KSI_RETURN(&err);
}

static size_t getTemplateLength(const KSI_TlvTemplate *tmpl) {
	const KSI_TlvTemplate *tmp = NULL;
	size_t len = 0;

	/* Count the number of templates. */
	tmp = tmpl;
	while (tmp != NULL && tmp++->tag) ++len;

	return len;
}

static int extractGenerator(KSI_CTX *ctx, void *payload, void *generatorCtx, const KSI_TlvTemplate *tmpl, int (*generator)(void *, KSI_TLV **), char *buf, size_t buf_len, const size_t buf_size) {
	KSI_ERR err;
	KSI_TLV *tlv = NULL;
	int res;

	KSI_uint64_t uint64Val = 0;
	void *voidVal = NULL;
	void *compositeVal = NULL;
	void *valuep = NULL;
	KSI_TLV *tlvVal = NULL;

	size_t template_len = 0;
	bool *templateHit = NULL;
	bool groupHit[2] = {false, false};
	bool oneOf[2] = {false, false};
	size_t i;
	size_t tmplStart = 0;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	template_len = getTemplateLength(tmpl);

	/* Create the hit buffer with all values set to zero. */
	if (template_len > 0) {
		templateHit = KSI_calloc(template_len, sizeof(bool));
	}

	while (1) {
		int matchCount = 0;
		size_t len = 0;
		res = generator(generatorCtx, &tlv);
		KSI_CATCH(&err, res) goto cleanup;

		if (tlv == NULL) break;

		KSI_LOG_trace(ctx, "Starting to parse TLV(0x%02x)", KSI_TLV_getTag(tlv));
		len = KSI_snprintf(buf + buf_len, buf_size - buf_len, "[0x%02x]", KSI_TLV_getTag(tlv));

		for (i = tmplStart; i < template_len; i++) {
			if (tmpl[i].tag != KSI_TLV_getTag(tlv)) continue;
			if (i == tmplStart && !tmpl[i].multiple) tmplStart++;

			if (tmpl[i].descr != NULL) {
				len += KSI_snprintf(buf + buf_len + len, buf_size - buf_len - len, "%s", tmpl[i].descr);
			}
			matchCount++;
			templateHit[i] = true;
			if ((tmpl[i].flags & KSI_TLV_TMPL_FLG_LEAST_ONE_G0) != 0) groupHit[0] = true;
			if ((tmpl[i].flags & KSI_TLV_TMPL_FLG_LEAST_ONE_G1) != 0) groupHit[1] = true;

			if (FLAGSET(tmpl[i], KSI_TLV_TMPL_FLG_MOST_ONE_G0)) {
				if (oneOf[0]) {
					KSI_FAIL(&err, KSI_INVALID_FORMAT, "Mutually exclusive elements present within group 0.");
					goto cleanup;
				}
				oneOf[0] = true;
			}

			if (FLAGSET(tmpl[i], KSI_TLV_TMPL_FLG_MOST_ONE_G1)) {
				if (oneOf[1]) {
					KSI_FAIL(&err, KSI_INVALID_FORMAT, "Mutually exclusive elements present within group 0.");
					goto cleanup;
				}
				oneOf[1] = true;
			}

			valuep = NULL;
			if (tmpl[i].getValue != NULL) {
				/* Validate the value has not been set */
				res = tmpl[i].getValue(payload, (void **)&valuep);
				KSI_CATCH(&err, res) goto cleanup;
			}

			if (valuep != NULL && !tmpl[i].multiple) {
				compositeVal = NULL;
				KSI_LOG_error(ctx, "Multiple occurrances of a unique tag 0x%02x", tmpl[i].tag);
				KSI_FAIL(&err, KSI_INVALID_FORMAT, "To avoid memory leaks, a value may not be set more than once while parsing.");
				goto cleanup;
			}
			/* Parse the current TLV */
			switch (tmpl[i].type) {
				case KSI_TLV_TEMPLATE_SEEK_POS:
					KSI_LOG_trace(ctx, "Detected seek position template for TLV value extraction.");

					uint64Val = (KSI_uint64_t)KSI_TLV_getAbsoluteOffset(tlv);

					res = ((int (*)(void *, KSI_uint64_t))tmpl[i].setValue)(payload, uint64Val);
					KSI_CATCH(&err, res) goto cleanup;

					break;
				case KSI_TLV_TEMPLATE_UNPROCESSED:
					KSI_LOG_trace(ctx, "Detected unprocessed template for TLV value extraction.");

					res = KSI_TLV_clone(tlv, &tlvVal);
					KSI_CATCH(&err, res) goto cleanup;

					res = storeObjectValue(ctx, &tmpl[i], payload, tlvVal);
					KSI_CATCH(&err, res) goto cleanup;

					tlvVal = NULL;

					break;
				case KSI_TLV_TEMPLATE_OBJECT:
					KSI_LOG_trace(ctx, "Detected object template for TLV value extraction.");
					if (tmpl[i].fromTlv == NULL) {
						KSI_FAIL(&err, KSI_UNKNOWN_ERROR, "Invalid template: fromTlv not set.");
						goto cleanup;
					}

					res = tmpl[i].fromTlv(tlv, &voidVal);
					KSI_CATCH(&err, res) goto cleanup;

					res = storeObjectValue(ctx, &tmpl[i], payload, voidVal);
					KSI_CATCH(&err, res) {
						tmpl[i].destruct(voidVal);
						goto cleanup;
					}

					break;
				case KSI_TLV_TEMPLATE_COMPOSITE:
				{
					size_t last_len = len;
					KSI_LOG_trace(ctx, "Detected composite template for TLV value extraction.");

					res = tmpl[i].construct(ctx, &compositeVal);
					KSI_CATCH(&err, res) goto cleanup;

					len += KSI_snprintf(buf + buf_len + len, buf_size - buf_len - len, "->");

					res = extract(ctx, compositeVal, tlv, tmpl[i].subTemplate, buf, buf_len + len, buf_size);
					KSI_CATCH(&err, res) {
						KSI_LOG_error(ctx, "Unable to parse composite TLV: %s", buf);
						tmpl[i].destruct(compositeVal);
						goto cleanup;
					}

					res = storeObjectValue(ctx, &tmpl[i], payload, (void *)compositeVal);
					KSI_CATCH(&err, res) goto cleanup;

					KSI_LOG_trace(ctx, "Composite value %s extracted.", buf);

					/* Reset the buffer. */
					buf[len = last_len] = '\0';

					break;
				}
				default:
					KSI_LOG_error(ctx, "No template found.");
					/* Should not happen, but just in case. */
					KSI_FAIL(&err, KSI_UNKNOWN_ERROR, "Undefined template type");
					goto cleanup;
			}

			if ((tmpl[i].flags & KSI_TLV_TMPL_FLG_MORE_DEFS) == 0) break;
		}

		/* Check if a match was found, an raise an error if the TLV is marked as critical. */
		if (matchCount == 0 && !KSI_TLV_isNonCritical(tlv)) {
			KSI_LOG_error(ctx, "Unknown critical tag: %s", buf);
			KSI_FAIL(&err, KSI_INVALID_FORMAT, NULL);
			goto cleanup;
		}

		buf[buf_len] = '\0';
	}


	/* Check that every mandatory component was present. */
	for (i = 0; i < template_len; i++) {
		char errm[100];
		if ((tmpl[i].flags & KSI_TLV_TMPL_FLG_MANDATORY) != 0 && !templateHit[i]) {
			KSI_snprintf(errm, sizeof(errm), "Mandatory element missing: %s->[0x%x]%s", buf, tmpl[i].tag, tmpl[i].descr != NULL ? tmpl[i].descr : "");
			KSI_LOG_debug(ctx, "%s", errm);
			KSI_FAIL(&err, KSI_INVALID_FORMAT, errm);
			goto cleanup;
		}
		if (((tmpl[i].flags & KSI_TLV_TMPL_FLG_LEAST_ONE_G0) != 0 && !groupHit[0]) ||
				((tmpl[i].flags & KSI_TLV_TMPL_FLG_LEAST_ONE_G1) != 0 && !groupHit[1])) {
			KSI_snprintf(errm, sizeof(errm), "Mandatory group missing: %s->[0x%x]%s", buf, tmpl[i].tag, tmpl[i].descr != NULL ? tmpl[i].descr : "");
			KSI_LOG_debug(ctx, "%s", errm);
			KSI_FAIL(&err, KSI_INVALID_FORMAT, errm);
			goto cleanup;
		}
	}

	KSI_SUCCESS(&err);

cleanup:

	KSI_free(templateHit);
	KSI_TLV_free(tlvVal);

	return KSI_RETURN(&err);
}

int KSI_TlvTemplate_extractGenerator(KSI_CTX *ctx, void *payload, void *generatorCtx, const KSI_TlvTemplate *tmpl, int (*generator)(void *, KSI_TLV **)) {
	char buf[0xffff];
	*buf = '\0';
	return extractGenerator(ctx, payload, generatorCtx, tmpl, generator, buf, 0, sizeof(buf));
}

static int construct(KSI_CTX *ctx, KSI_TLV *tlv, const void *payload, const KSI_TlvTemplate *tmpl, char *buf, size_t buf_len, const size_t buf_size) {
	KSI_ERR err;
	int res;
	KSI_TLV *tmp = NULL;
	void *payloadp = NULL;
	int isNonCritical = 0;
	int isForward = 0;

	size_t template_len = 0;
	bool *templateHit = NULL;
	bool groupHit[2] = {false, false};
	bool oneOf[2] = {false, false};

	size_t i;

	KSI_PRE(&err, tlv != NULL) goto cleanup;
	KSI_PRE(&err, tmpl != NULL) goto cleanup;

	KSI_BEGIN(ctx, &err);

	template_len = getTemplateLength(tmpl);

	if (template_len > 0) {
		templateHit = KSI_calloc(template_len, sizeof(bool));
	}

	for (i = 0; i < template_len; i++) {
		if ((tmpl[i].flags & KSI_TLV_TMPL_FLG_NO_SERIALIZE) != 0) continue;
		payloadp = NULL;

		res = tmpl[i].getValue(payload, &payloadp);
		KSI_CATCH(&err, res) goto cleanup;

		if (payloadp != NULL) {
			size_t len = KSI_snprintf(buf, buf_size - buf_len, "[%02x]%s", tmpl[i].tag, tmpl[i].descr == NULL ? "" : tmpl[i].descr);

			templateHit[i] = true;

			if ((tmpl[i].flags & KSI_TLV_TMPL_FLG_LEAST_ONE_G0) != 0) groupHit[0] = true;
			if ((tmpl[i].flags & KSI_TLV_TMPL_FLG_LEAST_ONE_G1) != 0) groupHit[1] = true;
			if (FLAGSET(tmpl[i], KSI_TLV_TMPL_FLG_MOST_ONE_G0)) {
				if (oneOf[0]) {
					KSI_FAIL(&err, KSI_INVALID_FORMAT, "Mutually exclusive elements present within group 0.");
				}
				oneOf[0] = true;
			}
			if (FLAGSET(tmpl[i], KSI_TLV_TMPL_FLG_MOST_ONE_G1)) {
				if (oneOf[1]) {
					KSI_FAIL(&err, KSI_INVALID_FORMAT, "Mutually exclusive elements present within group 0.");
				}
				oneOf[1] = true;
			}

			isNonCritical = (tmpl[i].flags & KSI_TLV_TMPL_FLG_NONCRITICAL) != 0;
			isForward = (tmpl[i].flags & KSI_TLV_TMPL_FLG_FORWARD) != 0;

			switch (tmpl[i].type) {
				case KSI_TLV_TEMPLATE_OBJECT:
					if (tmpl[i].toTlv == NULL) {
						KSI_FAIL(&err, KSI_UNKNOWN_ERROR, "Invalid template: toTlv not set.");
						goto cleanup;
					}

					if (tmpl[i].listLength != NULL) {
						int j;
						for (j = 0; j < tmpl[i].listLength(payloadp); j++) {
							void *listElement = NULL;
							res = tmpl[i].listElementAt(payloadp, j, &listElement);
							KSI_CATCH(&err, res) goto cleanup;

							res = tmpl[i].toTlv(ctx, listElement, tmpl[i].tag, isNonCritical, isForward != 0, &tmp);
							KSI_CATCH(&err, res) goto cleanup;

							res = KSI_TLV_appendNestedTlv(tlv, NULL, tmp);
							KSI_CATCH(&err, res) goto cleanup;
							tmp = NULL;
						}


					} else {
						res = tmpl[i].toTlv(ctx, payloadp, tmpl[i].tag, isNonCritical, isForward, &tmp);
						KSI_CATCH(&err, res) goto cleanup;

						res = KSI_TLV_appendNestedTlv(tlv, NULL, tmp);
						KSI_CATCH(&err, res) goto cleanup;
						tmp = NULL;
					}

					break;
				case KSI_TLV_TEMPLATE_COMPOSITE:
					len += KSI_snprintf(buf + buf_len + len, buf_size - buf_len - len, "->");

					if (tmpl[i].listLength != NULL) {
						int j;


						for (j = 0; j < tmpl[i].listLength(payloadp); j++) {
							void *listElement = NULL;

							res = KSI_TLV_new(ctx, KSI_TLV_PAYLOAD_TLV, tmpl[i].tag, isNonCritical, isForward, &tmp);
							KSI_CATCH(&err, res) goto cleanup;

							res = tmpl[i].listElementAt(payloadp, j, &listElement);
							KSI_CATCH(&err, res) goto cleanup;

							res = construct(ctx, tmp, listElement, tmpl[i].subTemplate, buf, buf_len + len, buf_size);
							KSI_CATCH(&err, res) goto cleanup;

							res = KSI_TLV_appendNestedTlv(tlv, NULL, tmp);
							KSI_CATCH(&err, res) goto cleanup;
							tmp = NULL;
						}
					} else {
						res = KSI_TLV_new(ctx, KSI_TLV_PAYLOAD_TLV, tmpl[i].tag, isNonCritical, isForward, &tmp);
						KSI_CATCH(&err, res) goto cleanup;

						res = construct(ctx, tmp, payloadp, tmpl[i].subTemplate, buf, buf_len + len, buf_size);
						KSI_CATCH(&err, res) goto cleanup;

						res = KSI_TLV_appendNestedTlv(tlv, NULL, tmp);
						KSI_CATCH(&err, res) goto cleanup;
						tmp = NULL;
					}
					break;
				default:
					KSI_LOG_error(ctx, "Unimplemented template type: %d", tmpl[i].type);
					KSI_FAIL(&err, KSI_UNKNOWN_ERROR, "Unimplemented template type.");
					goto cleanup;
			}
		}
	}

	buf[buf_len] = '\0';

	/* Check that every mandatory component was present. */
	for (i = 0; i < template_len; i++) {
		char errm[100];
		if ((tmpl[i].flags & KSI_TLV_TMPL_FLG_MANDATORY) != 0 && !templateHit[i]) {
			KSI_snprintf(errm, sizeof(errm), "Mandatory element missing: %s->[0x%02x]%s", buf, tmpl[i].tag, tmpl[i].descr == NULL ? "" : tmpl[i].descr);
			KSI_LOG_debug(ctx, "%s", errm);
			KSI_FAIL(&err, KSI_INVALID_FORMAT, errm);
			goto cleanup;
		}
		if (((tmpl[i].flags & KSI_TLV_TMPL_FLG_LEAST_ONE_G0) != 0 && !groupHit[0]) ||
				((tmpl[i].flags & KSI_TLV_TMPL_FLG_LEAST_ONE_G1) != 0 && !groupHit[1])) {
			KSI_snprintf(errm, sizeof(errm), "Mandatory group missing: %s->[0x%02x]%s", buf, tmpl[i].tag, tmpl[i].descr == NULL ? "" : tmpl[i].descr);
			KSI_LOG_debug(ctx, "%s", errm);
			KSI_FAIL(&err, KSI_INVALID_FORMAT, errm);
			goto cleanup;
		}
	}

	KSI_SUCCESS(&err);

cleanup:

	KSI_nofree(payloadp);

	KSI_free(templateHit);
	KSI_TLV_free(tmp);

	return KSI_RETURN(&err);
}

int KSI_TlvTemplate_construct(KSI_CTX *ctx, KSI_TLV *tlv, const void *payload, const KSI_TlvTemplate *tmpl) {
	char buf[0xffff];
	buf[0] = '\0';
	return construct(ctx, tlv, payload, tmpl, buf, 0, sizeof(buf));
}

int KSI_TlvTemplate_deepCopy(KSI_CTX *ctx, const void *from, const KSI_TlvTemplate *baseTemplate, void *to) {
	KSI_ERR err;
	KSI_TLV *tmpTlv = NULL;
	int res;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, from != NULL) goto cleanup;
	KSI_PRE(&err, baseTemplate != NULL) goto cleanup;
	KSI_PRE(&err, to != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	/* Create a dummy TLV */
	res = KSI_TLV_new(ctx, KSI_TLV_PAYLOAD_TLV, 0x0, 0, 0, &tmpTlv);
	KSI_CATCH(&err, res) goto cleanup;

	/* Create a TLV object */
	res = KSI_TlvTemplate_construct(ctx, tmpTlv, from, baseTemplate);
	KSI_CATCH(&err, res) goto cleanup;

	/* Evaluate the cloned object */
	res = KSI_TlvTemplate_extract(ctx, to, tmpTlv, baseTemplate);
	KSI_CATCH(&err, res) goto cleanup;

	KSI_SUCCESS(&err);

cleanup:

	KSI_TLV_free(tmpTlv);

	return KSI_RETURN(&err);
}

int KSI_TlvTemplate_serializeObject(KSI_CTX *ctx, const void *obj, unsigned tag, int isNc, int isFwd, const KSI_TlvTemplate *tmpl, unsigned char **raw, unsigned *raw_len) {
	KSI_ERR err;
	int res;
	KSI_TLV *tlv = NULL;
	unsigned char *tmp = NULL;
	unsigned tmp_len = 0;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_PRE(&err, obj != NULL) goto cleanup;
	KSI_PRE(&err, tmpl != NULL) goto cleanup;
	KSI_PRE(&err, raw != NULL) goto cleanup;
	KSI_PRE(&err, raw_len != NULL) goto cleanup;

	KSI_BEGIN(ctx, &err);

	/* Create TLV for the PDU object. */
	res = KSI_TLV_new(ctx, KSI_TLV_PAYLOAD_TLV, tag, isFwd, isNc, &tlv);
	KSI_CATCH(&err, res) goto cleanup;

	/* Evaluate the TLV. */
	res = KSI_TlvTemplate_construct(ctx, tlv, obj, tmpl);
	KSI_CATCH(&err, res) goto cleanup;

	KSI_LOG_logTlv(ctx, KSI_LOG_DEBUG, "Serializing object", tlv);

	/* Serialize the TLV. */
	res = KSI_TLV_serialize(tlv, &tmp, &tmp_len);
	KSI_CATCH(&err, res) goto cleanup;

	*raw = tmp;
	tmp = NULL;
	*raw_len = tmp_len;

	KSI_SUCCESS(&err);

cleanup:

	KSI_free(tmp);
	KSI_TLV_free(tlv);

	return KSI_RETURN(&err);
}
