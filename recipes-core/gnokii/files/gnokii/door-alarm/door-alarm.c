/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 *
 * Copyright (C) 2010 The Android Open Source Project
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

#include "config.h"
#include "compat.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


struct gn_statemachine;

#include <gnokii.h>
#include <gnokii/error.h>
#include <gnokii/common.h>
#include <gnokii/data.h>
#include <gnokii/encoding.h>
#include <gnokii/sms.h>

#include "database.h"
#include "gsmhelper.h"

static FILE *g_logfile = NULL;
static char *g_configfile = NULL;
static char *g_configmodel = NULL;

static struct gn_statemachine *g_state;
static gn_data *g_data;

static void busterminate(void)
{
    LOGMSG(LOG_LEVEL_DEBUG, "Terminate bus now.\n");
	gn_lib_phone_close(g_state);
	gn_lib_phoneprofile_free(&g_state);
	if (g_logfile)
		fclose(g_logfile);
	gn_lib_library_free();
}

static int businit(void)
{
	gn_error err;
	if ((err = gn_lib_phoneprofile_load_from_file(g_configfile, g_configmodel, &g_state)) != GN_ERR_NONE) {
		fprintf(stderr, "%s\n", gn_error_print(err));
		if (g_configfile)
			fprintf(stderr, _("File: %s\n"), g_configfile);
		if (g_configmodel)
			fprintf(stderr, _("Phone section: [phone_%s]\n"), g_configmodel);
		return 2;
	}

	/* register cleanup function */
	atexit(busterminate);
	/* signal(SIGINT, bussignal); */

	//if (install_log_handler()) 
    {
		fprintf(stderr, _("WARNING: cannot open g_logfile, logs will be directed to stderr\n"));
	}

	if ((err = gn_lib_phone_open(g_state)) != GN_ERR_NONE) {
		fprintf(stderr, "%s\n", gn_error_print(err));
		return 2;
	}
	g_data = &g_state->sm_data;
	return 0;
}

void usage()
{
    fprintf(stderr, "Usage:\n\tdoor-alarm-sms <configure file name> [<phone book folder>]\n");
    _exit(1);
}

volatile bool bshutdown = false;

/* SIGINT signal handler. */
void interrupted(int sig)
{
	signal(sig, SIG_IGN);
	bshutdown = true;
}

/*----------------------------------------------------------------------------------------------------
 * 
 * SMS command processing functions
 *
 *----------------------------------------------------------------------------------------------------
 */
const char *g_uid = "882909";
const char *g_cmd_list[] = {"dk", "huy", "kt", "bat", "tat" };
#define ARRAY_SIZE(array) sizeof(array)/sizeof(array[0])


//List of process command
static gn_error dk_command(char *remoteNo, char *msg, struct gn_statemachine *state)
{
    /*The message must include these info:
      + Register/Unregister/Check phone no.
Syntax: <Device ID> <DK|HUY|KT> <PHONE NO> [<name>]
Example: 
882909 dk 0989892766 Bao Ve -> register phone 0989892766 to receive alarm. This is phone no. of Bao Ve -> response ok
*/
    char phoneNo[32];
    char name[32] = "unname";
    int p = 0;
    char smsBuffer[256]; //Maximum buffer is 256 characters

    LOGMSG(LOG_LEVEL_INFO, "Process dk command\n");
    char *s = &msg[2];
    while(*s == ' ') {
        s++;
    }
    while(*s != ' ' && *s != '\0' && p < (sizeof(phoneNo) - 1)) {
        phoneNo[p++] = *s++;
    }
    phoneNo[p] = '\0';
    while(*s == ' ') {
        s++;
    }
    if (strlen(s) > 0) {
        strncpy(name, s, sizeof(name) - 1);
    }
    if (!validate_phone_no(phoneNo)) {
        LOGMSG(LOG_LEVEL_WARN, "Phone No: %s is not valid\n", phoneNo);
        return GN_ERR_NONE;
    }
    if (db_register_phone(phoneNo, name) != GN_ERR_NONE) {
        LOGMSG(LOG_LEVEL_ERROR, "Register phone no. %s fail.\n", phoneNo);
        return GN_ERR_FAILED;
    }
    LOGMSG(LOG_LEVEL_INFO, "\tRegister Phone No: %s\n\tName: %s\n", phoneNo, name);
    snprintf(smsBuffer, sizeof(smsBuffer), "DK thanh cong so phone: %s, ten: %s", phoneNo, name);
    gn_data *data = &state->sm_data;
    if (send_sms(smsBuffer, remoteNo, data, state) != GN_ERR_NONE) {
        LOGMSG(LOG_LEVEL_ERROR, "Could not reply sms for DK command. Out of MONEY ?Need to top-up ?\n");
        return GN_ERR_FAILED;
    }
    return GN_ERR_NONE;
}

