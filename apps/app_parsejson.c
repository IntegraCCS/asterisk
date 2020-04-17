/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2018, Digium, Inc.
 *
 * Sebastian Gutierrez <sgutierrez@integraccs.com>
 *
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*!
 * \file
 * \brief JSON to CHANVAR Decoder
 *
 * \author Sebastian Gutierrez <sgutierrez@integraccs.com>
 *
 *
 */

//
/*** MODULEINFO
	<defaultenabled>yes</defaultenabled>
	<support_level>core</support_level>
 ***/

#include <asterisk.h>

#include <asterisk/module.h>
#include <asterisk/app.h>
#include <asterisk/pbx.h>
#include <asterisk/channel.h>

#include <jansson.h>

/*** DOCUMENTATION
	<application name="ParseJSON" language="en_US">
		<synopsis>
			
		</synopsis>
		<syntax>
		Example:
		exten=> _XXXX,1,Set(json={"Guid": 1 , "Screen": false , "Form": "Codigos" , "Campaign" : "Entrada" , "Callerid" : "${CALLERID(num)}"  , "ParAndValues" : "par1=val1-par2=val2-par3=val3" , "Beep" : "TRUE"})
		exten=> _XXXX,2,ParseJSON(${json})
		exten=> _XXXX,3,NoOp(${Guid})
		exten=> _XXXX,4,NoOp(${Form})
		exten=> _XXXX,5,NoOp(${Beep})	</syntax>
		<description>
		<para>Retrieves data from JSON Strings.</para>
		</description>
	</application>
 ***/

//Asterisk App
static char *app = "ParseJSON";

const char *json_plural(int count);
static void setVariable(struct ast_channel *chan, const char *key, json_t *value);
static int indx = 0;

const char *json_plural(int count)
{
	return count == 1 ? "" : "s";
}

static void parseInnerObject(struct ast_channel *chan, json_t *element)
{
	size_t size;
	const char *key;
	json_t *value;
	char *variablehash;

	size = json_object_size(element);

	ast_debug(1, "ParseJSON Object of %ld pair%s:\n", size, json_plural(size));

	json_object_foreach(element, key, value)
	{
		asprintf(&variablehash, "HASH(array%d,%s)", indx, key);
		setVariable(chan,variablehash,value);
	}

	free(variablehash);
}

static void parseJsonArray(struct ast_channel *chan, json_t *element) {
    size_t i;
    size_t size = json_array_size(element);
	char buf[20];

	
    ast_debug(1,"JSONParse Array of %ld element%s:\n", size, json_plural(size));
	snprintf(buf, 20, "%ld", size);
	pbx_builtin_setvar_helper(chan, "ARRAYELEMENTS", buf);

	for (i = 0; i < size; i++) {
		indx++;
		snprintf(buf, 20, "%ld", i);
        setVariable(chan, buf, json_array_get(element, i));
		
    }
}

static void setVariable(struct ast_channel *chan, const char *key, json_t *value)
{
	char buf[4096];

	switch (json_typeof(value))
	{
	case JSON_OBJECT:
		snprintf(buf, 1024, "%s", "OBJECT");
		parseInnerObject(chan, value);
		break;
	case JSON_ARRAY:
		snprintf(buf, 1024, "%s", "ARRAY");
		parseJsonArray(chan, value);
		break;
	case JSON_STRING:
		snprintf(buf, 1024, "%s", json_string_value(value));
		break;
	case JSON_INTEGER:
		snprintf(buf, 1024, "%lld", json_integer_value(value));
		break;
	case JSON_REAL:
		snprintf(buf, 1024, "%f", json_real_value(value));
		break;
	case JSON_TRUE:
		snprintf(buf, 1024, "%s", "TRUE");
		break;
	case JSON_FALSE:
		snprintf(buf, 1024, "%s", "FALSE");
		break;
	case JSON_NULL:
		snprintf(buf, 1024, "%s", "NULL");
		break;
	default:
		ast_log(LOG_ERROR, "ParseJSON unrecognized JSON type %d\n", json_typeof(value));
	}

	pbx_builtin_setvar_helper(chan, key, buf);

	ast_verb(9, "ParseJSON Variable: %s, Value: %s\n", key, buf);
}

//Asterisk execution
static int execute(struct ast_channel *chan, const char *data)
{

	char *parse;
	AST_DECLARE_APP_ARGS(args, AST_APP_ARG(jsonstring););
	indx = 0;

	//Check for arguments
	if (ast_strlen_zero(data))
	{
		ast_log(LOG_WARNING, "ParseJSON requires arguments (JSON String)\n");
		return -1;
	}

	parse = ast_strdupa(data);

	AST_STANDARD_APP_ARGS(args, parse);

	//Check for all needed arguments
	if (ast_strlen_zero(args.jsonstring))
	{
		ast_log(LOG_WARNING, "Missing argument to ParseJSON (JSON String)\n");
		return -1;
	}

	json_t *root;
	json_error_t error;

	root = json_loads(args.jsonstring, 0, &error);

	if (root)
	{

		const char *key;
		json_t *value;

		void *iter = json_object_iter(root);

		while (iter)
		{

			key = json_object_iter_key(iter);
			value = json_object_iter_value(iter);

			setVariable(chan, key, value);

			iter = json_object_iter_next(root, iter);
			
		}

		json_decref(root);
	}
	else
	{
		ast_log(LOG_ERROR, "ParseJSON error on line %d: %s\n", error.line, error.text);
	}

	return 0;
}

//Asterisk Load and Unload Methids
static int unload_module(void)
{

	return ast_unregister_application(app);
}

static int load_module(void)
{

	return ast_register_application_xml(app, execute) ? AST_MODULE_LOAD_DECLINE : AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "ParseJSON");
