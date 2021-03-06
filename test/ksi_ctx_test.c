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

#include "cutest/CuTest.h"
#include <string.h>
#include "all_tests.h"
#include "../src/ksi/internal.h"
#include "../src/ksi/ctx_impl.h"

static int mockInitCount = 0;

static int mock_init(void) {
	mockInitCount++;
	return KSI_OK;
}

static void mock_cleanup(void) {
	mockInitCount--;
}

static void TestCtxInit(CuTest* tc) {
	int res = KSI_UNKNOWN_ERROR;

	KSI_CTX *ctx = NULL;
	res = KSITest_CTX_clone(&ctx);
	CuAssert(tc, "KSI_CTX_init did not return KSI_OK", res == KSI_OK);
	CuAssert(tc, "Context is NULL.", ctx != NULL);

	KSI_CTX_free(ctx);
}

static void TestRegisterGlobals(CuTest *tc) {
	int res;
	KSI_CTX *ctx = NULL;

	res = KSITest_CTX_clone(&ctx);
	CuAssert(tc, "Unable to create KSI context.", res == KSI_OK && ctx != NULL);

	res = KSI_CTX_registerGlobals(ctx, mock_init, mock_cleanup);
	CuAssert(tc, "Unable to register globals.", res == KSI_OK);

	res = KSI_CTX_registerGlobals(ctx, mock_init, mock_cleanup);
	CuAssert(tc, "Unable to register globals the 2nd time.", res == KSI_OK);

	CuAssert(tc, "Global init called wrong number of times", mockInitCount == 1);

	KSI_CTX_free(ctx);

	CuAssert(tc, "Globals not propperly cleaned up.", mockInitCount == 0);
}

static void TestErrorsToString(CuTest *tc) {
	int res;
	KSI_CTX *ctx = NULL;
	int error = KSI_INVALID_FORMAT;
	int line1;
	int line2;
	int line3;
	int line4;
	char expected_output[250];
	char *ret = NULL;
	char buf[250];


	res = KSITest_CTX_clone(&ctx);
	CuAssert(tc, "Unable to create KSI context.", res == KSI_OK && ctx != NULL);

	KSI_pushError(ctx, error, "Error: test1."); line1 = __LINE__;
	KSI_pushError(ctx, error, "Error: test2."); line2 = __LINE__;
	KSI_pushError(ctx, error, "Error: test3."); line3 = __LINE__;
	KSI_pushError(ctx, error, "Error: test4."); line4 = __LINE__;

	KSI_snprintf(expected_output, sizeof(expected_output),
		"KSI error trace:\n"
		"    4) %s:%u - (%d/0) Error: test4.\n"
		"    3) %s:%u - (%d/0) Error: test3.\n"
		"    2) %s:%u - (%d/0) Error: test2.\n"
		"    1) %s:%u - (%d/0) Error: test1.\n",
			__FILE__, line4, error,
			__FILE__, line3, error,
			__FILE__, line2, error,
			__FILE__, line1, error);

	ret = KSI_ERR_toString(ctx, buf, 0);
	CuAssert(tc, "ERR to string with buf_len = 0, must return NULL.", ret == NULL);

	ret = KSI_ERR_toString(ctx, NULL, 0);
	CuAssert(tc, "ERR to string with buf == NULL, must return NULL.", ret == NULL);

	ret = KSI_ERR_toString(ctx, buf, 2);
	CuAssert(tc, "Invalid output.", ret == buf && strcmp(buf, "K") == 0);

	ret = KSI_ERR_toString(ctx, buf, sizeof(buf));
	CuAssert(tc, "Invalid output.", ret == buf && strcmp(buf, expected_output) == 0);

	KSI_CTX_free(ctx);
}

