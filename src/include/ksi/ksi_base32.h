/*
 * $Id: base32.h 74 2010-02-22 11:42:26Z ahto.truu $
 *
 * Copyright 2008-2010 GuardTime AS
 *
 * This file is part of the GuardTime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef BASE32_H_INCLUDED
#define BASE32_H_INCLUDED

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \addtogroup util Util
 * @{
 */

/**
 * Decodes given base32 encoded data.
 * \param[in]		base32			Pointer to the base32 encoded source string.
 * \param[out]		data			Pointer to the receiving pointer.
 * \param[out]		data_len		Pointer to the raw value length variable.
 *
 * \return status code (#KSI_OK, when operation succeeded, otherwise an error code).
 * \note The caller is responsible for freeing the output memory \c data.
 */
int KSI_base32Decode(const char *base32, unsigned char **data, size_t *data_len);

/**
 * Encodes given binary data to base32.
 * \param[in]		data			Pointer to the input data.
 * \param[in]		data_len		Length of the input data.
 * \param[out]		encoded			Pointer to the receiving pointer.
 *
 * \return status code (#KSI_OK, when operation succeeded, otherwise an error code).
 * \note The caller is responsible for freeing the output memory \c encoded.
 */
int KSI_base32Encode(const unsigned char *data, size_t data_len, size_t group_len, char **encoded);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* not BASE32_H_INCLUDED */
