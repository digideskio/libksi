T2 - Verifying Tutorial
=====================

Disclaimer
----------

For simplicity reasons, the error handling in this tutorial is mostly omitted.
In practice almost all the functions in the SDK return a status code which
value should be checked to be #KSI_OK, which means all went well.

1. Preparation
---------------

For preparation see [Basics Tutorial](tutorial/t0_basics.md).

2. Parsing
----------

Usually if a signature needs to be verified, it has been serialized into a binary format
which is stored in a file or database. We won't cover reading the binary data, as it may vary
on different integrations. Let's assume the signature is copied into a buffer
called \c raw and it's length is stored in \c raw_len. To parse the signature we need to
call #KSI_Signature_parse:

~~~~~~~~~~{.c}

	KSI_Signature *sig = NULL;
	KSI_Signature_parse(ksi, raw, raw_len, &sig);

~~~~~~~~~~

After a successful call to #KSI_Signature_parse the buffer \c raw can be freed by the caller
as it is not referenced by the signature.

3. Simple verification
----------------------

For the most basic verification needs we can just call #KSI_verifySignature and check that the return code is #KSI_OK:

~~~~~~~~~~{.c}

	res = KSI_verifySignature(ksi, sig);
	if (res == KSI_OK) {
		printf("Signature successfully verified!\n");
	} else {
		printf("Signature verification failed!\n");
	}

~~~~~~~~~~

In the above example we didn't verify the signature against the original document. However we can do so by calling
#KSI_Signature_verifyDocument. The document is pointed to by \c doc and its length is given in \c doc_len:

~~~~~~~~~~{.c}

	res = KSI_Signature_verifyDocument(sig, ksi, doc, doc_len);
	if (res == KSI_OK) {
		printf("Signature and document successfully verified!\n");
	} else {
		printf("Signature and document verification failed!\n");
	}

~~~~~~~~~~

Alternatively, if we already have a document hash available in \c hsh, we can simply call #KSI_verifyDataHash:

~~~~~~~~~~{.c}

	res = KSI_verifyDataHash(ksi, sig, hsh);
	if (res == KSI_OK) {
		printf("Signature and document successfully verified!\n");
	} else {
		printf("Signature and document verification failed!\n");
	}

~~~~~~~~~~

Abovemnetioned examples in this chapter used the general verification policy implicitly (see next chapter for details) and relied on a
pre-configured publications file and extender in KSI context. If this is all we need and we are not interested in the
details of the verification result (e.g. in case of a failure), then that's all we need to know about the verification!

If however we want more control over the verification process, we can choose the verification policy explicitly and
provide relevant input data in the verification context. All of this is explained in the following chapters.

4. Policies
-----------

Signatures are verified according to one or more policies (even in the simple examples of the previous chapter). A verification
policy is a set of ordered rules that verify relevant signature properties. Verifying a signature according to a policy results
in one of three possible outcomes:
- Verification is successful, which means that there is enough data to prove that the signature is correct.
- Verification is not possible, which means that there is not enough data to prove or disprove the correctness
of the signature. Note: with some other policy it might still be possible to prove the correctness of the signature.
- Verification failed, which means that the signature is definitely invalid or the document does not match
the signature.