static void TestGetBaseError(CuTest *tc) {
	int res;
	KSI_CTX *ctx = NULL;
	int ksi_error = -1;
	int ext_error = -1;
	char buf[1024];


	res = KSITest_CTX_clone(&ctx);
	CuAssert(tc, "Unable to create KSI context.", res == KSI_OK && ctx != NULL);

	res = KSI_ERR_getBaseErrorMessage(ctx, buf, sizeof(buf) ,&ksi_error, &ext_error);
	CuAssert(tc, "Unable to get base error.", res == KSI_OK && strcmp(buf, KSI_getErrorString(KSI_OK)) == 0 && ksi_error == KSI_OK && ext_error == 0);

	KSI_ERR_push(ctx, KSI_INVALID_ARGUMENT, 10, __FILE__, __LINE__, "Error: test A.");
	KSI_ERR_push(ctx, KSI_UNKNOWN_ERROR, 11, __FILE__, __LINE__, "Error: test B.");

	res = KSI_ERR_getBaseErrorMessage(ctx, buf, sizeof(buf), NULL, NULL);
	CuAssert(tc, "Unable to get correct error data.", res == KSI_OK && strcmp(buf, "Error: test A.") == 0);

	res = KSI_ERR_getBaseErrorMessage(ctx, buf, sizeof(buf), &ksi_error, NULL);
	CuAssert(tc, "Unable to get correct error data.", res == KSI_OK && strcmp(buf, "Error: test A.") == 0 && ksi_error == KSI_INVALID_ARGUMENT);

	res = KSI_ERR_getBaseErrorMessage(ctx, buf, sizeof(buf), NULL, &ext_error);
	CuAssert(tc, "Unable to get correct error data.", res == KSI_OK && strcmp(buf, "Error: test A.") == 0 && ext_error == 10);

	res = KSI_ERR_getBaseErrorMessage(ctx, buf, sizeof(buf), &ksi_error, &ext_error);
	CuAssert(tc, "Unable to get correct error data.", res == KSI_OK && strcmp(buf, "Error: test A.") == 0
			&& ext_error == 10 && ksi_error == KSI_INVALID_ARGUMENT);


	KSI_CTX_free(ctx);
}

static void TestCtxOptions_pduVersion(CuTest *tc) {
	int res;
	KSI_CTX *ctx = NULL;

	res = KSITest_CTX_clone(&ctx);
	CuAssert(tc, "Unable to create KSI context.", res == KSI_OK && ctx != NULL);
	CuAssert(tc, "Default aggregation PDU version not set correctly.", ctx->options[KSI_OPT_AGGR_PDU_VER] == KSI_AGGREGATION_PDU_VERSION);
	CuAssert(tc, "Default extending PDU version not set correctly.", ctx->options[KSI_OPT_EXT_PDU_VER] == KSI_EXTENDING_PDU_VERSION);

	res = KSI_CTX_setOption(NULL, KSI_OPT_AGGR_PDU_VER, (void*)KSI_PDU_VERSION_1);
	CuAssert(tc, "Context NULL accepted.", res == KSI_INVALID_ARGUMENT);

	res = KSI_CTX_setOption(ctx, __KSI_NUMBER_OF_OPTIONS , (void*)KSI_PDU_VERSION_1);
	CuAssert(tc, "Flag value outside boundary accepted.", res == KSI_INVALID_ARGUMENT);

	res = KSI_CTX_setOption(ctx, KSI_OPT_AGGR_PDU_VER, (void*)(KSI_AGGREGATION_PDU_VERSION == KSI_PDU_VERSION_1 ? KSI_PDU_VERSION_2 : KSI_PDU_VERSION_1));
	CuAssert(tc, "Unable to set aggregation PDU version.", res == KSI_OK && ctx->options[KSI_OPT_AGGR_PDU_VER] != KSI_AGGREGATION_PDU_VERSION);

	res = KSI_CTX_setOption(ctx, KSI_OPT_EXT_PDU_VER, (void*)(KSI_EXTENDING_PDU_VERSION == KSI_PDU_VERSION_1 ? KSI_PDU_VERSION_2 : KSI_PDU_VERSION_1));
	CuAssert(tc, "Unable to set extending PDU version.", res == KSI_OK && ctx->options[KSI_OPT_EXT_PDU_VER] != KSI_EXTENDING_PDU_VERSION);

	res = KSI_CTX_setOption(ctx, KSI_OPT_AGGR_PDU_VER, (void*)KSI_AGGREGATION_PDU_VERSION);
	CuAssert(tc, "Unable to set aggregation PDU version.", res == KSI_OK && ctx->options[KSI_OPT_AGGR_PDU_VER] == KSI_AGGREGATION_PDU_VERSION);

	res = KSI_CTX_setOption(ctx, KSI_OPT_EXT_PDU_VER, (void*)KSI_EXTENDING_PDU_VERSION);
	CuAssert(tc, "Unable to set extending PDU version.", res == KSI_OK && ctx->options[KSI_OPT_EXT_PDU_VER] == KSI_EXTENDING_PDU_VERSION);

	KSI_CTX_free(ctx);
}

