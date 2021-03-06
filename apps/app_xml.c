/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * JSON Functions
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include <libxml/parser.h>
#include <libxml/tree.h>


/*** DOCUMENTATION
	<application name="XML" language="en_US">
		<synopsis>
			
		</synopsis>
		<syntax>
		Example:
			exten=> _XXXX,1,Set(xml=<res><body><msgtype>12</msgtype><langflag>zh_CN</langflag><engineid>1</engineid><tokensn>1000000001</tokensn><dynamicpass>111111</dynamicpass><emptyfield></emptyfield></body></res>)
			exten=> _XXXX,2,XML(${xml})
			exten=> _XXXX,3,NoOp(${msgtype})
			exten=> _XXXX,4,NoOp(${tokensn})
			exten=> _XXXX,5,NoOp(${langflag})
		</syntax>
		<description>
		<para>Retrieves data from XML Strings.</para>
		</description>
	</application>
 ***/

static void set_variables(struct ast_channel *chan, xmlNode * a_node);

//Asterisk App
static char *app = "XML";


//Asterisk execution
static int execute(struct ast_channel *chan, const char *data)
{


	char *parse;
		AST_DECLARE_APP_ARGS(args,
			AST_APP_ARG(xmlstring);
		);

	//Check for arguments
	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "XML requires arguments (XML String)\n");
		return -1;
	}

	parse = ast_strdupa(data);

	AST_STANDARD_APP_ARGS(args, parse);

	//Check for all needed arguments
	if (ast_strlen_zero(args.xmlstring)) {
		ast_log(LOG_WARNING, "Missing argument to XML (XML String)\n");
		return -1;
	}

	xmlDocPtr doc; /* the resulting document tree */
	int length = strlen(args.xmlstring);

    /*
     * The document being in memory, it have no base per RFC 2396,
     * and the "noname.xml" argument will serve as its base.
     */
    doc = xmlReadMemory(args.xmlstring, length, "noname.xml", NULL, 0);
    if (doc == NULL) {
        ast_log(LOG_WARNING,"Failed to parse document\n");

    } else {

		set_variables(chan,xmlDocGetRootElement(doc));
 		xmlFreeDoc(doc);
    }

 	xmlCleanupParser();
	return 0;

}


static void set_variables(struct ast_channel *chan, xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
    xmlChar *value = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
       
        if (cur_node->type == XML_TEXT_NODE) {
            value = xmlNodeGetContent(cur_node); 

			pbx_builtin_setvar_helper(chan, cur_node->parent->name, (char*)value);
         	ast_verb( 9,"Variable: %s Value: %s\n", cur_node->parent->name, (char*)value);
            xmlFreeFunc(value);
        }

        set_variables(chan,cur_node->children);
    }
}


//Asterisk Load and Unload Methids
static int unload_module (void)
{

	return ast_unregister_application (app);

}


static int load_module (void)
{

	return ast_register_application_xml(app, execute) ?	AST_MODULE_LOAD_DECLINE : AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "XML");