The SDK provides the following predefined policies for verification:
- Internal policy. This policy verifies the consistency of various internal components of the signature without
requiring any additional data from the user. The verified components are the aggregation chain, calendar chain (optional),
calendar authentication record (optional) and publication record (optional). Additionally, if a document hash is provided,
the signature is verified against it.
- User provided publication string based policy. This policy verifies the signature publication record against the
publication string. if necessary (and permitted), the signature is extended to the user publication. For conclusive results
the signature must either contain a publication record with a suitable publication or signature extending must be allowed.
A publication string must be provided and an extender must be configured.
- Publications file based policy. This policy verifies the signature publication record against a publication
in the publication file. If necessary (and permitted), the signature is extended to the publication. For conclusive results
the signature must either contain a publication record with a suitable publication or signature extending must be allowed.
A publications file must be provided for lookup and an extender must be configured. 
- Key-based policy. This policy verifies the PKI signature and calendar chain data in the calendar authentication record of the signature.
For conclusive results, a calendar hash chain and calendar authentication record must be present in the signature.
A trusted publication file must be provided for performing lookup of a matching certificate.
- Calendar-based policy. This policy first extends the signature to either the head of the calendar or to the 
same time as the calendar chain of the signature. The extended signature calendar chain is then verified against
aggregation chain of the signature. For conclusive results the extender must be configured.
- General policy. This policy uses all the previously mentioned policies in the specified order. Verification starts off
with internal verification and if successful, continues with key-based, publication-based and/or calendar-based verification,
depending on the availability of calendar chain, calendar authentication record or publication record in the signature.
The general policy tries all available verification policies until a signature correctness is proved or disproved and is thus
the recommended policy for verification unless some restriction dictates the use of a specific verification policy. 

Note: all of the policies perform internal verification as a prerequisite to the specific verification and a policy will 
never result in a success if internal verification fails.

5. Verifying a signature according to a policy
----------------------------------------------

To perform verification according to a policy, we first need to set up a verification context. To do this, we first call
#KSI_VerificationContext_init to initialize our context variable with default values. Then, by using our favorite API method,
we obtain the signature that we want to verify and assign it directly into the verification context. Let's assume that our
signature contains a publication record and we have a recent enough publication file to possibly contain the same publication. 
We obtain the publications file by using our API method of choice and assign it directly into the verification context.
For now we have set up all the required information to perform the verification, so we can verify the signature by
calling #KSI_SignatureVerifier_verify. We get two kinds of information from this function. First, the functions returns a
status code that indicates verification completeness. Under normal circumstances the return code should be #KSI_OK, meaning
that the verification process was completed without any errors (e.g. invalid parameters, out of memory errors, extender errors, etc).
Second, the function creates a verification result that contains the actual result of the verification according to our chosen policy.
As mentioned before, the result can be a success or failure if there was enough data to verify the signature or inconclusive if
there was not enough data (e.g. no publication file configured) Note: #KSI_OK alone as a positive status code does not indicate
a successful verification result (although it is a prerequisite), so we must inspect the verification result for details:

~~~~~~~~~~{.c}
	
	int res; /* The return value. */
	KSI_VerificationContext context;
	KSI_PolicyVerificationResult *result = NULL; /* Must be freed. */

	res = KSI_VerificationContext_init(&context, ksi);
	res = KSI_Signature_fromFile(ksi, getFullResourcePath("some_signature.ksig"), &context.signature);
	res = KSI_PublicationsFile_fromFile(ksi, getFullResourcePath("ksi-publications.bin"), &context.userPublicationsFile);
	/* Verify the signature according to general policy. */
	res = KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_GENERAL, &context, &result);
	if (res == KSI_OK) {
		if (result->finalResult.resultCode == KSI_VER_RES_OK)
			printf("Signature verified successfully!\n");
		} else {
			/* Error handling. Verification failed or was inconclusive. */
			/* Check result->finalResult.errorCode for error code. */
		}
	} else {
		/* Error handling. Verification not completed due to internal error. */
	}

~~~~~~~~~~
 
6. Verification context
-----------------------

The key to conclusive verification is having sufficient information set up in the verification context without assuming too much
from the signature itself. For most cases this means that we have to set up a publications file in the verification context and
configure an extender (see [Basics Tutorial](tutorial/t0_basics.md)). In some cases (see example below) we need to allow extending
as well. If we want to verify the signature against a specific publication, we can do so by setting up the publication string in the
verification context. If we want to verify the signature against a document, the document hash can be stored in the verification
context. Additionally, the verification context can be used for specifying the initial aggregation level and enabling/disabling
extending for publication-based verification.

Let's continue with another example where we want to verify the signature against a specific publication. We don't really need any
of the other verification policies, so we can use the predefined policy #KSI_VERIFICATION_POLICY_USER_PUBLICATION_BASED. For conclusive
results we need to set up the publication string in the verification context:

