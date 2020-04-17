/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * AMD Early Media DSP Detection
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

/*! \file
 *
 * \brief AMD for Early Media
 *
 * \author Sebastian Gutierrez <sgutierrez@integraccs.com>
 *
 * \ingroup applications
 */

/*** MODULEINFO
	<defaultenabled>yes</defaultenabled>
	<support_level>core</support_level>
 ***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision$")

#include "asterisk/file.h"
#include "asterisk/frame.h"
#include "asterisk/channel.h"
#include "asterisk/pbx.h"
#include "asterisk/module.h"
#include "asterisk/lock.h"
#include "asterisk/translate.h"
#include "asterisk/dsp.h"
#include "asterisk/utils.h"
#include "asterisk/format.h"
#include "asterisk/app.h"

/*** DOCUMENTATION
	<application name="AMDEarlyMedia" language="en_US">
		<synopsis>
			Wait for Voice on EarlyMedia via DSP.
		</synopsis>
		<syntax>
			<parameter name="timeout" required="true" />
			<parameter name="progzone" required="true" />
		</syntax>
		<description>
			<para>Returns <literal>0</literal> after waiting at least <replaceable>timeout</replaceable> seconds.
			 Returns <literal>0</literal> on success or
			<literal>-1</literal> on hangup ProgZones (us,cr,uk).</para>
		</description>
	</application>
 ***/

static char *app = "AMDEarlyMedia";

static int set_read_to_slin(struct ast_channel *chan, struct ast_format **orig_format)
{
	if (!chan || !orig_format)
	{
		return -1;
	}
	*orig_format = ao2_bump(ast_channel_readformat(chan));
	return ast_set_read_format(chan, ast_format_slin);
}

static int amdearlymedia_exec(struct ast_channel *chan, const char *data)
{
	int res = 0;

	struct ast_frame *fr = NULL;
	struct ast_frame *fr2 = NULL;
	struct ast_format *rfmt = NULL;

	struct ast_dsp *dsp = NULL;
	struct ast_format *origformat = NULL;

	int waitdur = 0;
	time_t timeout = 0;


	char *parse;
	AST_DECLARE_APP_ARGS(args,
						 AST_APP_ARG(emtimeout);
						 AST_APP_ARG(progzone);
						 );

	//Check for arguments
	if (ast_strlen_zero(data))
	{
		ast_log(LOG_WARNING, "AMDEarlyMedia requires arguments (Timeout and ProgZone)\n");
		return -1;
	}

	parse = ast_strdupa(data);

	AST_STANDARD_APP_ARGS(args, parse);

	//Check for all needed arguments
	if (ast_strlen_zero(args.emtimeout) || ast_strlen_zero(args.progzone))
	{
		ast_log(LOG_WARNING, "Missing argument to AMDEarlyMedia (Timeout or ProgZone)\n");
		return -1;
	}

	waitdur = atoi(args.emtimeout);
	pbx_builtin_setvar_helper(chan, "AMDEARLYMEDIA", "");

	res = set_read_to_slin(chan, &rfmt);
	if (res < 0)
	{
		ast_log(LOG_WARNING, "Unable to set to linear mode, giving up\n");
		ao2_cleanup(rfmt);
		return -1;
	}

	if (!(dsp = ast_dsp_new()))
	{
		ast_log(LOG_WARNING, "Unable to allocate DSP!\n");
		res = -1;
	}

	if (dsp)
	{
		ast_dsp_set_threshold(dsp, 256);
		ast_dsp_set_features(dsp, DSP_FEATURE_CALL_PROGRESS);
		ast_dsp_set_call_progress_zone(dsp,args.progzone);
	}

	if (!res)
	{
		if (waitdur > 0)
			timeout = time(NULL) + (time_t)waitdur;

		while (ast_waitfor(chan, -1) > -1)
		{
			if (waitdur > 0 && time(NULL) > timeout)
			{
				res = 0;
				pbx_builtin_setvar_helper(chan, "AMDEARLYMEDIA", "NO ANSWER");
				break;
			}

			fr = ast_read(chan);
			if (!fr)
			{
				ast_debug(3, "Got Hangup...\n");
				res = -1;
				break;
			}

			fr2 = ast_dsp_process(chan, dsp, fr);
			if (!fr2)
			{
				ast_log(LOG_WARNING, "Bad DSP received (what happened?)\n");
				fr2 = fr;
				ast_frfree(fr2);
			}

			if ((fr->frametype == AST_FRAME_VOICE) && (ast_format_slin))
			{

				if (ast_dsp_get_tstate(dsp) == DSP_TONE_STATE_RINGING)
				{
					ast_debug(3, "Got Ring...\n");
					pbx_builtin_setvar_helper(chan, "AMDEARLYMEDIA", "RINGING");
					ast_frfree(fr);
				}

				if (ast_dsp_get_tstate(dsp) == DSP_TONE_STATE_SILENCE)
				{
					ast_debug(3, "Got Silence...\n");
					pbx_builtin_setvar_helper(chan, "AMDEARLYMEDIA", "SILENCE");
					ast_frfree(fr);
				}
				

				if (ast_dsp_get_tstate(dsp) == DSP_TONE_STATE_TALKING)
				{
					ast_debug(3, "Got Voice...\n");
					pbx_builtin_setvar_helper(chan, "AMDEARLYMEDIA", "VOICE");
					ast_frfree(fr);
					break;
				}

				if (ast_dsp_get_tstate(dsp) == DSP_TONE_STATE_BUSY)
				{
					ast_debug(3, "Got Busy...\n");
					pbx_builtin_setvar_helper(chan, "AMDEARLYMEDIA", "BUSY");
					ast_frfree(fr);
					break;
				}

				if (ast_dsp_get_tstate(dsp) == DSP_TONE_STATE_HUNGUP)
				{
					ast_debug(3, "Got Hangup...\n");
					pbx_builtin_setvar_helper(chan, "AMDEARLYMEDIA", "HANGUP");
					ast_frfree(fr);
					break;
				}
				
			}
		}
	}
	else
	{
		ast_log(LOG_WARNING, "Could not answer channel ");
	}

	if (res > -1)
	{
		if (origformat && ast_set_read_format(chan, origformat))
		{
			ast_log(LOG_WARNING, "Failed to restore read format");
		}
	}

	if (dsp)
		ast_dsp_free(dsp);

	return res;
}

static int unload_module(void)
{
	return ast_unregister_application(app);
}

static int load_module(void)
{
	return ast_register_application_xml(app, amdearlymedia_exec);
}

AST_MODULE_INFO_STANDARD_EXTENDED(ASTERISK_GPL_KEY, "Waits until voice is detected or timeout");
