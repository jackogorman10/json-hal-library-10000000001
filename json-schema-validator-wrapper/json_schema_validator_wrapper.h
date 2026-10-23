/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#ifndef _JSON_VALIDATIOR_H
#define _JSON_VALIDATIOR_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Enum to store error codes. */
typedef enum _ERROR_TYPE_
{
    VALIDATOR_RETURN_OK = 0,
    VALIDATOR_RETURN_ERR,
    VALIDATOR_RETURN_INVALID_ARGUMENTS,
    VALIDATOR_RETURN_MEM_ALLOC_FAILED
}error_type_t;

/**
 * @brief - Initialise json schema validator. This API will
 * initialise the json_validator object for the validation.
 * @param[in] json_schema_path Path to the JSON schema to load.
 * @return VALIDATOR_RETURN_OK on success; a validator error code on failure.
 */
int json_validator_init(const char *json_schema_path);

/**
 * @brief - Delete the json_validator instance.
 * @return VALIDATOR_RETURN_OK on success; a validator error code on failure.
 */
int json_validator_terminate();

/**
 * @brief - Validate the json sample given against the loaded schema. This string
 * will converted into json format and do the validation.
 * @param[in] json_string Serialized JSON message to validate.
 * @return VALIDATOR_RETURN_OK when validation succeeds;
 * VALIDATOR_RETURN_INVALID_ARGUMENTS for a null input; VALIDATOR_RETURN_ERR
 * for a reported schema validation failure.
 * @warning Initialize successfully before calling. JSON parsing and validation
 * exceptions are not caught by the implementation; malformed JSON is not
 * guaranteed to produce a return code.
 */
int json_validator_validate_request(const char *json_string);

#ifdef __cplusplus
}
#endif

#endif //_JSON_VALIDATIOR_H
