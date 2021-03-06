/*
 * Copyright 2013-2016 Guardtime, Inc.
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

#include <string.h>

#include "all_tests.h"
#include <ksi/tlv.h>
#include <ksi/tlv_template.h>
#include <ksi/io.h>
#include "../src/ksi/ctx_impl.h"
#include "../src/ksi/internal.h"

static char const * const ok_sample[] = {
		"resource/tlv/ok_int-1.tlv",
		"resource/tlv/ok_int-2.tlv",
		"resource/tlv/ok_int-3.tlv",
		"resource/tlv/ok_int-4.tlv",
		"resource/tlv/ok_int-5.tlv",
		"resource/tlv/ok_int-6.tlv",
		"resource/tlv/ok_int-7.tlv",
		"resource/tlv/ok_int-8.tlv",
		"resource/tlv/ok_int-9.tlv",
		"resource/tlv/ok_nested-1.tlv",
		"resource/tlv/ok_nested-2.tlv",
		"resource/tlv/ok_nested-3.tlv",
		"resource/tlv/ok_nested-4.tlv",
		"resource/tlv/ok_nested-5.tlv",
		"resource/tlv/ok_nested-6.tlv",
		"resource/tlv/ok_nested-7.tlv",
		"resource/tlv/ok_nested-8.tlv",
		"resource/tlv/ok_nested-9.tlv",
		"resource/tlv/ok_nested-10.tlv",
		"resource/tlv/ok_str-1.tlv",
		"resource/tlv/ok_str-2.tlv",
		"resource/tlv/ok_str-3.tlv",
		"resource/tlv/ok_str-4.tlv",
		"resource/tlv/ok_str-5.tlv",
		"resource/tlv/ok_str-6.tlv",
		"resource/tlv/ok_str-7.tlv",
		"resource/tlv/ok_oct-1.tlv",
		"resource/tlv/ok_oct-2.tlv",
		NULL
};

static char const * const nok_sample[] = {
		"resource/tlv/nok_int-1.tlv",
		"resource/tlv/nok_int-2.tlv",
		"resource/tlv/nok_int-3.tlv",
		"resource/tlv/nok_int-4.tlv",
		"resource/tlv/nok_str-1.tlv",
		NULL
};

extern KSI_CTX *ctx;

static int parseStructure(KSI_TLV *tlv, int indent) {
	int res;
	KSI_TLV *nested = NULL;
	KSI_LIST(KSI_TLV) *list = NULL;
	size_t i;
	KSI_Utf8String *utf = NULL;
	KSI_Integer *integer = NULL;
	KSI_OctetString *octet = NULL;

	switch (KSI_TLV_getTag(tlv)) {
		case 0x01:
			/* Cast as numeric TLV */
			/* Parse number */
			res = KSI_Integer_fromTlv(tlv, &integer);
			if (res != KSI_OK) goto cleanup;
			break;
		case 0x02:
			/* Cast as string TLV */
			res = KSI_Utf8String_fromTlv(tlv, &utf);
			if (res != KSI_OK) goto cleanup;
			break;
		case 0x03:
		case 0x1003:
			res = KSI_TLV_getNestedList(tlv, &list);
			if (res != KSI_OK) goto cleanup;

			/* Parse nested */
			for (i = 0; i < KSI_TLVList_length(list); i++) {
				res = KSI_TLVList_elementAt(list, i, &nested);
				if (res != KSI_OK) goto cleanup;

				if (nested == NULL) break;

				res = parseStructure(nested, indent);
				if (res != KSI_OK) goto cleanup;
			}
			break;
		case 0x04:
			/* Cast as octet string*/
			res = KSI_OctetString_fromTlv(tlv, &octet);
			if (res != KSI_OK) goto cleanup;
			break;
		default:
			res = KSI_INVALID_FORMAT;
			goto cleanup;
	}

cleanup:

	KSI_OctetString_free(octet);
	KSI_Utf8String_free(utf);
	KSI_Integer_free(integer);
	return res;
}

static void TestOkFiles(CuTest* tc) {
	int res;
	int i = 0;

	KSI_TLV *tlv = NULL;

	KSI_ERR_clearErrors(ctx);

	while (ok_sample[i] != NULL) {
		CuAssert(tc, "Unable to read valid TLV.", KSITest_tlvFromFile(getFullResourcePath(ok_sample[i++]), &tlv) == KSI_OK);

		res = parseStructure(tlv, 0);

		CuAssert(tc, "Unable to parse valid TLV.", res == KSI_OK);

		KSI_TLV_free(tlv);
		tlv = NULL;

		break;
	}
}

