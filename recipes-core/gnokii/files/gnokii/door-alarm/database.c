#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include "database.h"
#include "gsmhelper.h"
#include "cJSON.h"

#define DEFAULT_CONFIG_DIR  "/system/phone"
#define DEFAULT_PHONE_FILE  "phone.json"
#define DEFAULT_SENSOR_FILE "sensor.json"

struct phonebook_t {
    phone_entry_t* head;
    int size;

} G_phoneBook = {
    NULL, 0
};

static void _add_phone(const char* name, const char* phoneno)
{
    //TODO: mutex protection of calling function
    phone_entry_t* entry = malloc(sizeof(*entry));
    entry->name = strdup(name);
    entry->phoneNo = strdup(phoneno);
    entry->next = G_phoneBook.head;
    G_phoneBook.head = entry;
    G_phoneBook.size++;
    LOGMSG(LOG_LEVEL_DEBUG, "Adding phone entry: name=%s phone=%s \n", name, phoneno);
}

static void delete_phone_entry(phone_entry_t **_entry)
{
    phone_entry_t *entry = *_entry;
    *_entry = NULL;

    free(entry->name);
    free(entry->phoneNo);
    free(entry);
}


static void _remove_phone(const char* phoneno)
{
    phone_entry_t* entry;
    phone_entry_t* prev = NULL;

    //TODO: mutex protection of calling function
    entry = G_phoneBook.head;
    while(entry && strcmp(entry->phoneNo, phoneno)) {
        prev = entry;
        entry = entry->next;
    }
    if (entry == NULL) {
        LOGMSG(LOG_LEVEL_WARN, "Could not found registered phone no %s. Has been removed or un-registered ?\n", phoneno);
        return;
    }
    if (prev == NULL) {
        G_phoneBook.head = entry->next;
    } else {
        prev->next = entry->next;
    }
    //Delete this number
    delete_phone_entry(&entry);
    G_phoneBook.size--;
    LOGMSG(LOG_LEVEL_INFO, "Remove phone no %s success.\n", phoneno);
}

static void empty_phonebook(void)
{
    phone_entry_t* entry;

    //TODO: mutex protection here
    LOGMSG(LOG_LEVEL_INFO, "Empty phonebook\n");
    while((entry = G_phoneBook.head) != NULL) {
        G_phoneBook.head = entry->next;
        delete_phone_entry(&entry);
    }
    G_phoneBook.size = 0;
}

static gn_error parse_phonebook(char *text)
{
    cJSON *root;
	
	root = cJSON_Parse(text);
	if (!root) {
        LOGMSG(LOG_LEVEL_CRIT, "Error before: [%s]\n",cJSON_GetErrorPtr());
        return GN_ERR_FAILED;
    }
    cJSON* phonebook = cJSON_GetObjectItem(root,"phonebook");
    if (!phonebook) {
        LOGMSG(LOG_LEVEL_CRIT, "Error: could not read phonebook object\n");
        return GN_ERR_FAILED;
    }
    int i; 
    for (i = 0;i < cJSON_GetArraySize(phonebook);i++)
    {
        cJSON *phoneItem = cJSON_GetArrayItem(phonebook,i);
        // handle subitem.	
        LOGMSG(LOG_LEVEL_DEBUG, "item #%d: name=%s phone=%s\n",i, 
                cJSON_GetObjectItem(phoneItem, "name")->valuestring,
                cJSON_GetObjectItem(phoneItem, "phone")->valuestring);
        _add_phone(cJSON_GetObjectItem(phoneItem, "name")->valuestring,
                cJSON_GetObjectItem(phoneItem, "phone")->valuestring);
    }
    cJSON_Delete(root);
    return GN_ERR_NONE;
}
//#define RC4_KEY             "cKzHH9$%65^"
extern void encode(char* buffer, int len);
extern void reset_key(void);

/* Read a file, parse, render back, etc. */
static gn_error read_phonebook(char *filename, int isEncoded)
{
	FILE *f=fopen(filename,"rb");

    if (!f) {
        LOGMSG(LOG_LEVEL_CRIT, "Could not open file %s\n", filename);
        return GN_ERR_FAILED;
    }
    fseek(f,0,SEEK_END);

    long len=ftell(f);
    fseek(f,0,SEEK_SET);

	char *data = (char*)malloc(len+1);
    fread(data,1,len,f);
    fclose(f);
    if (isEncoded) {
        encode(data, len);
    }
	gn_error err = parse_phonebook(data);
	free(data);
    return err;
}

