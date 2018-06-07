#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>			//sigaction, sigemptyset, struct sigaction, SIGCHLD, SA_RESTART, SA_NOCLDSTOP
#include <sys/wait.h>		//waitpid, pid_t,WNOHANG
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>

#define MAXPROCESSES 5
#define PACKSIZE 500
#define TOTALFORKS 50
#define BACKLOG 5
#define TOTALCHARS 70002

//ascii values
#define LOWERASCII 65      //A
#define HIGHERASCII 90		//Z
#define SPACEASCII 32		//(space)

#define SPACEINDEX 26
#define AINDEX 0

//GLOBALS
struct sigaction SIGCHLD_action;

int numforks;
char*legalchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

//this signal will reap children processes
//SOURCE http://beej.us/net2/bgnet.html
void handle_sigchld(int sig) {
	int saved_errno = errno;
	pid_t dead;
	char * die = "dead";
	while (dead = waitpid((pid_t)(-1), 0, WNOHANG) > 0) {

		numforks--;
	}
	errno = saved_errno;
}

//from BEEJ GUIDE
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//SOURCE http://beej.us/net2/bgnet.html
int setUPConnection(char*port, struct addrinfo *servinfo) {

	struct addrinfo hints;
	struct sockaddr_storage their_addr;		//connector's address info
	socklen_t sin_size;
	int rv;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;		//use my IP


	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}


	struct addrinfo *p;
	int sockfd;
	int yes = 1;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server:socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}
	freeaddrinfo(servinfo);
	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	return sockfd;

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

void sendDecryptText(int sockfd, char*decrypttext, int plainSize) {
	int n;
	n = sendall(sockfd, decrypttext, &plainSize);
	if (n == -1) {
		fprintf(stderr, "ERROR sendall decrypttext \n");
		exit(1);
	}
}


//
//only in the child process will the actual encryption take place, 
//and the ciphertext be written back :

void resizeCharArr(char* carr, int size) {

	carr = malloc(size * sizeof(char));

}
int * decryption(char*cipherbuffer, char*keybuffer, int ciphersize, int keysize) {
	char * decryptttext;
	//add 1 for null term
	decryptttext = malloc(sizeof(char)* ciphersize + 1);
	memset(decryptttext, '\0', sizeof(decryptttext));
	int* decryptnums;
	decryptnums = malloc(sizeof(int)*ciphersize);

	int i;
	int j;
	int k;
	j = 0;
	k = 0;
	//go through each char of cipherbuffer, 'i' is invariant
	for (i = 0; i < strlen(cipherbuffer); i++) {
		int ciphernum;
		int keynum;
		int diff;		//diff= ciphernum -keynum
		int mod;

		while (cipherbuffer[i] != legalchars[j] && (j < 27)) {
			
			j++;
		}
		ciphernum = j;
		//printf("ciphernum is %d\n", ciphernum);
		while (keybuffer[i] != legalchars[k] && (k < 27)) {
			k++;
		}
		keynum = k;
		//printf("keynum is %d\n", keynum);

		diff = ciphernum - keynum;
		//if i did this, i lose A....
	
		 if (diff < 0) {
			mod = diff + 27;
		}
		else {
			mod = diff % 27;
		}
		//printf("mod is %d\n", mod);
		decryptnums[i] = mod;

		j = 0;
		k = 0;
	}
	return decryptnums;
}

char * convertDecryptNums(int * decryptNums, int cipherSize) {
	char * decrypttext;
	decrypttext = malloc(sizeof(char)* cipherSize);
	int i;
	int j;
	for (i = 0; i < cipherSize; i++) {
		decrypttext[i] = legalchars[decryptNums[i]];
		
	}
	return decrypttext;
}