static gn_error huy_command(char *remoteNo, char *msg, struct gn_statemachine *state)
{
    /*The message must include these info:
      + Register/Unregister/Check phone no.
        Syntax: <Device ID> <DK|HUY|KT> <PHONE NO> [<name>]
        Example: 
         882909 huy 0989892766       -> unregister phone 0989892766 from receiving alarm -> response ok ?
         882909 huy het              -> unregister all -> reponse: da huy het. NOTE: default the owner can not be removed.
      NOTE: all registered phone must be stored and encrypted. -> encrypted file system | RC4 encrypted ?
    */

    LOGMSG(LOG_LEVEL_INFO, "Process huy command\n");
    return GN_ERR_NONE;
}

static gn_error kt_command(char *remoteNo, char *msg, struct gn_statemachine *state)
{
    /*The message must include these info:
      + Register/Unregister/Check phone no.
        Syntax: <Device ID> <DK|HUY|KT> <PHONE NO> [<name>]
        Example: 
         892890 kt                   -> send back sms including register phone number: all registered phone
      NOTE: all registered phone must be stored and encrypted. -> encrypted file system | RC4 encrypted ?
    */

    LOGMSG(LOG_LEVEL_INFO, "Process kt command\n");
    return GN_ERR_NONE;
}

static gn_error bat_command(char *remoteNo, char *msg, struct gn_statemachine *state)
{
    /*The message must include these info:
      + Enable/Disable monitoring
        Synctax: <Device ID> <TAT|BAT> -> reponse "he thong <bat|tat> thanh cong>

      NOTE: all registered phone must be stored and encrypted. -> encrypted file system | RC4 encrypted ?
    */

    LOGMSG(LOG_LEVEL_INFO, "Process bat command\n");
    return GN_ERR_NONE;
}

static gn_error tat_command(char *remoteNo, char *msg, struct gn_statemachine *state)
{
    /*The message must include these info:
      + Enable/Disable monitoring
        Synctax: <Device ID> <TAT|BAT> -> reponse "he thong <bat|tat> thanh cong>

      NOTE: all registered phone must be stored and encrypted. -> encrypted file system | RC4 encrypted ?
    */
    LOGMSG(LOG_LEVEL_INFO, "Process tat command\n");
    return GN_ERR_NONE;
}

typedef gn_error (*sms_cmd_processing_t)(char*, char*, struct gn_statemachine *);
typedef struct sms_command_ {
    const char *cmdName;
    sms_cmd_processing_t cmdProcess;
} sms_command_t;
static sms_command_t g_sms_command_list[] = {
    {"dk", dk_command },
    {"huy", huy_command },
    {"kt", kt_command},
    {"bat", bat_command},
    {"tat", tat_command}
};

/*
 * End of SMS command processing functions
 */


/*----------------------------------------------------------------------------------------------------
 * 
 * State machine functions
 *
 *----------------------------------------------------------------------------------------------------
 */