~~~~~~~~~~{.c}

	const char pubString[] = "AAAAAA-CUCYWA-AAOBM6-PNYLRK-EPI3VG-2PJGCF-Y5QHV3-XURLI2-GRFBK4-VHBED2-Q37QIB-UE3ENA";

	res = KSI_VerificationContext_init(&context, ksi);
	res = KSI_Signature_fromFile(ksi, getFullResourcePath("some_signature.ksig"), &context.signature);
	res = KSI_PublicationData_fromBase32(ksi, pubString, &context.userPublication);
	res = KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_USER_PUBLICATION_BASED, &context, &result);
	if (res == KSI_OK) {
		if (result->finalResult.resultCode == KSI_VER_RES_OK)
			printf("Signature verified successfully!\n");
		} else {
			/* Error handling. Verification failed or was inconclusive. */
			/* Check result->finalResult.errorCode for error code. */
		}
	} else {
		/* Error handling. Verification not completed due to internal error. */
	}

~~~~~~~~~~

If we want to rely on the key-based policy, we must set up the publications file as shown in the above examples. The only time we
don't need a publication file or string is when we perform calendar-based verification. However we do need a valid extender for the
online verification. The corresponding verification calls:

~~~~~~~~~~{.c}

	res = KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_KEY_BASED , &context, &result);
	res = KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_CALENDAR_BASED, &context, &result);

~~~~~~~~~~

For allowing extending of a signature for publication based policies, we have to enable it in the verification context. By default
the extending is not allowed, which in some situations is what we want, but in other situations can lead to inconclusive verification
results if a suitable publication is not found.
Note: if extending is allowed, a valid extender should also be configured (see [Basics Tutorial](tutorial/t0_basics.md)).

~~~~~~~~~~{.c}

	/* Any non-zero value allows extending, zero disables extending. */
	context.extendingAllowed = 1;

~~~~~~~~~~

If we need to verify a signature with an aggregation level other than the default 0, we can specify this in the
verification context. Initial aggregation level cannot be greater than 0xFF:

~~~~~~~~~~{.c}

	context.docAggrLevel = 10;

~~~~~~~~~~

For verifying the document, we need to set up the document hash in the verification context. The document hash, if set up, will be verified
as part of all predefined policies, but for the sake of a simple example we will choose the internal policy:

~~~~~~~~~~{.c}

	res = KSI_VerificationContext_init(&context, ksi);
	res = KSI_Signature_fromFile(ksi, getFullResourcePath("some_signature.ksig"), &context.signature);
	context.documentHash = document_hash;
	res = KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_INTERNAL, &context, &result);
	if (res == KSI_OK) {
		if (result->finalResult.resultCode == KSI_VER_RES_OK)
			printf("Signature verified successfully!\n");
		} else {
			/* Error handling. Verification failed or was inconclusive. */
			/* Check result->finalResult.errorCode for error code. */
		}
	} else {
		/* Error handling. Verification not completed due to internal error. */
	}
 
~~~~~~~~~~

7. Inspecting the result of verification
----------------------------------------

As mentioned before, the prerequisite of a conclusive verification result is that #KSI_SignatureVerifier_verify
returns #KSI_OK. If the return code is other than #KSI_OK, e.g. #KSI_INVALID_ARGUMENT or #KSI_OUT_OF_MEMORY, the
verification process was not completed and it is not possible to say if the signature is valid or incorrect.
If however #KSI_OK is returned, we must evaluate the \c result object, which is created by #KSI_SignatureVerifier_verify.
Only then can we say if the verification was a success or failure.

~~~~~~~~~~{.c}

	res = KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_GENERAL, &context, &result);
	if (res == KSI_OK) {
		/* Verification process was completed without errors, inspect the result. */
		switch (result->finalResult.resultCode) {
			case KSI_VER_RES_OK:
				printf("Verification successful, signature is valid.\n");
				break;
			case KSI_VER_RES_FAIL:
				printf("Verification failed, signature is not valid.\n");
				printf("Verification error code: %d\n", result->finalResult.errorCode);
				break;
			case KSI_VER_RES_NA:
				printf("Verification inconclusive, not enough data to prove or disprove signature correctness.\n");
				break;
		}
	} else {
		printf("Unable to complete verification due to an internal error: %x.\n", res);
		printf("Error description: %s\n", KSI_getErrorString(res));
	}

