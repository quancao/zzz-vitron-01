#ifndef __GSM_HELPER_H__
#define __GSM_HELPER_H__

#include <stdlib.h>

#include <gnokii.h>
#include <gnokii/error.h>
#include <gnokii/common.h>

#define LOG_LEVEL_CRIT  1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_INFO  4
#define LOG_LEVEL_DEBUG 5
#define LOGMSG(level, fmt, ...) do {\
    fprintf(stderr, fmt, ##__VA_ARGS__);\
}while(0)

gn_gsm_number_type get_number_type(const char *number);
int validate_phone_no(const char *phoneNo);
int dialvoice(const char *number, gn_data *data, struct gn_statemachine *state);
gn_error hangup(int callid, gn_data *data, struct gn_statemachine *state);
gn_error dial_phone(const char* phoneNo, int delayMils, gn_data* data, struct gn_statemachine* state);
gn_error send_sms(const char* msg, const char* phoneNo, gn_data* data, struct gn_statemachine* state);

#endif /* __GSM_HELPER_H__ */

