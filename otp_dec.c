#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h> //allows files opening, reading, writing
#include <stdint.h>
#include <errno.h>

#define TOTALCHARS 70002
#define LOWERASCII 65      //A
#define HIGHERASCII 90		//Z
#define SPACEASCII 32		//(space)
#define PACKSIZE 500
/***
*
*FILE CHECKING STUFF
*
*/

int checkFile(char * fileName, char*buffer) {
	FILE * pFile;
	int plaintextfd;
	int plaintext_size;
	plaintext_size = 0;

	int charIndex;
	charIndex = 0;

	//open the file
	pFile = fopen(fileName, "r");
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
		if ((cInt != 32 && cInt != 10) && (cInt < 64 || cInt > 91)) {
			fprintf(stderr, "error: input contains bad characters\n");
			exit(1);
		}


		buffer[charIndex++] = c;

		plaintext_size++;
	} while (1);

	fclose(pFile);
	return plaintext_size;

}



void compareKeyAndCipherText(int keySize, int plainSize, char*keyFile) {
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

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int attemptConnection(struct hostent * server, char* port) {
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

	//attempt connection
	if (connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR connecting");
	}
	return sockfd;

}
//returns sock for comm
int connectToOTP(char*port, struct addrinfo *servinfo) {
	int sockfd, numbytes;
	int rv;
	struct addrinfo hints;
	struct addrinfo *p;
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("127.0.0.1", port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	//loop through all results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(2);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
		s, sizeof s);
	//printf("client: connecting to %s\n", s);
	return sockfd;
}

// 's' is socket you want to send to
// 'buf' is the buffer containing data
//len is pointer to an int container the number of bytes in buffer
//SOURCE http://beej.us/net2/bgnet.html
int sendall(int s, char *buf, int *len)
{
	int total = 0;        // how many bytes we've sent
	int bytesleft = *len; // how many we have left to send
	int n;

	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}

	*len = total; // return number actually sent here

	return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}
//SOURCE http://beej.us/net2/bgnet.html
int recvall(int s, char *buf, int *len)
{
	int total = 0;        // how many bytes we've sent
	int bytesleft = *len; // how many we have left to send
	int n;

	while (total < *len) {
		n = recv(s, buf + total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}

	*len = total; // return number actually sent here

	return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

int writeDecryptTextToStdout(char*decrypttext, int *decryptSize) {
	int n;
	int total = 0;
	int bytesLeft = *decryptSize;
	void* newLine = "\n";

	while (total < *decryptSize) {
		n = write(STDOUT_FILENO, decrypttext, bytesLeft);
		if (n == -1) { break; }
		total += n;
		bytesLeft -= n;
	}
	if (write(STDOUT_FILENO, newLine, 1) == -1) {
		fprintf(stderr, "error writing newline.\n");
		exit(1);
	}
	*decryptSize = total;
	return n == -1 ? -1 : 0;

	//if (n == -1) {
	//	fprintf(stderr, "ERROR: couldn't send ciphertext to stdout\n");
	//	exit(1);
	//}

}

//otp_dec ciphertext1 mykey <port#>
int main(int argc, char*argv[]) {

	int socketFD;
	struct addrinfo *servinfo;

	//store server data
	struct hostent *server;
	char * cipherfile;
	char* keyfile;
	int cipher_size;
	int keytext_size;
	//initialize buffers
	char bufferCipherText[TOTALCHARS];
	memset(bufferCipherText, '\0', sizeof(bufferCipherText));
	char bufferKeyText[TOTALCHARS];
	memset(bufferKeyText, '\0', sizeof(bufferKeyText));
	char bufferDecryptText[TOTALCHARS];
	memset(bufferDecryptText, '\0', sizeof(bufferDecryptText));


	if (argc < 3) {
		fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]); exit(0);
	} // Check usage & args

	cipherfile = argv[1];
	keyfile = argv[2];

	cipher_size = checkFile(cipherfile, bufferCipherText);

	keytext_size = checkFile(keyfile, bufferKeyText);

	compareKeyAndCipherText(keytext_size, cipher_size, keyfile);

	socketFD = connectToOTP(argv[3], servinfo);

	int n;

	char* confirm = "otp_dec";

	n = send(socketFD, confirm, strlen(confirm), 0);
	if (n == -1) {
		fprintf(stderr, "ERROR: could not send confirm\n");
		exit(2);
	}


	char ack[PACKSIZE];
	memset(ack, 0, sizeof(ack));
	//n = read(socketFD, ack, sizeof(ack));
	n = recv(socketFD, ack, sizeof(ack), 0);
	if (strcmp(ack, confirm) == 0) {

	}
	else {
		fprintf(stderr, "ERROR: could not contact otp_dec_d %s\n", argv[3]);
		exit(2);
	}

	//send plaintext to otp_enc_d
	//sendall(socketFD,)

	//send size of cipher file to otp_dec_d
	n = send(socketFD, &cipher_size, sizeof(cipher_size), 0);
	if (n == -1) {
		fprintf(stderr, "ERROR sending cipher_size\n");
		exit(1);
	}


	//after sending size of cipher, send cipher file
	n = sendall(socketFD, bufferCipherText, &cipher_size);
	if (n == -1) {
		fprintf(stderr, "couldn't send entire cipher text\n");
		exit(1);
	}

	//send size of key
	n = send(socketFD, &keytext_size, sizeof(keytext_size), 0);
	if (n == -1) {
		fprintf(stderr, "ERROR sending keytext_size\n");
		exit(1);
	}




	//send key file
	n = sendall(socketFD, bufferKeyText, &keytext_size);
	if (n == -1) {
		fprintf(stderr, "couldn't send entire key text\n");
		exit(1);
	}

	int decryptSize;
	//recv size of decrypt
	n = recv(socketFD, &decryptSize, 4, 0);
	if (n == -1) {
		fprintf(stderr, "couldn't recv entire decrypt size\n");
		exit(1);
	}

	n = recvall(socketFD, bufferDecryptText, &decryptSize);
	if (n == -1) {
		fprintf(stderr, "couldn't recv entire decrypt text\n");
		exit(1);
	}

	n = writeDecryptTextToStdout(bufferDecryptText, &decryptSize);

	if (n == -1) {
		fprintf(stderr, "couldn't write decrypt text to STDOUT\n");
		exit(1);
	}
}