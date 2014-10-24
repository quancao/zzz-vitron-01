#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "gsmhelper.h"

#ifndef true
#define true 1
#define false 0
#endif

gn_gsm_number_type get_number_type(const char *number)
{
	gn_gsm_number_type type;

	if (!number)
		return GN_GSM_NUMBER_Unknown;
	if (*number == '+') {
		type = GN_GSM_NUMBER_International;
		number++;
	} else {
		type = GN_GSM_NUMBER_Unknown;
	}
	while (*number) {
		if (!isdigit(*number))
			return GN_GSM_NUMBER_Alphanumeric;
		number++;
	}
	return type;
}

//#define TRUE 1
//#define FALSE 0
int validate_phone_no(const char *phoneNo)
{
    if (phoneNo == NULL) {
        return false;
    }
    LOGMSG(LOG_LEVEL_DEBUG, "Validate phone number:[%s] len=%d \n", phoneNo, (int)strlen(phoneNo));
    const char *s = phoneNo;
    //Atleast, 10 number
    if (strlen(phoneNo) < 10) {
        return false;
    }
    while(*s != '\0') {
        if (*s != '+' && 
                ('0' > *s) && 
                (*s > '9')) {
            return false;
        }
        s++;
    }
    if (phoneNo[0] == '0') {
        /* 10 numbers: 0989892566 */
        if ((phoneNo[1] == '9' && strlen(phoneNo) == 10) || 
             (phoneNo[1] != '9' && strlen(phoneNo) == 11)) {
            return true;
        }
        return false;
    }
    if (phoneNo[0] == '+' && phoneNo[1] == '8' && phoneNo[2] == '4') {
        phoneNo += 3;
        //For example: +84976574877 or +841234567890
        if ((phoneNo[0] == '9' && strlen(phoneNo) == 9) || 
             (phoneNo[0] != '9' && strlen(phoneNo) == 10)) {
            return true;
        }
        return false;
    }
    return false;
}

/* Voice dialing mode. */
/**
 * @brief 
 *
 * @param number
 * @param data
 * @param state
 *
 * @return callid
 */
int dialvoice(const char *number, gn_data *data, struct gn_statemachine *state)
{
	gn_call_info call_info;
	int call_id;
	gn_error error;

	memset(&call_info, 0, sizeof(call_info));
	snprintf(call_info.number, sizeof(call_info.number), "%s", number);
	call_info.type = GN_CALL_Voice;
	call_info.send_number = GN_CALL_Default;

	gn_data_clear(data);
	data->call_info = &call_info;

	if ((error = gn_call_dial(&call_id, data, state)) != GN_ERR_NONE) {
		LOGMSG(LOG_LEVEL_ERROR, "Dialing failed: %s\n", gn_error_print(error));
        call_id = -1;
    }
	else {
		LOGMSG(LOG_LEVEL_ERROR, "Dialled call, id: %d (lowlevel id: %d)\n", call_id, call_info.call_id);
    }
	return call_id;
}

/* Hangup the call */
gn_error hangup(int callid, gn_data *data, struct gn_statemachine *state)
{
	gn_call_info callinfo;
	gn_error error;

	memset(&callinfo, 0, sizeof(callinfo));
	callinfo.call_id = callid;
    errno = 0;
	if (errno || callinfo.call_id < 0) {
        return GN_ERR_FAILED;
    }

	gn_data_clear(data);
	data->call_info = &callinfo;

	error = gn_sm_functions(GN_OP_CancelCall, data, state);

	if (error != GN_ERR_NONE) {
		LOGMSG(LOG_LEVEL_ERROR, "Error: %s\n", gn_error_print(error));
	}
	
	return error;
}

gn_error dial_phone(const char* phoneNo, int delayMils, gn_data* data, struct gn_statemachine* state)
{
    int callid = dialvoice(phoneNo, data, state);
    if (callid < 0) {
        return GN_ERR_FAILED;
    }
    //struct timespec tp = {
    //    .tv_sec = delayMils / 1000,
    //    .tv_nsec = (delayMils % 1000) * 1000000
    //};
    sleep(delayMils/1000);
    //if (nanosleep(&tp, NULL)) {
    //    LOGMSG(LOG_LEVEL_ERROR, "delay for dial phone fail. Errno=%d\n", errno);
    //}
    LOGMSG(LOG_LEVEL_ERROR, "Hang up now\n");
    return hangup(callid, data, state);
}


gn_error send_sms(const char* msg, const char* phoneNo, gn_data* data, struct gn_statemachine* state)
{
    gn_sms sms;
    gn_error error;
    /* The maximum length of an uncompressed concatenated short message is
       255 * 153 = 39015 default alphabet characters */
    int curpos = 0;

    /* The memory is zeroed here */
    gn_sms_default_submit(&sms);

    snprintf(sms.remote.number, sizeof(sms.remote.number), "%s", phoneNo);
    sms.remote.type = get_number_type(sms.remote.number);
    if (sms.remote.type == GN_GSM_NUMBER_Alphanumeric) {
        LOGMSG(LOG_LEVEL_ERROR, "Invalid phone number\n");
        return GN_ERR_WRONGDATAFORMAT;
    }
    //HOPE that message is shorter than 200
    //strncpy(&sms.user_data[curpos], msg, 200);
    gn_sms_user_data *udata = &sms.user_data[curpos];
    udata->length = snprintf(udata->u.text, sizeof(udata->u.text), "%s", msg);

    sms.user_data[curpos].type = GN_SMS_DATA_Text;
    if ((sms.dcs.u.general.alphabet != GN_SMS_DCS_8bit)
            && !gn_char_def_alphabet(sms.user_data[curpos].u.text))
        sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;
    sms.user_data[++curpos].type = GN_SMS_DATA_None;
    data->sms = &sms;

    /* Send the message. */
    error = gn_sms_send(data, state);

    if (error == GN_ERR_NONE) {
        if (sms.parts > 1) {
            int j;
            LOGMSG(LOG_LEVEL_INFO, "Message sent in %d parts with reference numbers:", sms.parts);
            for (j = 0; j < sms.parts; j++)
                LOGMSG(LOG_LEVEL_INFO, " %d", sms.reference[j]);
            LOGMSG(LOG_LEVEL_INFO, "\n");
        } else
            LOGMSG(LOG_LEVEL_INFO, "Send succeeded with reference %d!\n", sms.reference[0]);
    } else {
        LOGMSG(LOG_LEVEL_INFO, "SMS Send failed (%s)\n", gn_error_print(error));
    }

    free(sms.reference);
    return error;
}