static void TestCtxOptions_hmacAlgorithm(CuTest *tc) {
	int res;
	KSI_CTX *ctx = NULL;

	res = KSITest_CTX_clone(&ctx);
	CuAssert(tc, "Unable to create KSI context.", res == KSI_OK && ctx != NULL);

	CuAssert(tc, "Default aggregator HMAC algorithm.", (KSI_HashAlgorithm)ctx->options[KSI_OPT_AGGR_HMAC_ALGORITHM] == KSI_getHashAlgorithmByName("default"));
	CuAssert(tc, "Default extender HMAC algorithm.", (KSI_HashAlgorithm)ctx->options[KSI_OPT_EXT_HMAC_ALGORITHM] == KSI_getHashAlgorithmByName("default"));

	res = KSI_CTX_setOption(ctx, KSI_OPT_AGGR_HMAC_ALGORITHM, (void*)KSI_HASHALG_SHA2_384);
	CuAssert(tc, "Unable to set aggregation HMAC algorithm.", res == KSI_OK && (KSI_HashAlgorithm)ctx->options[KSI_OPT_AGGR_HMAC_ALGORITHM] == KSI_HASHALG_SHA2_384);

	res = KSI_CTX_setOption(ctx, KSI_OPT_EXT_HMAC_ALGORITHM, (void*)KSI_HASHALG_SHA2_512);
	CuAssert(tc, "Unable to set extending HMAC algorithm.", res == KSI_OK && (KSI_HashAlgorithm)ctx->options[KSI_OPT_EXT_HMAC_ALGORITHM] == KSI_HASHALG_SHA2_512);

	res = KSI_CTX_setAggregatorHmacAlgorithm(ctx, KSI_HASHALG_SHA2_512);
	CuAssert(tc, "Unable to set aggregation HMAC algorithm.", res == KSI_OK && (KSI_HashAlgorithm)ctx->options[KSI_OPT_AGGR_HMAC_ALGORITHM] == KSI_HASHALG_SHA2_512);

	res = KSI_CTX_setExtenderHmacAlgorithm(ctx, KSI_HASHALG_SHA2_384);
	CuAssert(tc, "Unable to set extending HMAC algorithm.", res == KSI_OK && (KSI_HashAlgorithm)ctx->options[KSI_OPT_EXT_HMAC_ALGORITHM] == KSI_HASHALG_SHA2_384);

	KSI_CTX_free(ctx);
}

CuSuite* KSITest_CTX_getSuite(void)
{
	CuSuite* suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, TestCtxInit);
	SUITE_ADD_TEST(suite, TestRegisterGlobals);
	SUITE_ADD_TEST(suite, TestErrorsToString);
	SUITE_ADD_TEST(suite, TestGetBaseError);
	SUITE_ADD_TEST(suite, TestCtxOptions_pduVersion);
	SUITE_ADD_TEST(suite, TestCtxOptions_hmacAlgorithm);

	return suite;
}
