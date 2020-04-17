/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * VirtualHold
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

#include <asterisk.h>

#include <asterisk/module.h>
#include <asterisk/app.h>
#include <asterisk/pbx.h>
#include <asterisk/channel.h>





static char *app = "VirtualHold";





static int generate_virtual(struct ast_channel *chan, const char *data)
{
	char *parse;
	AST_DECLARE_APP_ARGS(args,
		AST_APP_ARG(Channel);
		AST_APP_ARG(ID);
		AST_APP_ARG(Context);
		AST_APP_ARG(Extension);
		AST_APP_ARG(Priority);
		AST_APP_ARG(CIDText);
		AST_APP_ARG(Schedule);
	);
	
	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "VirtualHold requires arguments (Channel,ID,Context,Extension,Priority,CIDText,[Schedule])\n");
		return -1;
	}
	
	parse = ast_strdupa(data);

	AST_STANDARD_APP_ARGS(args, parse);

	if (ast_strlen_zero(args.Channel) || ast_strlen_zero(args.ID) || ast_strlen_zero(args.Context)|| ast_strlen_zero(args.Extension)|| ast_strlen_zero(args.Priority) || ast_strlen_zero(args.CIDText)) {
		ast_log(LOG_WARNING, "Missing argument to VirtualHold (Channel,ID,Context,Extension,Priority,CIDText)\n");
		return -1;
	}
	
	FILE * pFile;
	char * path = "/tmp/"; 
	char * filename = args.ID;
	char * ext = ".call";
	char * when = args.Schedule;
	char * space = " ";
	char * cmdmv = "mv ";
	char * cmdt = "touch -t ";
	char * spool = "/var/spool/asterisk/outgoing/";
	
    //Generate Call File
	char * fullpath = malloc(strlen(path) + strlen(filename) + strlen(ext) + 1);
      if ( fullpath != NULL )
      {
         strcpy(fullpath, path);
         strcat(fullpath, filename);
		 strcat(fullpath, ext);
         puts(fullpath);
         
      }
	    
	pFile = fopen (fullpath,"w");
	if (pFile!=NULL)
	{
		fputs ("Channel: ",pFile);
		fputs (args.Channel,pFile); //example: Local/VirtualHold@integra-in
		fputs ("\n",pFile);
		fputs ("CallerID: VH ",pFile);
		fputs (args.CIDText,pFile);
		fputs (" <",pFile);
		fputs (args.Extension,pFile); // VirtualHold <RealCallerID>
		fputs (">\n",pFile);
		fputs ("MaxRetries: 3",pFile);
		fputs (">\n",pFile);
		fputs ("RetryTime: 60",pFile);
		fputs ("\n",pFile);
		fputs ("WaitTime: 30000",pFile);
		fputs ("\n",pFile);
		fputs ("Context: ",pFile);
		fputs (args.Context,pFile); // Context to fire at agent answer
		fputs ("\n",pFile);
		fputs ("Extension: ",pFile);
		fputs (args.Extension,pFile); // Extension to fire at agent answer
		fputs ("\n",pFile);
		fputs ("Priority: ",pFile);
		fputs (args.Priority,pFile); // Extension to fire at agent answer
		fputs ("\n",pFile);

		fclose (pFile);
	}
   
     
	//Touch Call File if necesary (Schedule call)
	if (!ast_strlen_zero(args.Schedule))
	{
	char * commandt = malloc(strlen(cmdt) + strlen(when) + strlen(space) + strlen(fullpath) + 1);
	 if ( commandt != NULL )
      {
         strcpy(commandt, cmdt);
         strcat(commandt, when);
		 strcat(commandt, space);
		 strcat(commandt, fullpath);
         puts(commandt);
         
      }
	  
		FILE *fpt;
		fpt = popen(commandt,"w");
		pclose (fpt);
		free(commandt);
	}
	
	
	//Move Call File
	 char * commandmv = malloc(strlen(cmdmv) + strlen(fullpath) + strlen(space) + strlen(spool) + strlen(filename) + strlen(ext) + 1);
	if ( commandmv != NULL )
      {
         strcpy(commandmv, cmdmv);
         strcat(commandmv, fullpath);
		 strcat(commandmv, space);
		 strcat(commandmv, spool);
		 strcat(commandmv, filename);
		 strcat(commandmv, ext);
         puts(commandmv);
         
      }
	  
	
	FILE *fp;
	fp = popen(commandmv,"r");
	pclose (fp);
	free(commandmv);
	
	
	free(fullpath);
	
	

	return 0;
	
}



static int unload_module (void)
{

	return ast_unregister_application (app);
	
}


static int load_module (void)
{

	return ast_register_application_xml(app, generate_virtual) ? 
		AST_MODULE_LOAD_DECLINE : AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "VirtualHold");
