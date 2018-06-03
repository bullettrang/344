#include <stdio.h>
#include <stdlib.h>
#include <time.h> //to use srand()
#include <string.h>
#include <unistd.h>
/*SYNTAX: keygen keylength
**keylength is the length of the key file in characters
**keygen outputs to stdout
**the key file will be any of the 27 allowed characters, generated using the standard UNIX randomization methods
**ASCII caps is 65 - 90 (A-Z) (whitespace can be '91')?
** use modulo 27
*/
#define LOWERASCII 65      //A
#define HIGHERASCII 90		//Z
#define SPACEASCII 32		//(space)

#define SPACEINDEX 26
#define AINDEX 0

//int generateRandom()
// Generates and prints 'count' random
// numbers in range [lower, upper].
void printRandoms(int lower, int upper,int count)
{
	int i;
	for (i = 0; i < count; i++) {
		int num = (rand() %
			(upper - lower + 1)) + lower;
		printf("%d ", num);
	}
}

char* convertKeyIndexToChars(int * keys,int size) {
	char *local;
	//create char array to store chars
	local = malloc(sizeof(char)*size+1);
	//convert each num to char
	int i;
	int tempy;
	char cLocal;
	for (i = 0; i < size; i++) {
		//i.e. convert int to char
		tempy = LOWERASCII + keys[i];
		//printf("tempy is %d\n", tempy);
		//printf("tempy as char is %c\n", tempy);
		//handling space conversion
		if (tempy > 90) {
			tempy = 32;
		}
		cLocal = tempy;
			//sprintf(local[i], "%c",cLocal);
		local[i] = cLocal;
	}
	//printf("appending newline to end of local convert string\n");
	local[size + 1] = '\0';
	//printf("local after conversion is %s \n", local);
	return local;
}

void writeToStdOut(char* input,int size) {
	void *newline = "\n";
	write(STDOUT_FILENO, input, size);
	write(STDOUT_FILENO, newline, 1);
}
int main(int argc, char*argv[]) {
	//send rand
	srand(time(0));
	//vars
	int keylength;
	int *keyarr;
	char * output;
	keylength = atoi(argv[1]);

	//printf("keylength is %d\n", keylength);
	//printf("creating int array to store key nums\n"); 
	keyarr = malloc(sizeof(int)*keylength);

	int i;

	//for length user specify, create random key chars in keylength specified
	//i.e. create 10 random key chars
	
	for (i = 0; i < keylength; i++){
		int randkey;
		randkey = (rand() % (SPACEINDEX - AINDEX + 1)) + AINDEX;
		/*printf("%d\t", randkey);*/
		keyarr[i] = randkey;
		//printf("%d\t",i);
		//printf("%d\t",keyarr[i]);
		//printf("\n");

	}

	output = convertKeyIndexToChars(keyarr, keylength);
	//printf("output after conversion is %s\n", output);

	writeToStdOut(output, keylength);
	//free when done
	free(keyarr);


}




//OUTPUT redirect to stdout
//the last char 