static void TestNokFiles(CuTest* tc) {
	int res;
	int i = 0;

	KSI_TLV *tlv = NULL;

	KSI_ERR_clearErrors(ctx);

	while (nok_sample[i] != NULL) {
		res = KSITest_tlvFromFile(getFullResourcePath(nok_sample[i++]), &tlv);

		if (res == KSI_OK) {
			res = parseStructure(tlv, 0);
		}

		CuAssert(tc, "Parser did not fail with invalid TLV.", res != KSI_OK);

		KSI_TLV_free(tlv);
		tlv = NULL;
	}
}

static void TestSerialize(CuTest* tc) {
	int res;
	KSI_TLV *tlv = NULL;

	unsigned char in[0xffff + 4];
	unsigned char out[0xffff + 4];
	char errstr[1024];

	size_t out_len;
	size_t in_len;

	FILE *f = NULL;
	int i = 0;

	KSI_ERR_clearErrors(ctx);

	while (ok_sample[i] != NULL) {
		KSI_LOG_debug(ctx, "TestSerialize: opening file '%s'.", ok_sample[i]);
		f = fopen(getFullResourcePath(ok_sample[i]), "rb");
		CuAssert(tc, "Unable to open test file.", f != NULL);

		in_len = (unsigned)fread(in, 1, sizeof(in), f);

		fclose(f);
		f = NULL;

		res = KSI_TLV_parseBlob2(ctx, in, in_len, 0, &tlv);
		CuAssert(tc, "Unable to parse TLV.", res == KSI_OK);

		res = parseStructure(tlv, 0);
		CuAssert(tc, "Unable to parse TLV structure.", res == KSI_OK);

		/* Re assemble TLV */
		KSI_TLV_serialize_ex(tlv, out, sizeof(out), &out_len);

		CuAssert(tc, "Serialized TLV size mismatch.", in_len == out_len);
		sprintf(errstr, "Serialized TLV content does not match original: %s.", ok_sample[i]);
		CuAssert(tc, errstr, !memcmp(in, out, in_len));

		KSI_TLV_free(tlv);
		tlv = NULL;
		i++;
	}
}

static void TestClone(CuTest *tc) {
	int res;
	KSI_TLV *tlv = NULL;
	KSI_TLV *clone = NULL;

	unsigned char in[0xffff + 4];
	unsigned char out1[0xffff + 4];
	char errstr[1024];

	size_t out_len;
	size_t in_len;

	FILE *f = NULL;
	int i = 0;

	KSI_ERR_clearErrors(ctx);

	while (ok_sample[i] != NULL) {
		f = fopen(getFullResourcePath(ok_sample[i]), "rb");
		CuAssert(tc, "Unable to open test file.", f != NULL);

		in_len = (unsigned) fread(in, 1, sizeof(in), f);

		fclose(f);
		f = NULL;

		res = KSI_TLV_parseBlob2(ctx, in, in_len, 0, &tlv);
		CuAssert(tc, "Unable to parse TLV.", res == KSI_OK);

		res = parseStructure(tlv, 0);
		CuAssert(tc, "Unable to parse TLV structure.", res == KSI_OK);

		res = KSI_TLV_clone(tlv, &clone);
		CuAssert(tc, "Unsable to clone TLV.", res == KSI_OK && clone != NULL);

		/* Re assemble TLV */
		res = KSI_TLV_serialize_ex(clone, out1, sizeof(out1), &out_len);
		CuAssert(tc, "Unable to serialize TLV.", res == KSI_OK);

		CuAssert(tc, "Serialized TLV size mismatch.", in_len == out_len);
		sprintf(errstr, "Serialised TLV content does not match original: '%s'.", ok_sample[i]);
		CuAssert(tc, errstr, !memcmp(in, out1, in_len));

		KSI_TLV_free(clone);
		clone = NULL;

		KSI_TLV_free(tlv);
		tlv = NULL;
		i++;
	}
}

static void testObjectSerialization(CuTest *tc, const char *sample,
									int (*parse)(KSI_CTX *, unsigned char *, size_t, void **),
									int (*serialize)(void *, unsigned char **, size_t *),
									void (*objFree)(void *)) {
	int res;
	void *pdu = NULL;
	unsigned char in[0xffff + 4];
	size_t in_len;
	unsigned char *out = NULL;
	size_t out_len;
	FILE *f = NULL;
	char errm[1024];

	f = fopen(sample, "rb");
	KSI_snprintf(errm, sizeof(errm), "Unable to open pdu file: '%s'.", sample);
	CuAssert(tc, errm, f != NULL);

	in_len = (unsigned)fread(in, 1, sizeof(in), f);
	fclose(f);
	KSI_snprintf(errm, sizeof(errm), "Unable to read pdu file: '%s'.", sample);
	CuAssert(tc, errm, in_len > 0);

	res = parse(ctx, in, in_len, &pdu);
	KSI_snprintf(errm, sizeof(errm), "Unable to parse pdu: '%s'.", sample);
	CuAssert(tc, errm, res == KSI_OK && pdu != NULL);

	res = serialize(pdu, &out, &out_len);
	KSI_snprintf(errm, sizeof(errm), "Unable to serialize pdu: '%s'.", sample);
	CuAssert(tc, errm, res == KSI_OK && out != NULL && out_len > 0);

	KSI_snprintf(errm, sizeof(errm), "Serialized pdu length mismatch: '%s'.", sample);
	CuAssert(tc, errm, res == KSI_OK && out_len == in_len);

	KSI_snprintf(errm, sizeof(errm), "Serialised pdu content mismatch: '%s'.", sample);
	CuAssert(tc, errm, !KSITest_memcmp(in, out, in_len));

	KSI_free(out);
	objFree(pdu);
}