~~~~~~~~~~

8. Cleanup
----------

As the final step we need to free all the allocated resources. As an owner of all the resources that we set in the
verification context (signature, document hash, publication file/string), we are responsible for freeing them. 
The verification context contains temporary data that is allocated during verification and this must be freed by
calling #KSI_VerificationContext_clean. After we are done inspecting the verification result, we must
free it with #KSI_PolicyVerificationResult_free.

Note that the KSI context may be reused as much as needed (within a single thread) and must not be created every time.
It is also important to point out that the context, if freed, must be freed last.

~~~~~~~~~~{.c}

	KSI_Signature_free(context.signature);
	KSI_VerificationContext_clean(&context);
	KSI_PolicyVerificationResult_free(result);
	KSI_CTX_free(ksi); /* Must be freed last. */

~~~~~~~~~~

9. Migrating to the new verification functionality
---------------------------------------------------

The above described new policy-based verification replaces the old verification functionality. In particular, the following interfaces
are deprecated and will be removed from future SDK releases:

- int KSI_Signature_verify(KSI_Signature *sig, KSI_CTX *ctx)
- int KSI_Signature_verifyAggregated(KSI_Signature *sig, KSI_CTX *ctx, KSI_uint64_t level)
- int KSI_Signature_verifyOnline(KSI_Signature *sig, KSI_CTX *ctx)
- int KSI_Signature_verifyDataHash(KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *docHash)
- int KSI_Signature_verifyWithPublication(KSI_Signature *sig, KSI_CTX *ctx, const KSI_PublicationData *publication)
- int KSI_Signature_verifyAggregatedHash(KSI_Signature *sig, KSI_CTX *ctx, KSI_DataHash *rootHash, KSI_uint64_t rootLevel)
- int KSI_Signature_getVerificationResult(KSI_Signature *sig, const KSI_VerificationResult **info)

When replacing the usage of deprecated verification functionality, we need to choose a policy that matches that of the deprecated
functionality and then set up the relevant data in the verification context. A straightforward replacement exists for KSI_Signature_verify - 
just use #KSI_verifySignature as shown in chapter 3. We will show how to replace the remaining functionalities.
Note: for brevity, all error handling has been removed in the following examples.

~~~~~~~~~~{.c}

	/* Choose the general policy if you plan to replace one of the following: */
	/* KSI_Signature_verifyAggregated */
	/* KSI_Signature_verifyDataHash */
	/* KSI_Signature_verifyAggregatedHash */
	KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_GENERAL, &context, &result);

	/* Choose the calendar-based policy if you plan to replace: */
	/* KSI_Signature_verifyOnline */
	KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_CALENDAR_BASED, &context, &result);

	/* Choose the user provided publication based policy if you plan to replace: */
	/* KSI_Signature_verifyWithPublication */
	KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_USER_PUBLICATION_BASED, &context, &result);

~~~~~~~~~~

Depending on the chosen policy we need to set up relevant data in the verification context:

~~~~~~~~~~{.c}

	/* The signature is the only mandatory component in the verification context. */
	/* Perform these steps for all chosen policies. */
	KSI_VerificationContext_init(&context, ksi);
	context.signature = sig;

	/* Set the publications file if you plan to replace one of the following: */
	/* KSI_Signature_verifyAggregated */
	/* KSI_Signature_verifyDataHash */
	/* KSI_Signature_verifyAggregatedHash */
	context.userPublicationsFile = pubFile;

	/* Set the publication string if you plan to replace: */
	/* KSI_Signature_verifyWithPublication */
	context.userPublication = pubString;

	/* By default extending is not allowed in a publication based policy. */
	/* If needed, you can allow extending when replacing one of the following: */
	/* KSI_Signature_verifyAggregated */
	/* KSI_Signature_verifyDataHash */
	/* KSI_Signature_verifyAggregatedHash */
	/* KSI_Signature_verifyWithPublication */
	context.extendingAllowed = 1;

	/* Set the document hash if you plan to replace one of the following: */
	/* KSI_Signature_verifyDataHash */
	/* KSI_Signature_verifyAggregatedHash */
	context.documentHash = hsh;

	/* By default the initial aggregation level is 0. */
	/* If needed, set a different initial aggregation level if you plan to replace one of the following: */
	/* KSI_Signature_verifyAggregated */
	/* KSI_Signature_verifyAggregatedHash */
	context.docAggrLevel = level;

