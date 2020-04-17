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
			exten=> _XXXX,2,ParseXML(${xml})
		</syntax>
		<description>
		<para>Retrieves data from XML Strings.</para>
		</description>
	</application>
 ***/

static void set_variables(struct ast_channel *chan, xmlNode * a_node, int indent_len);
static int is_leaf(xmlNode * node);

//Asterisk App
static char *app = "ParseXML";
static int indx = 1;
static int last_indent = 1;

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
		indx = 0;
		set_variables(chan,xmlDocGetRootElement(doc),1);
 		xmlFreeDoc(doc);
    }

 	xmlCleanupParser();
	return 0;

}

int is_leaf(xmlNode * node)
{
  xmlNode * child = node->children;
  while(child)
  {
    if(child->type == XML_ELEMENT_NODE) return 0;
 
    child = child->next;
  }
 
  return 1;
}
 
void set_variables(struct ast_channel *chan, xmlNode * node, int indent_len)
{
	char *value = NULL;
	char buf[20];
	char *variablehash;

    while(node)
    {
		
        if(node->type == XML_ELEMENT_NODE)
        {
			if (last_indent != indent_len) {
				indx++;
				last_indent = indent_len;
			}

			value = is_leaf(node)?(char *)xmlNodeGetContent(node):NULL;

			if (value == NULL && indx > 0) {
				indx--;
			}

		 	ast_verb(9,"%*c%s:%s(%d)\n", indent_len, '-', node->name, value,indx);

			
			asprintf(&variablehash, "HASH(array%d,%s)", indx, (char *)node->name);
			pbx_builtin_setvar_helper(chan, variablehash, value);
         	free(variablehash);
			
		  	
        }
        set_variables(chan,node->children, indent_len + 1);
        node = node->next;
    }

	snprintf(buf, 20, "%d", indx);
	pbx_builtin_setvar_helper(chan, "ARRAYELEMENTS", buf);
	
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