static void aggregationPduTest(CuTest *tc) {
	ctx->options[KSI_OPT_AGGR_PDU_VER] = KSI_PDU_VERSION_1;
	testObjectSerialization(tc, getFullResourcePath("resource/tlv/v1/aggr_request.tlv"),
			(int (*)(KSI_CTX *, unsigned char *, size_t, void **))KSI_AggregationPdu_parse,
			(int (*)(void *, unsigned char **, size_t *))KSI_AggregationPdu_serialize,
			( void (*)(void *))KSI_AggregationPdu_free);
	testObjectSerialization(tc, getFullResourcePath("resource/tlv/v1/aggr_response.tlv"),
			(int (*)(KSI_CTX *, unsigned char *, size_t, void **))KSI_AggregationPdu_parse,
			(int (*)(void *, unsigned char **, size_t *))KSI_AggregationPdu_serialize,
			( void (*)(void *))KSI_AggregationPdu_free);
	ctx->options[KSI_OPT_AGGR_PDU_VER] = KSI_AGGREGATION_PDU_VERSION;
}

static void aggregationPduVer2Test(CuTest *tc) {
	ctx->options[KSI_OPT_AGGR_PDU_VER] = KSI_PDU_VERSION_2;
	testObjectSerialization(tc, getFullResourcePath("resource/tlv/v2/aggr_request.tlv"),
			(int (*)(KSI_CTX *, unsigned char *, size_t, void **))KSI_AggregationPdu_parse,
			(int (*)(void *, unsigned char **, size_t *))KSI_AggregationPdu_serialize,
			( void (*)(void *))KSI_AggregationPdu_free);
	testObjectSerialization(tc, getFullResourcePath("resource/tlv/v2/aggr_response.tlv"),
			(int (*)(KSI_CTX *, unsigned char *, size_t, void **))KSI_AggregationPdu_parse,
			(int (*)(void *, unsigned char **, size_t *))KSI_AggregationPdu_serialize,
			( void (*)(void *))KSI_AggregationPdu_free);
	ctx->options[KSI_OPT_AGGR_PDU_VER] = KSI_AGGREGATION_PDU_VERSION;
}

static void extendPduTest(CuTest *tc) {
	ctx->options[KSI_OPT_EXT_PDU_VER] = KSI_PDU_VERSION_1;
	testObjectSerialization(tc, getFullResourcePath("resource/tlv/v1/extend_request.tlv"),
			(int (*)(KSI_CTX *, unsigned char *, size_t, void **))KSI_ExtendPdu_parse,
			(int (*)(void *, unsigned char **, size_t *))KSI_ExtendPdu_serialize,
			( void (*)(void *))KSI_ExtendPdu_free);
	testObjectSerialization(tc, getFullResourcePath("resource/tlv/v1/extend_response.tlv"),
			(int (*)(KSI_CTX *, unsigned char *, size_t, void **))KSI_ExtendPdu_parse,
			(int (*)(void *, unsigned char **, size_t *))KSI_ExtendPdu_serialize,
			( void (*)(void *))KSI_ExtendPdu_free);
	ctx->options[KSI_OPT_EXT_PDU_VER] = KSI_EXTENDING_PDU_VERSION;
}

static void extendPduVer2Test(CuTest *tc) {
	ctx->options[KSI_OPT_EXT_PDU_VER] = KSI_PDU_VERSION_2;
	testObjectSerialization(tc, getFullResourcePath("resource/tlv/v2/extend_request.tlv"),
			(int (*)(KSI_CTX *, unsigned char *, size_t, void **))KSI_ExtendPdu_parse,
			(int (*)(void *, unsigned char **, size_t *))KSI_ExtendPdu_serialize,
			( void (*)(void *))KSI_ExtendPdu_free);
	testObjectSerialization(tc, getFullResourcePath("resource/tlv/v2/extend_response.tlv"),
			(int (*)(KSI_CTX *, unsigned char *, size_t, void **))KSI_ExtendPdu_parse,
			(int (*)(void *, unsigned char **, size_t *))KSI_ExtendPdu_serialize,
			( void (*)(void *))KSI_ExtendPdu_free);
	ctx->options[KSI_OPT_EXT_PDU_VER] = KSI_EXTENDING_PDU_VERSION;
}