~~~~~~~~~~

The verification result is an output parameter of #KSI_SignatureVerifier_verify, so the KSI_Signature_getVerificationResult interface
is now obsolete. A typical verification result inspection could look like this:

~~~~~~~~~~{.c}

	res = KSI_SignatureVerifier_verify(KSI_VERIFICATION_POLICY_GENERAL, &context, &result);
	if (res == KSI_OK) {
		switch (result->finalResult.resultCode) {
			case KSI_VER_RES_OK:
				printf("Verification successful, signature is valid.\n");
				break;
			case KSI_VER_RES_FAIL:
				printf("Verification failed, signature is not valid.\n");
				printf("Verification error code: %d\n", result->finalResult.errorCode);
				break;
			case KSI_VER_RES_NA:
				printf("Verification inconclusive, not enough data to prove or disprove signature correctness.\n");
				break;
	} else {
		/* Error handling, policy could not be verified. */
	}

~~~~~~~~~~

As always, used resources need to be freed. See previous chapters on how to free the context and result.

10. Building your own policies
-----------------------------

If the predefined policies do not meet our needs of verification, we can build our own policies. For this we need
to put rules (implemented as verification functions) in some order that meets our verification needs. We can reuse
predefined rules or define our own rules for this purpose. Let's create a policy based on our own rules, defined in
the rules array \c customRules. Let's name our policy "CustomPolicy".

~~~~~~~~~~{.c}

	KSI_Policy *policy = NULL;	/* Must be freed later. */
	static const KSI_Rule customRules[] = {
		{KSI_RULE_TYPE_BASIC, VerifyingFunction1},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction2},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction3},
		{KSI_RULE_TYPE_BASIC, NULL}					/* Every rule array has to end with this empty rule. */
	};

	res = KSI_Policy_create(ksi, customRules, "CustomPolicy", &customPolicy);

~~~~~~~~~~

Each element in \c customRules array consists of two parts: rule type and a pointer. In our first example, we use the
basic rule type #KSI_RULE_TYPE_BASIC, which means that the second part - pointer - is a pointer to a verifying function.
When a policy is verified by #KSI_SignatureVerifier_verify, it goes through this array and checks the rule type.
If the rule type is #KSI_RULE_TYPE_BASIC, it calls the verifying function and examines the verification result of this
function. If the function returns #KSI_OK and verification result is #KSI_VER_RES_OK, it continues with the next rule
in the array and does so until it encounters the final empty rule. In this case the verification is successful.
If at some point any of the functions does not return #KSI_OK or the verification result is not #KSI_VER_RES_OK, the
verification fails and no more rules are processed. There is however one exception when the next rule is processed
and we will see this in one of the following examples. For now, let's examine the typical verifying function:

~~~~~~~~~~{.c}

	int VerifyingFunction1(KSI_VerificationContext *context, KSI_RuleVerificationResult *result) {
		int res = KSI_UNKNOWN_ERROR;

		if (context == NULL || result == NULL) {
			/* Unable to complete verification, set KSI_VER_RES_NA as result. */
			result->resultCode = KSI_VER_RES_NA;
			result->errorCode = KSI_VER_ERR_GEN_2;
			/* Return relevant error code. */
			res = KSI_INVALID_ARGUMENT;
			goto cleanup;
		}

		/* Perform some sort of verification of the signature. */
		if (!success) {
			/* Set KSI_VER_RES_FAIL as result and set appropriate error code. */
			result->resultCode = KSI_VER_RES_FAIL;
			result->errorCode = KSI_VER_ERR_CUST_1;
			/* Return KSI_OK because verification was completed. */
			res = KSI_OK;
		} else {
			/* Set KSI_VER_RES_OK as result. */
			result->resultCode = KSI_VER_RES_OK;
			result->errorCode = KSI_VER_ERR_NONE;
			/* Return KSI_OK because verification was completed. */
			res = KSI_OK;
		}

	cleanup:
		/* Perform cleanup of resources, if needed. */
		return res;
	}

