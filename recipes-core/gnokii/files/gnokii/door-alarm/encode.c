static char TEXT[] = {'c', 'K', 'z', 'H', 'H', '9', '$', '%', '6', '5', '^'};
static int g_keyId = 0;

extern void encode(char* buffer, int len);
extern void reset_key(void);

void reset_key(void)
{
    g_keyId = 0;
}

void encode(char* buffer, int len)
{
    int i = 0;
    while(i < len) {
        if (g_keyId == sizeof(TEXT)) {
            g_keyId = 0;
        }
        buffer[i++] ^= TEXT[g_keyId++];
    }
}

