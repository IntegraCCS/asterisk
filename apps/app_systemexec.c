/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * SystemExec Functions
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


/*** DOCUMENTATION
	<application name="SystemExec" language="en_US">
		<synopsis>
			Simple Application to Exec Commands with return data.
		</synopsis>
		<syntax>
		</syntax>
		<description>
		<para>Retrieves result of the command.</para>
		</description>
	</application>
 ***/



static char *app = "SystemExec";


static int system_exec(struct ast_channel *chan, const char *data)
{
	
		char *parse;
		AST_DECLARE_APP_ARGS(args,
			AST_APP_ARG(command);
		);

	//Check for arguments
	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "SystemExec requires argument (Command String)\n");
		return -1;
	}

	parse = ast_strdupa(data);

	AST_STANDARD_APP_ARGS(args, parse);

	//Check for all needed arguments
	if (ast_strlen_zero(args.command)) {
		ast_log(LOG_WARNING, "Missing argument to SystemExec (Command String)\n");
		return -1;
	}
	

	char res[4096];	
	FILE *pipe;
	pipe = popen(args.command,"r");

	if (pipe) {

		while(!feof(pipe)) {
			if(fgets(res, 4096, pipe) != NULL){}
		}

			pclose(pipe);
			res[strlen(res)-1] = '\0';
		}


	//fgets(res,1024,fp);
	
	ast_verb( 3, "app_systemexec: result %s\n", res);
	
	pbx_builtin_setvar_helper (chan, "res", res);
	
	//pclose (fp);


	return 0;
	
}


static int unload_module (void)
{
	
	return ast_unregister_application (app);
	
}


static int load_module (void)
{

	return ast_register_application_xml(app, system_exec) ? 
		AST_MODULE_LOAD_DECLINE : AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "System Exec");