~~~~~~~~~~

Important points to remember about a verifying function:
1. Regardless of verification success or failure, the return code in \c should be #KSI_OK, indicating that the function
had enough input data and was able to reach a conclusion about the correctness of the data.
2. The only case where the return code should be anything other than #KSI_OK is when the function encounters an internal
error (e.g. is not able to allocate a resource, cannot find a publications file, is not able to connect to the extender, etc.)
or cannot use the provided input data (e.g. a mandatory parameter is NULL, data is in the wrong format, etc.). A relevant error code
should then be returned to indicate the reason why verification was not completed.

Time to move on to some more complex rules. Let's say we want to provide alternative, equally conclusive paths to a verification result.
Path A means that we would have to go through a set of three rules, path B requires verification of a different set of four rules and path C
consists of yet another set of two rules. Finally, after succeeding at either path A, B, or C, we still want to run some final rule before 
deciding on the result of the policy. We can write it down like this:

~~~~~~~~~~{.c}

	KSI_Policy *complexPolicy = NULL;	/* Must be freed later. */
	static const KSI_Rule pathARules[] = {
		{KSI_RULE_TYPE_BASIC, VerifyingFunction1},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction2},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction3},
		{KSI_RULE_TYPE_BASIC, NULL}					/* Every rule array has to end with this empty rule. */
	};

	static const KSI_Rule pathBRules[] = {
		{KSI_RULE_TYPE_BASIC, VerifyingFunction4},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction5},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction6},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction7},
		{KSI_RULE_TYPE_BASIC, NULL}					/* Every rule array has to end with this empty rule. */
	};

	static const KSI_Rule pathCRules[] = {
		{KSI_RULE_TYPE_BASIC, VerifyingFunction8},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction9},
		{KSI_RULE_TYPE_BASIC, NULL}					/* Every rule array has to end with this empty rule. */
	};

	static const KSI_Rule chooseABCRule[] = {
		{KSI_RULE_TYPE_COMPOSITE_OR, pathARules},
		{KSI_RULE_TYPE_COMPOSITE_OR, pathBRules},
		{KSI_RULE_TYPE_COMPOSITE_OR, pathCRules},
		{KSI_RULE_TYPE_BASIC, NULL}					/* Every rule array has to end with this empty rule. */
	};

	static const KSI_Rule complexRules[] = {
		{KSI_RULE_TYPE_COMPOSITE_AND, chooseABCRule},
		{KSI_RULE_TYPE_BASIC, VerifyingFunction10},
		{KSI_RULE_TYPE_BASIC, NULL}					/* Every rule array has to end with this empty rule. */
	};

	res = KSI_Policy_create(ksi, complexRules, "ComplexPolicy", &complexPolicy);

~~~~~~~~~~

We introduced two new rule types here: #KSI_RULE_TYPE_COMPOSITE_OR and #KSI_RULE_TYPE_COMPOSITE_AND. Both are composite rule types,
which means that the second part of the rule - the pointer - is not a function pointer (as was the case with the basic rule type),
but instead a pointer to another array of rules. The array of rules can contain both basic and composite rules, meaning that
composite rules can be nested. As you would expect from any array of rules, the composite rule is also also verified in a linear
fashion until a rule fails or until all rules including the last one are successful.

