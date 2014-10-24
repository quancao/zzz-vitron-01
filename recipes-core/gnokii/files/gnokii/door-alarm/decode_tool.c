#include <stdio.h>
#include <stdlib.h>

extern void encode(char* buffer, int len);
extern void reset_key(void);

static void read_phonebook(char *filename)
{
	FILE *f=fopen(filename,"rb");

    if (!f) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return;
    }
    fseek(f,0,SEEK_END);

    long len=ftell(f);
    fseek(f,0,SEEK_SET);

	char *data = (char*)malloc(len+1);
    fread(data,1,len,f);
    fclose(f);
    reset_key();
    encode(data, len);
    fprintf(stderr, "DECODED DATA:\n%s\n", data);
	free(data);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: decodetool <filename>\n");
        return -1;
    }
    read_phonebook(argv[1]);    
    return 0;
}