static void testErrorMessage(CuTest* tc, const char *expected, const char *tlv_file,
		int (*obj_new)(KSI_CTX *ctx, void **),
		void (*obj_free)(void*),
		const KSI_TlvTemplate *tmplete) {
	int res;
	void *obj = NULL;
	char buf[1024];
	size_t len;
	FILE *f = NULL;
	KSI_FTLV ftlv;

	KSI_ERR_clearErrors(ctx);

	f = fopen(getFullResourcePath(tlv_file), "rb");
	CuAssert(tc, "Failed to open file.", f != NULL);

	res = KSI_FTLV_fileRead(f, (unsigned char *)buf, sizeof(buf), &len, &ftlv);
	CuAssert(tc, "Failed read from file.", res == KSI_OK);

	res = obj_new(ctx, &obj);
	CuAssert(tc, "Unable create new obj.", res == KSI_OK);

	res = KSI_TlvTemplate_parse(ctx, (unsigned char *)buf, (unsigned)len, tmplete, obj);
	CuAssert(tc, "Parsing invalid obj must fail.", res != KSI_OK);

	res = KSI_ERR_getBaseErrorMessage(ctx, buf, sizeof(buf), NULL, NULL);
	CuAssert(tc, "Unable to get base error message.", res == KSI_OK);
	CuAssert(tc, "Wrong error message.", strcmp(buf, expected) == 0);

	if (f != NULL) fclose(f);
	obj_free(obj);
}

KSI_IMPORT_TLV_TEMPLATE(KSI_AggregationPdu);

static void testUnknownCriticalTagError(CuTest* tc) {
	testErrorMessage(tc, "Unknown critical tag: [0x200]->[0x203]aggr_error_pdu->[0x01]",
			"resource/tlv/v1/tlv_unknown_tag.tlv",
			(int (*)(KSI_CTX *ctx, void **))KSI_AggregationPdu_new,
			(void (*)(void*))KSI_AggregationPdu_free,
			KSI_TLV_TEMPLATE(KSI_AggregationPdu)
			);
}

static void testMissingMandatoryTagError(CuTest* tc) {
	testErrorMessage(tc, "Mandatory element missing: [0x200]->[0x203]aggr_error_pdu->[0x4]status",
			"resource/tlv/v1/tlv_missing_tag.tlv",
			(int (*)(KSI_CTX *ctx, void **))KSI_AggregationPdu_new,
			(void (*)(void*))KSI_AggregationPdu_free,
			KSI_TLV_TEMPLATE(KSI_AggregationPdu)
			);
}

KSI_IMPORT_TLV_TEMPLATE(KSI_AggregationRespPdu);

static void testUnknownCriticalTagErrorPduVer2(CuTest* tc) {
	testErrorMessage(tc, "Unknown critical tag: [0x221]->[0x03]aggr_err->[0x01]",
			"resource/tlv/v2/tlv_unknown_tag.tlv",
			(int (*)(KSI_CTX *ctx, void **))KSI_AggregationPdu_new,
			(void (*)(void*))KSI_AggregationPdu_free,
			KSI_TLV_TEMPLATE(KSI_AggregationRespPdu)
			);
}

static void testMissingMandatoryTagErrorPduVer2(CuTest* tc) {
	testErrorMessage(tc, "Mandatory element missing: [0x221]->[0x03]aggr_err->[0x4]status",
			"resource/tlv/v2/tlv_missing_tag.tlv",
			(int (*)(KSI_CTX *ctx, void **))KSI_AggregationPdu_new,
			(void (*)(void*))KSI_AggregationPdu_free,
			KSI_TLV_TEMPLATE(KSI_AggregationRespPdu)
			);
}

CuSuite* KSITest_TLV_Sample_getSuite(void)
{
	CuSuite* suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, TestOkFiles);
	SUITE_ADD_TEST(suite, TestNokFiles);
	SUITE_ADD_TEST(suite, TestSerialize);
	SUITE_ADD_TEST(suite, TestClone);
	SUITE_ADD_TEST(suite, aggregationPduTest);
	SUITE_ADD_TEST(suite, aggregationPduVer2Test);
	SUITE_ADD_TEST(suite, extendPduTest);
	SUITE_ADD_TEST(suite, extendPduVer2Test);
	SUITE_ADD_TEST(suite, testUnknownCriticalTagError);
	SUITE_ADD_TEST(suite, testMissingMandatoryTagError);
	SUITE_ADD_TEST(suite, testUnknownCriticalTagErrorPduVer2);
	SUITE_ADD_TEST(suite, testMissingMandatoryTagErrorPduVer2);

	return suite;
}