The result of the composite rule, whether success or failure, is interpreted according to the rule type. If an OR-type rule
is successfully verified, further rules in the rule array are skipped and the whole rule of which the OR-type rule is part of,
is considered successfully verified. In our example, if \c pathARules verifies successfully, the subsequent rules \c pathBRules
and \c pathCRules are skipped and the rule \c chooseABCRule is considered successful. The analogy to an OR-statement continues,
but with a slightly different definition of failure - if an OR-type rule result is inconclusive (#KSI_VER_RES_NA), we are allowed to 
verify the the next rule in the array. However, if an OR-type rule result fails with #KSI_VER_RES_FAIL, subsequent rules are not verified
and the result of array of rules is a failure. So in our example, if \c pathARules results in #KSI_VER_RES_NA, the rule \c pathBRules
is verified. If this rule result is also inconclusive, the rule \c pathCRules is verified. If however any of those rules fail
with #KSI_VER_RES_FAIL, the rule \c chooseABCRule has also failed. 

In our example the rule \c chooseABCRule itself is a composite AND-type rule, which means that its result must be successful for
the verification to continue. So for a successful result of our \c complexPolicy, both \c chooseABCRule and \c VerifyingFunction10
must verify successfully. If an AND-type rule fails, the whole rule array of which it is part of, fails as well (no further rules
are verified). 

Let's summarize how rule results are interpreted:
1. If the return code is not #KSI_OK, the rule has failed due to some internal error. No further rules are checked and the return code
is propagated upwards to the top level rule, concluding with a failure of the policy.
2. If the return code is #KSI_OK, the rule was verified and its result must be examined.
3. A basic rule or a composite AND-type rule is considered successful if the result is #KSI_VER_RES_OK. In this case the verification
continues with the next rule on the same level.
4. A composite OR-type rule is considered successful if the result is #KSI_VER_RES_OK. Further rules on the same level are skipped and
verification continues one level higher.
5. Any rule is considered a failure if the result is #KSI_VER_RES_FAIL. No further rules are checked and the result is propagated upwards
to the top level rule, concluding with a failure of the policy.
6. A basic rule or a composite AND-type rule is considered inconclusive if the result is #KSI_VER_RES_NA. The result is propagated one
level upwards, but further rules on the same level are skipped. Verification may stop or continue depending on rule types of the
upper level rules.
7. A composite OR-type rule is considered inconclusive if the result code is #KSI_VER_RES_NA. The result is ignored and the next rule
on the same level is checked. This is the only exception where verification is guaranteed to continue even if the result is not
#KSI_VER_RES_OK.
8. The result of the last checked rule on any level is always propagated one level higher.

For an additional level of customization we can chain policies to each other to allow automatic fallback to a different verification
policy if the original policy fails. For our own policies that we have created with #KSI_Policy_create, we can set the fallback policy
pointer by calling #KSI_Policy_setFallback. If we want to use one of the predefined policies as the original policy, we first need to
clone it by calling #KSI_Policy_clone. When we have created or cloned a policy, we also need to free it after use by calling #KSI_Policy_free.

~~~~~~~~~~{.c}

	/* Chaining our own policies: customPolicy->complexPolicy */
	res = KSI_Policy_create(ksi, customRules, "CustomPolicy", &customPolicy);
	res = KSI_Policy_create(ksi, complexRules, "ComplexPolicy", &complexPolicy);
	res = KSI_Policy_setFallback(ksi, customPolicy, complexPolicy);
	/* Set up the verification context. */
	res = KSI_SignatureVerifier_verify(customPolicy, &context, &result);
	/* Error handling. */
	KSI_Policy_free(customPolicy);
	KSI_Policy_free(complexPolicy);

	/* Chaining predefined policies: key based -> publications file based */
	res = KSI_Policy_clone(ksi, KSI_VERIFICATION_POLICY_KEY_BASED, &clonedPolicy);
	res = KSI_Policy_setFallback(ksi, clonedPolicy, KSI_VERIFICATION_POLICY_PUBLICATIONS_FILE_BASED);
	/* Set up the verification context. */
	res = KSI_SignatureVerifier_verify(clonedPolicy, &context, &result);
	/* Error handling. */
	KSI_Policy_free(clonedPolicy);

~~~~~~~~~~