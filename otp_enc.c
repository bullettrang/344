#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h> //allows files opening, reading, writing
/*
*
*This program connects to otp_enc_d, 
*and asks it to perform a one-time pad style encryption 
*as detailed above. 
* By itself, otp_enc doesn’t do the encryption - otp_end_d does.
* The syntax of otp_enc is as follows:
* otp_enc plaintext key port
*/
#define TOTALCHARS 70002
#define LOWERASCII 65      //A
#define HIGHERASCII 90		//Z
#define SPACEASCII 32		//(space)


void error(const char *msg) { 
	perror(msg); exit(0); 
}
/***
*
*FILE CHECKING STUFF
*
*/

int checkFile(char * plaintextfile,char*buffer) {
	FILE * pFile;
	int plaintextfd;
	int plaintext_size;
	plaintext_size = 0;

	int charGrabbed;
	int charIndex;
	charIndex = 0;

	//open the file
	pFile = fopen(plaintextfile, "r");
	//error checking for open
	if (pFile == NULL) {
		perror("error opening file");
	}//keep reading until end of file
	do {
		int cInt;
		char c = fgetc(pFile);
		// Checking for end of file
		if (feof(pFile))
			break;
		cInt = c;

		//skip newline
		if (c == '\n') {
			
			fflush(stdout);
			break;
		}

		//this COND handles bad chars
		//if not space or between A-Z
		if ( (cInt != 32 && cInt != 10) && (cInt < 64 || cInt > 91)) {
			fprintf(stderr, "error: input contains bad characters\n");
			exit(1);
		}


		buffer[charIndex++] = c;
		
		plaintext_size++;
	} while (1);
	//printf("buffer is %s\n", buffer);
	//fflush(stdout);
	//printf("buffer length is %d\n", strlen(buffer));
	//fflush(stdout);
	//check the file for bad chars 
	//read in char by char
	fclose(pFile);
	return plaintext_size;
	
}

void compareKeyAndPlainText(int keySize, int plainSize,char*keyFile) {
	//if key file is shorter then plaintext, terminate
	//send error text to stderr, exit(1)
	if (keySize < plainSize) {
		fprintf(stderr, "ERROR: key '%s' is too short\n", keyFile);
		exit(1);
	}

}
/***
*
*SOCKET PREP STUFF
*
*/

int attemptConnection(struct hostent * server,char* port) {
	struct sockaddr_in serverAddress;
	int sockfd;
	int portNum;
	char * localhost = "localhost";

	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	portNum = atoi(port);
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNum); // Store the port number
	server = gethostbyname(localhost); // Convert the machine name into a special form of address
	if (server == NULL) {
		fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0);
	}
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)server->h_addr, server->h_length);

	//create socket 
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	//check for error
	if (sockfd < 0) {
		error("ERROR creating socket");
	}
	printf("socket created\n");

	
	printf("port assigned to portNum\n");
	fflush(stdout);






	//attempt connection
	if (connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR connecting");
	}
	return sockfd;

}

int main(int argc, char*argv[]) {
	
	int socketFD, portNumber, charsWritten, charsRead;

	
	//store server data
	struct hostent *server;
	char * plaintext;
	char* keyfile;
	int plaintext_size;
	int keytext_size;
	//initialize buffers
	char bufferPlainText[TOTALCHARS];
	memset(bufferPlainText, '\0', sizeof(bufferPlainText));
	char bufferKeyText[TOTALCHARS];
	memset(bufferKeyText, '\0', sizeof(bufferKeyText));
	//END OF initialize buffers


	if (argc < 3) { 
		fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]); exit(0); 
	} // Check usage & args

	plaintext = argv[1];
	printf("plaintext= %s\n", plaintext);
	keyfile = argv[2];
	printf("keyfile= %s\n\n", keyfile);
	printf("port is = %s\n\n", argv[3]);
	//check plaintext file
	plaintext_size=checkFile(plaintext,bufferPlainText);


	//check keyfile
	keytext_size = checkFile(keyfile, bufferKeyText);



	//compare keyfile and plaintextfile lengths
	compareKeyAndPlainText(keytext_size, plaintext_size, keyfile);

	socketFD = attemptConnection(server, argv[3]);



	//send confirm to opt_enc_d.c
	int n;

	char*confirm = "otp_enc";
	n = write(socketFD, confirm, strlen(confirm));
	if (n < 0) {
		error("WRITING TO SOCKET CREATED AN ERROR");
	}














	
	//close after sending msg
	printf("closing socket\n");
	fflush(stdout);
	close(socketFD);


	//printf("plaintext_size= %d\n\n", plaintext_size)

	
	//memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	//portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	//printf("portNumber of otp_enc is %d\n", portNumber);


	//serverAddress.sin_family = AF_INET; // Create a network-capable socket
	//serverAddress.sin_port = htons(portNumber); // Store the port number
	//serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address

	//printf("host name is %s\n", serverHostInfo->h_name);
	//if (serverHostInfo == NULL) {
	//	fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0);
	//}

	////prserve the special arrangement of the bytes in these variables by copying the bytes 
	////in the given order, regardless of format,structure,
	//memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	//// Set up the socket
	//socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	//if (socketFD < 0) error("CLIENT: ERROR opening socket");

	//// Connect to server
	//if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
	//	error("CLIENT: ERROR connecting");


}