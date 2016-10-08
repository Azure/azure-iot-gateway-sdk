// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"
#include "message.h"
#include "parson.h"

#include "identitymap.h"
#include "identitymap_hl.h"

#define MACADDR "macAddress"
#define DEVICENAME "deviceId"
#define DEVICEKEY "deviceKey"

static bool addOneRecord(VECTOR_HANDLE inputVector, JSON_Object * record)
{
	bool success;
	if (record == NULL)
	{
		/*Codes_SRS_IDMAP_HL_17_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_HL_Create shall fail and return NULL. ]*/
		success = false;
	}
	else
	{
		const char * macAddress;
		const char * deviceId;
		const char * deviceKey;
		if ((macAddress = json_object_get_string(record, MACADDR)) == NULL)
		{
			/*Codes_SRS_IDMAP_HL_17_009: [ If the array object does not contain a value named "macAddress" then IdentityMap_HL_Create shall fail and return NULL. ]*/
			LogError("Did not find expected %s configuration", MACADDR);
			success = false;
		}
		else if ((deviceId = json_object_get_string(record, DEVICENAME)) == NULL)
		{
			/*Codes_SRS_IDMAP_HL_17_010: [ If the array object does not contain a value named "deviceId" then IdentityMap_HL_Create shall fail and return NULL. ]*/
			LogError("Did not find expected %s configuration", DEVICENAME);
			success = false;
		}
		else if ((deviceKey = json_object_get_string(record, DEVICEKEY)) == NULL)
		{
			/*Codes_SRS_IDMAP_HL_17_011: [ If the array object does not contain a value named "deviceKey" then IdentityMap_HL_Create shall fail and return NULL. ]*/
			LogError("Did not find expected %s configuration", DEVICEKEY);
			success = false;
		}
		else
		{
			/*Codes_SRS_IDMAP_HL_17_012: [ IdentityMap_HL_Create shall use "macAddress", "deviceId", and "deviceKey" values as the fields for an IDENTITY_MAP_CONFIG structure and call VECTOR_push_back to add this element to the vector. ]*/
			IDENTITY_MAP_CONFIG config;
			config.macAddress = macAddress;
			config.deviceId = deviceId;
			config.deviceKey = deviceKey;
			if (VECTOR_push_back(inputVector, &config, 1) != 0)
			{
				/*Codes_SRS_IDMAP_HL_17_020: [ If pushing into the vector is not successful, then IdentityMap_HL_Create shall fail and return NULL. ]*/
				LogError("Did not push vector");
				success = false;
			}
			else
			{
				success = true;
			}
		}
	}
	return success;
}


/*
 * @brief	Create an identity map HL module.
 */
