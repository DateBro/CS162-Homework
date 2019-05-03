#include <stdio.h>
#include <ctype.h>

void wc(FILE *inFile, char *inName);

int main (int argc, char *argv[]) {
    if (argc == 1)
    {
        wc(stdin,"input");
    }else
    {
        for (int i = 1; i < argc; ++i)
        {
            FILE * inputFile = fopen(argv[i], "r");
            wc(inputFile,argv[i]);
            fclose(inputFile);
        }
    }
    return 0;
}

void wc(FILE *inFile, char *inName) {
    int lines = 0; 
    int words = 0; 
    int bytes = 0;
    char current_letter;
    char last_letter = ' ';
    while((current_letter = fgetc(inFile)) != EOF) {
	bytes += 1;
	if(current_letter == '\n'){
	    lines += 1;
	}
	if(isspace(current_letter) && !isspace(last_letter)){
	    words += 1;
	}
    if(current_letter == '\0'){
	    break;
	}
	last_letter = current_letter;
    }
    if(words == 0 && bytes > 0 && !isspace(last_letter))
	words = 1;
    printf(" %d %d %d %s\n", lines, words, bytes, inName);	
}