void childActivity(int sockfd, char*port) {
	char message[PACKSIZE];
	memset(message, 0, sizeof message);
	char * confirm = "otp_dec";
	int n;
	char * dynamicChar;
	int keySize;
	int cipherSize;
	char bufferCipher[TOTALCHARS];
	char bufferKey[TOTALCHARS];
	memset(bufferKey, 0, sizeof bufferKey);
	n = read(sockfd, message, sizeof(message));

	//confirm its otp_dec before forking
	if (strcmp(message, confirm) == 0) {

		n = write(sockfd, confirm, sizeof(confirm));
	}//ELSE TERMINATE EARLY
	else {
		fprintf(stderr, "Error: could not contact otp_dec_d on port %s\n", port);
		exit(2);
	}



	memset(bufferCipher, 0, sizeof bufferCipher);
	//we now know length of ciphersize
	n = recv(sockfd, &cipherSize, 4, 0);
	if (n == -1) {
		fprintf(stderr, "ERROR recv cipher size\n");
		exit(1);
	}

	//printf("plainSize %d\n", plainSize);


	//need to recv cipher file
	n = recvall(sockfd, bufferCipher, &cipherSize);

	if (n == -1) {
		fprintf(stderr, "ERRR couldn't recv entire cipher text\n");
		exit(1);
	}

	//printf("buffer %s\n", bufferPlain);
	//fflush(stdout);



	//receive keysize
	n = recv(sockfd, &keySize, 4, 0);
	if (n == -1) {
		fprintf(stderr, "ERROR recv key size\n");
		exit(1);
	}

	//printf("keySize %d\n", keySize);
	//fflush(stdout);

	//receive keyfile
	n = recvall(sockfd, bufferKey, &keySize);
	if (n == -1) {
		fprintf(stderr, "ERROR recvall bufferKey content\n");
		exit(1);
	}


	int *decryptnums;


	//do decryption
	decryptnums = decryption(bufferCipher, bufferKey, cipherSize, keySize);
	int i;

	printf("\n\n\n\n");
	fflush(stdout);

	char *decrypttext;
	decrypttext = convertDecryptNums(decryptnums, cipherSize);
	int j;
	for (j = 0; j < cipherSize; j++) {
		printf("%c", decrypttext[j]);
	}
	printf("\n\n\n\n");
	fflush(stdout);
	int decryptSize;

	decryptSize = strlen(decrypttext);
	printf("decryptSize is %d\n", decryptSize);
	fflush(stdout);

	//send size of decrypttext
	n = send(sockfd, &decryptSize, sizeof(decryptSize), 0);
	if (n == -1) {
		fprintf(stderr, "ERROR sending decrypt size\n");
		exit(1);
	}


	//send decrypttext
	sendDecryptText(sockfd, decrypttext, decryptSize);
	

	_exit(0);
}

//from BEEJ GUIDE
void handleProcesses(int sockfd, char*port) {
	char * confirm = "otp_dec";


	char s[INET6_ADDRSTRLEN];
	socklen_t sin_size;
	struct sockaddr_storage their_addr; // connector's address information
	int new_fd;
	pid_t spawn;

	numforks = 0;
	while (1) {

		int bytesRECV;
		int bytesSENT;
		char msg[1024];
		memset(msg, 0, sizeof msg);
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
		if (new_fd == -1) {
			//perror("ERROR on accept");
			continue;
		}
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);
		fflush(stdout);
		numforks++;

		if (numforks > 5) {
			printf("Too many requests\n");
			fflush(stdout);
			continue;
		}
		//CHILD PROCESS
		if (spawn = fork() == 0) {

			close(sockfd);// child doesn't need the listener
						  //do stuff
			childActivity(new_fd, port);
		}
		//parent doesn't need this
		close(new_fd);
	}//END OF WHILE

}

int main(int argc, char*argv[]) {
	SIGCHLD_action.sa_handler = &handle_sigchld;	//reap children
	sigemptyset(&SIGCHLD_action.sa_mask);

	//DO I NEED  SA_NOCLDSTOP???
	SIGCHLD_action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	//associate SIGCHLD with handle_sigchld
	if (sigaction(SIGCHLD, &SIGCHLD_action, 0) == -1) {
		perror(0);
		exit(1);
	}

	struct addrinfo *servinfo;
	int sockfd;
	if (argc < 2) {
		fprintf(stderr, "Make sure you enter a port #\n", argv[0]);
		exit(1);
	}

	sockfd = setUPConnection(argv[1], servinfo);
	handleProcesses(sockfd, argv[1]);
}