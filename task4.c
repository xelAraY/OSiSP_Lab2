#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

int main(int argc, char const *argv[])
{
	if(argc != 3){
		fprintf(stderr, "Invalid arguments.\nCommand format: %s filename lines_count\n", argv[0]);
        fprintf(stderr, "filename: name of file to read.\n");
        fprintf(stderr, "lines_count: count of lines in block to read.\nEnter 0 to read file in one moment.\n");
		return 1;
	}
    
    char* endptr;
    char* N = argv[2];

    int lines_count = strtol(N, &endptr, 10);

    if ((errno == ERANGE && (lines_count == LONG_MAX || lines_count == LONG_MIN)) || (errno != 0 && lines_count == 0)) {
        fprintf(stderr, "You entered wrong group size:\n");
        fprintf(stderr, "Out of range...\n");
        return -2;
    }

    if (endptr == N) {
        fprintf(stderr, "You entered wrong group size:\n");
        fprintf(stderr, "It should have int value>0, if you want output by groups of lines or it should be 0, if you want solid text\n");
        return -2;
    }

    if(lines_count < 0){
        fprintf(stderr, "Invalid count of lines.\n");
        return 1;
    }

	FILE *file = fopen(argv[1], "r");
	if(!file){
        perror("fopen");
		return 1;
	}

    int i = 0;
    while((i < lines_count || lines_count == 0)){
        char c;
        do{
            c = fgetc(file);
            if(c != EOF)
                printf("%c", c);
        } while (c != EOF && c != '\n');
        
        if(c == EOF)
            break;
        
        i++;
        if(i == lines_count){
            getc(stdin);
            i = 0;
        }
    }

	if(fclose(file)){
        perror("fclose");
		return 1;
	}

	return 0;
}