static MODULE_HANDLE IdentityMap_HL_Create(BROKER_HANDLE broker, const void* configuration)
{
	MODULE_HANDLE result;
	if ((broker == NULL) || (configuration == NULL))
	{
		/*Codes_SRS_IDMAP_HL_17_003: [ If broker is NULL then IdentityMap_HL_Create shall fail and return NULL. ]*/
		/*Codes_SRS_IDMAP_HL_17_004: [ If configuration is NULL then IdentityMap_HL_Create shall fail and return NULL. ]*/
		LogError("Invalid NULL parameter, broker=[%p] configuration=[%p]", broker, configuration);
		result = NULL;
	}
	else
	{
		/*Codes_SRS_IDMAP_HL_17_006: [ IdentityMap_HL_Create shall parse the configuration as a JSON array of objects. ]*/
		JSON_Value* json = json_parse_string((const char*)configuration);
		if (json == NULL)
		{
			/*Codes_SRS_IDMAP_HL_17_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_HL_Create shall fail and return NULL. ]*/
			LogError("Unable to parse json string");
			result = NULL;
		}
		else
		{
			/*Codes_SRS_IDMAP_HL_17_006: [ IdentityMap_HL_Create shall parse the configuration as a JSON array of objects. ]*/
			JSON_Array* jsonArray = json_value_get_array(json);
			if (jsonArray == NULL)
			{
				/*Codes_SRS_IDMAP_HL_17_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_HL_Create shall fail and return NULL. ]*/
				LogError("Expected a JSON Array in configuration");
				result = NULL;
			}
			else
			{
				/*Codes_SRS_IDMAP_HL_17_007: [ IdentityMap_HL_Create shall call VECTOR_create to make the identity map module input vector. ]*/
				VECTOR_HANDLE inputVector = VECTOR_create(sizeof(IDENTITY_MAP_CONFIG));
				if (inputVector == NULL)
				{
					/*Codes_SRS_IDMAP_HL_17_019: [ If creating the vector fails, then IdentityMap_HL_Create shall fail and return NULL. ]*/
					LogError("Failed to create the input vector");
					result = NULL;
				}
				else
				{
					size_t numberOfRecords = json_array_get_count(jsonArray);
					size_t record;
					bool arrayParsed = true;
					/*Codes_SRS_IDMAP_HL_17_008: [ IdentityMap_HL_Create shall walk through each object of the array. ]*/
					for (record = 0; record < numberOfRecords; record++)
					{
						/*Codes_SRS_IDMAP_HL_17_006: [ IdentityMap_HL_Create shall parse the configuration as a JSON array of objects. ]*/
						if (addOneRecord(inputVector, json_array_get_object(jsonArray, record)) != true)
						{
							arrayParsed = false;
							break;
						}
					}
					if (arrayParsed != true)
					{
						/*Codes_SRS_IDMAP_HL_17_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_HL_Create shall fail and return NULL. ]*/
						result = NULL;
					}
					else
					{
						MODULE_APIS apis;
						MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)(&apis);
						/*Codes_SRS_IDMAP_HL_17_013: [ IdentityMap_HL_Create shall invoke identity map module's create, passing in the message broker handle and the input vector. ]*/
						/*Codes_SRS_IDMAP_HL_17_014: [ When the lower layer identity map module create succeeds, IdentityMap_HL_Create shall succeed and return a non-NULL value. ]*/
						/*Codes_SRS_IDMAP_HL_17_015: [ If the lower layer identity map module create fails, IdentityMap_HL_Create shall fail and return NULL. ]*/
						result = apis.Module_Create(broker, inputVector);
					}
					/*Codes_SRS_IDMAP_HL_17_016: [ IdentityMap_HL_Create shall release all data it allocated. ]*/
					VECTOR_destroy(inputVector);
				}
			}
			/*Codes_SRS_IDMAP_HL_17_016: [ IdentityMap_HL_Create shall release all data it allocated. ]*/
			json_value_free(json);
		}
	}
	return result;
}

/*
* @brief	Destroy an identity map HL module.
*/
static void IdentityMap_HL_Destroy(MODULE_HANDLE moduleHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)(&apis);
	/*Codes_SRS_IDMAP_HL_17_017: [ IdentityMap_HL_Destroy shall free all used resources. ]*/
	apis.Module_Destroy(moduleHandle);
}

/*
* @brief	Start the Module.
*/
static void IdentityMap_HL_Start(MODULE_HANDLE moduleHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)(&apis);
	if (apis.Module_Start != NULL)
	{
		/*Codes_SRS_IDMAP_HL_17_021: [ IdentityMap_HL_Start shall pass the received parameters to the underlying identity map module start function, if defined. ]*/
		apis.Module_Start(moduleHandle);
	}
}

/*
 * @brief	Receive a message from the message broker.
 */
static void IdentityMap_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE)(&apis);
	/*Codes_SRS_IDMAP_HL_17_018: [ IdentityMap_HL_Receive shall pass the received parameters to the underlying identity map module receive function. ]*/
	apis.Module_Receive(moduleHandle, messageHandle);
}


/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS IdentityMap_HL_APIS_all =
{
	IdentityMap_HL_Create,
	IdentityMap_HL_Destroy,
	IdentityMap_HL_Receive,
	IdentityMap_HL_Start
};

/*Codes_SRS_IDMAP_HL_26_001: [ `Module_GetAPIS` shall fill the provided `MODULE_APIS` function with the required function pointers. ]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT void MODULE_STATIC_GETAPIS(IDENTITYMAP_MODULE_HL)(MODULE_APIS* apis)
#else
MODULE_EXPORT void Module_GetAPIS(MODULE_APIS* apis)
#endif
{
	(*apis) = IdentityMap_HL_APIS_all;
}
