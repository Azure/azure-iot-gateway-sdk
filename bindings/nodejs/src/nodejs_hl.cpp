// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"

/*because it is linked statically, this include will bring in some uniquely (by convention) named functions*/
#include "nodejs.h"
#include "nodejs_hl.h"

static MODULE_HANDLE NODEJS_HL_Create(BROKER_HANDLE broker, const void* configuration)
{
    MODULE_HANDLE result;

    /*Codes_SRS_NODEJS_HL_13_001: [ NODEJS_HL_Create shall return NULL if broker is NULL. ]*/
    /*Codes_SRS_NODEJS_HL_13_002: [ NODEJS_HL_Create shall return NULL if configuration is NULL. ]*/
    if (broker == NULL || configuration == NULL)
    {
        LogError("NULL parameter detected broker=%p configuration=%p", broker, configuration);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_NODEJS_HL_13_012: [ NODEJS_HL_Create shall parse configuration as a JSON string. ]*/
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            /*Codes_SRS_NODEJS_HL_13_003: [ NODEJS_HL_Create shall return NULL if configuration is not a valid JSON string. ]*/
            LogError("unable to json_parse_string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                /*Codes_SRS_NODEJS_HL_13_014: [ NODEJS_HL_Create shall return NULL if the configuration JSON does not start with an object at the root. ]*/
                LogError("unable to json_value_get_object");
                result = NULL;
            }
            else
            {
                /*Codes_SRS_NODEJS_HL_13_013: [ NODEJS_HL_Create shall extract the value of the main_path property from the configuration JSON. ]*/
                const char* main_path = json_object_get_string(obj, "main_path");
                if (main_path == NULL)
                {
                    /*Codes_SRS_NODEJS_HL_13_004: [ NODEJS_HL_Create shall return NULL if the configuration JSON does not contain a string property called main_path. ]*/
                    LogError("json_object_get_string failed");
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_NODEJS_HL_13_006: [ NODEJS_HL_Create shall extract the value of the args property from the configuration JSON. ]*/
                    JSON_Value* args = json_object_get_value(obj, "args"); // args is allowed to be NULL
                    char* args_str = json_serialize_to_string(args);

                    NODEJS_MODULE_CONFIG config =
                    {
                        main_path, args_str
                    };

                    /*Codes_SRS_NODEJS_HL_13_005: [ NODEJS_HL_Create shall populate a NODEJS_MODULE_CONFIG object with the values of the main_path and args properties and invoke NODEJS_Create passing the broker handle and the config object. ]*/
					MODULE_APIS apis;
					MODULE_STATIC_GETAPIS(NODEJS_MODULE)(&apis);
					result = apis.Module_Create(broker, (const void*)&config);
                    if (result == NULL)
                    {
                        /*Codes_SRS_NODEJS_HL_13_008: [ If NODEJS_Create fail then the value NULL shall be returned. ]*/
                        LogError("Unable to create Node JS module");
                        // return 'result' as-is
                    }
                    else
                    {
                        /*Codes_SRS_NODEJS_HL_13_007: [ If NODEJS_Create succeeds then a valid MODULE_HANDLE shall be returned. ]*/
                        // return 'result' as-is
                    }

                    free(args_str);
                }
            }

            json_value_free(json);
        }
    }

    return result;
}

static void NODEJS_HL_Destroy(MODULE_HANDLE module)
{
    /*Codes_SRS_NODEJS_HL_13_010: [ NODEJS_HL_Destroy shall destroy all used resources. ]*/
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(NODEJS_MODULE)(&apis);
	apis.Module_Destroy(module);
}

static void NODEJS_HL_Start(MODULE_HANDLE moduleHandle)
{
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(NODEJS_MODULE)(&apis);
	if (apis.Module_Start != NULL)
	{
		apis.Module_Start(moduleHandle);

	}
}

static void NODEJS_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_NODEJS_HL_13_009: [ NODEJS_HL_Receive shall pass the received parameters to the underlying module's _Receive function. ]*/
	MODULE_APIS apis;
	MODULE_STATIC_GETAPIS(NODEJS_MODULE)(&apis);
	apis.Module_Receive(moduleHandle, messageHandle);
}

/*
 *	Required for all modules:  the public API and the designated implementation functions.
 */
static const MODULE_APIS NODEJS_HL_APIS_all =
{
    NODEJS_HL_Create,
    NODEJS_HL_Destroy,
    NODEJS_HL_Receive,
    NODEJS_HL_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT void MODULE_STATIC_GETAPIS(NODEJS_MODULE_HL)(MODULE_APIS* apis)
#else
MODULE_EXPORT void Module_GetAPIS(MODULE_APIS* apis)
#endif
{
	if (!apis)
	{
		LogError("NULL passed to Module_GetAPIS");
	}
    else
	{
        /* Codes_SRS_NODEJS_HL_26_001: [ `Module_GetAPIS` shall fill out the provided `MODULES_API` structure with required module's APIs functions. ] */
		(*apis) = NODEJS_HL_APIS_all;
	}
}