static gn_error post_process_sms(gn_sms *message, struct gn_statemachine *state, void *callback_data)
{
    char *s = message->user_data[0].u.text;
    int msgId = message->number;
    char *phoneNum = message->remote.number;

    /*The message must include these info:
      + Register/Unregister/Check phone no.
        Syntax: <Device ID> <DK|HUY|KT> <PHONE NO> [<name>]
        Example: 
         882909 dk 0989892766 Bao Ve -> register phone 0989892766 to receive alarm. This is phone no. of Bao Ve -> response ok
         882909 huy 0989892766       -> unregister phone 0989892766 from receiving alarm -> response ok ?
         882909 huy het              -> unregister all -> reponse: da huy het. NOTE: default the owner can not be removed.
         892890 kt                   -> send back sms including register phone number: all registered phone

      + Enable/Disable monitoring
        Synctax: <Device ID> <TAT|BAT> -> reponse "he thong <bat|tat> thanh cong>

      NOTE: all registered phone must be stored and encrypted. -> encrypted file system | RC4 encrypted ?
    */
    //Parse command

    //strip white-space
    while(*s == ' ') {
        s++;
    }
    //First check if phoneNum is allowed to configurate ?
    int uid_len = strlen(g_uid);
    //TODO: g_uid will be read from ROM file
    if (strncmp(g_uid, s, uid_len) || s[uid_len] != ' ') {
        LOGMSG(LOG_LEVEL_INFO, "Not command sms message. Ignore\n");
        goto out;
    }
    if (strlen(s) < (uid_len + 3)) {
        LOGMSG(LOG_LEVEL_WARN, "CMD too short\n");
        goto out;
    }
    //Lowercase all
    int i1;
    for (i1 = 0; i1 < strlen(s); i1++) {
        s[i1] = tolower(s[i1]);
    }
    int cmdId;
    s += uid_len + 1;
    for (cmdId = 0; cmdId < ARRAY_SIZE(g_sms_command_list); cmdId++) {
        if (!strncmp(s, g_sms_command_list[cmdId].cmdName, strlen(g_sms_command_list[cmdId].cmdName))) {
            g_sms_command_list[cmdId].cmdProcess(phoneNum, s, state);
            goto out;
        }
    }
    LOGMSG(LOG_LEVEL_WARN, "Unknown sms command: %s \n", s);
out:
    if (msgId >= 50) {
        LOGMSG(LOG_LEVEL_WARN, "Too many message (%d), empty inbox now!\n", msgId);
        //TODO: empty inbox
    }
    return GN_ERR_NONE;
}

/* SMS handler for --smsreader mode */
static gn_error smsslave(gn_sms *message, struct gn_statemachine *state, void *callback_data)
{
	FILE *output;
	char *s = message->user_data[0].u.text;
	char buf[10240];
	int i = message->number;
	int i1, i2, msgno, msgpart;
	static int unknown = 0;
	char c, number[GN_BCD_STRING_MAX_LENGTH];
	char *p = message->remote.number;
	const char *smsdir = "/tmp/sms";

	if (p[0] == '+') {
		p++;
	}
	snprintf(number, sizeof(number), "%s", p);
	fprintf(stderr, _("SMS received from number: %s\n"), number);

	/* From Pavel Machek's email to the gnokii-ml (msgid: <20020414212455.GB9528@elf.ucw.cz>):
	 *  It uses fixed format of provider in CR. If your message matches
	 *  WWW1/1:1234-5678 where 1234 is msgno and 5678 is msgpart, it should
	 *  work.
	 */
	while (*s == 'W')
		s++;
	fprintf(stderr, _("Got message %d: %s\n"), i, s);
	if ((sscanf(s, "%d/%d:%d-%c-", &i1, &i2, &msgno, &c) == 4) && (c == 'X'))
		snprintf(buf, sizeof(buf), "%s/mail_%d_", smsdir, msgno);
	else if (sscanf(s, "%d/%d:%d-%d-", &i1, &i2, &msgno, &msgpart) == 4)
		snprintf(buf, sizeof(buf), "%s/mail_%d_%03d", smsdir, msgno, msgpart);
	else
		snprintf(buf, sizeof(buf), "%s/sms_%s_%d_%d", smsdir, number, getpid(), unknown++);
	if ((output = fopen(buf, "r")) != NULL) {
		fprintf(stderr, _("### Exists?!\n"));
		fclose(output);
		return GN_ERR_FAILED;
	}
	mkdir(smsdir, 0700);
	if ((output = fopen(buf, "w+")) == NULL) {
		fprintf(stderr, _("### Cannot create file %s\n"), buf);
		return GN_ERR_FAILED;
	}

	/* Skip formatting chars */
	if (!strstr(buf, "mail"))
		fprintf(output, "%s", message->user_data[0].u.text);
	else {
		s = message->user_data[0].u.text;
		while (!(*s == '-'))
			s++;
		s++;
		while (!(*s == '-'))
			s++;
		s++;
		fprintf(output, "%s", s);
	}
	fclose(output);
	//return GN_ERR_NONE;
    return post_process_sms(message, state, callback_data);
}

