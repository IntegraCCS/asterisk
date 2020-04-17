/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * GUID Gen Functions
 *
 * Sebastian Gutierrez scgm11@gmail.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 *
 * 
 * 
 *
 */

/*** MODULEINFO
	<defaultenabled>yes</defaultenabled>
	<support_level>core</support_level>
 ***/

#include <asterisk.h>

#include <asterisk/module.h>
#include <asterisk/app.h>
#include <asterisk/pbx.h>
#include <asterisk/channel.h>
#include <uuid/uuid.h>

/*** DOCUMENTATION
	<application name="GUID" language="en_US">
		<synopsis>
			Simple Application to get GUIDs.
		</synopsis>
		<syntax>
		</syntax>
		<description>
		<para>This application is for retrieve GUIDs to use as Unique Identifier.</para>
		</description>
	</application>
 ***/

static char *app = "GUID";

static int getguid(struct ast_channel *chan, const char *data)
{

	// typedef unsigned char uuid_t[16];
	uuid_t uuid;

	// generate
	uuid_generate(uuid);

	// unparse (to string)
	char uuid_str[37]; // ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0"
	uuid_unparse_lower(uuid, uuid_str);

	ast_verb(3, "app_guidgen: new guid %s\n", uuid_str);

	pbx_builtin_setvar_helper(chan, "__guid", uuid_str);

	uuid_clear(uuid);

	return 0;
}

static int unload_module(void)
{

	return ast_unregister_application(app);
}

static int load_module(void)
{

	return ast_register_application_xml(app, getguid) ? AST_MODULE_LOAD_DECLINE : AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Get GUID");