static char PHONE_FILENAME_BUFFER[128] = DEFAULT_SENSOR_FILE "/" DEFAULT_PHONE_FILE;
static const char *BEGIN_PHONE = "{\"phonebook\":[\n";
static const char *END_PHONE = "]}";

static int safe_write(int fd, char* buffer, int len)
{
    int n;
    errno = 0;
    do {
        n = write(fd, buffer, len);
        if (n > 0) {
            len -= n;
            buffer += n;
        }
    } while(len > 0 && (errno == 0 || errno == EINTR));
    return len;
}

gn_error db_phone_store(void)
{
    char *begin = strdup(BEGIN_PHONE);
    char *end = strdup(END_PHONE);
    int fd;
    char buffer[256];
    int len;

    snprintf(buffer, sizeof(buffer), "%s.encode", PHONE_FILENAME_BUFFER);
    LOGMSG(LOG_LEVEL_INFO, "Will save phonebook to file %s\n", buffer);
    fd = open(buffer, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        LOGMSG(LOG_LEVEL_CRIT, "Could not open file %s for saving phonebook\n", buffer);
        goto out;
    }
    //Reset key
    reset_key();
    encode(begin, strlen(BEGIN_PHONE));
    if (safe_write(fd, begin, strlen(BEGIN_PHONE))) {
        LOGMSG(LOG_LEVEL_CRIT, "Write fail. Could not save phonebook\n");
        goto out;
    }
    phone_entry_t* entry = G_phoneBook.head;
    while(entry) {
        if (entry->next) {
            len = snprintf(buffer, sizeof(buffer), "{\"name\": \"%s\", \"phone\": \"%s\"},\n", 
                    entry->name, entry->phoneNo);
        } else {
            len = snprintf(buffer, sizeof(buffer), "{\"name\": \"%s\", \"phone\": \"%s\"}\n", 
                    entry->name, entry->phoneNo);
        }
        encode(buffer, len);
        if (safe_write(fd, buffer, len)) {
            LOGMSG(LOG_LEVEL_CRIT, "Write fail. Could not save phonebook\n");
            goto out;
        }
        entry = entry->next;
    }
    encode(end, strlen(END_PHONE));
    if (safe_write(fd, end, strlen(END_PHONE))) {
        LOGMSG(LOG_LEVEL_CRIT, "Write fail. Could not save phonebook\n");
        goto out;
    }
out:
    if (fd >= 0) {
        close(fd);
    }
    free(begin);
    free(end);
    return GN_ERR_NONE;
}

#ifndef true
#define true 1
#define false 0
#endif

gn_error db_phone_load(const char* phoneDir)
{
    if (phoneDir == NULL) {
        snprintf(PHONE_FILENAME_BUFFER, sizeof(PHONE_FILENAME_BUFFER), "%s/%s", DEFAULT_CONFIG_DIR, DEFAULT_PHONE_FILE);
    } else {
        snprintf(PHONE_FILENAME_BUFFER, sizeof(PHONE_FILENAME_BUFFER), "%s/%s", phoneDir, DEFAULT_PHONE_FILE);
    }
    if (read_phonebook(PHONE_FILENAME_BUFFER, true) != GN_ERR_NONE) {
        LOGMSG(LOG_LEVEL_ERROR, "Parse encoded phonebook fail. Fall back to clear text!\n");
        read_phonebook(PHONE_FILENAME_BUFFER, false);
    }
    db_phone_store();
    return GN_ERR_NONE;
}

gn_error db_register_phone(const char* phoneNo, const char* name)
{
    //We support maximum 2 alarm phone. If user registers more than that, we will ignore!
    _add_phone(name, phoneNo);
    return GN_ERR_NONE;
} 

gn_error db_unregister_phone(const char* phoneNo)
{
    if (!strcmp("het", phoneNo)) {
         empty_phonebook();
    } else {
        _remove_phone(phoneNo);
    }
    return GN_ERR_NONE;
}