static gn_error do_get_sms(int smsId, gn_data *data, struct gn_statemachine *state)
{
	gn_sms_folder folder;
	gn_sms_folder_list folderlist;
	gn_sms message;
	char *memory_type_string;


    memory_type_string = "SM";
    folder.folder_id = 0;
	data->sms_folder = &folder;
	data->sms_folder_list = &folderlist;

    memset(&message, 0, sizeof(gn_sms));
    message.memory_type = gn_str2memory_type(memory_type_string);
    message.number = smsId;
    data->sms = &message;

    if (GN_ERR_NONE == gn_sms_get(data, state)) {
       post_process_sms(&message, state, NULL); 
    }
    return GN_ERR_NONE;
}

extern gn_error shutdown_sensor_com(struct gn_statemachine *state);
extern gn_error init_sensor_com(struct gn_statemachine *state);

gn_error smsreader(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

    if (init_sensor_com(state) != GN_ERR_NONE) {
        LOGMSG(LOG_LEVEL_CRIT, "Init sensor com fail. Exit!\n");
        return GN_ERR_FAILED;
    }
	state->callbacks.on_sms = smsslave;
	data->callback_data = NULL;
	error = gn_sm_functions(GN_OP_OnSMS, data, state);
	if (!error) {
		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);
		fprintf(stderr, _("Entered sms reader mode...\n"));

#if 0
        //Testing code
        if (GN_ERR_NONE != dial_phone("0976574864", 10000, data, state)) {
            LOGMSG(LOG_LEVEL_ERROR, "Dial phone error.\n");
        }
        int i1 = 15;
        for (i1 = 15; i1 < 27; i1++) {
            LOGMSG(LOG_LEVEL_DEBUG, "Read sms id %d \n", i1);
            do_get_sms(i1, data, state);
            LOGMSG(LOG_LEVEL_DEBUG, "DONE \n");
        }
        LOGMSG(LOG_LEVEL_DEBUG, "Sending sms back!\n");
        //End testing code
#endif

		while (!bshutdown) {
			gn_sm_loop(1, state);
			/* Some phones may not be able to notify us, thus we give
			   lowlevel chance to poll them */
			error = gn_sm_functions(GN_OP_PollSMS, data, state);
		}
		fprintf(stderr, _("Shutting down\n"));

		fprintf(stderr, _("Exiting sms reader mode...\n"));
		state->callbacks.on_sms = NULL;
        //shutdown sensor communication
        shutdown_sensor_com(state);        
		error = gn_sm_functions(GN_OP_OnSMS, data, state);
		if (error != GN_ERR_NONE)
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	} else
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));

	return error;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        LOGMSG(LOG_LEVEL_ERROR, "Not enough parameter.\n");
        usage();
    }
    g_configfile = argv[1];
    if (businit() != 0) {
		LOGMSG(LOG_LEVEL_CRIT, "Init fail. Halt\n");
		_exit(1);
	}
    LOGMSG(LOG_LEVEL_INFO, "Init ok\n");
    if (argc > 2) {
        db_phone_load(argv[2]);
    } else {
        db_phone_load(NULL);
    }
	smsreader(g_data, g_state);
    return 0;
}
