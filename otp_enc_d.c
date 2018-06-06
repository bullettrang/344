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
//this program will run as daemon in background
//upon execution, must output an error if it cannot be run due to a network error

//function is to perform actual encoding

//uses listen()
//uses accept()
////must output an error if it cannot be run due to a network error i.e. port unavailable
//perfoms actual encoding (see wiki)
//use a separate process to handle rest of transaction (see below), which wil occur on newly accepetd socket
//otp_enc_d must support five concurrent socket connections


//only in child process actual encryption takes place

// runs like otp_enc_d 51002 &

//your version of otp_enc_d must support up to 
//five concurrent socket connections running at same time
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
//pid data structure
struct pidarray {
	int pidcount;
	pid_t pidarr[512];
};


//GLOBALS
struct sigaction SIGCHLD_action;
struct pidarray pstack;
int numforks;
char*legalchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

void pidConstructor() {
	pstack.pidcount = 0;
	int i = 0;
	for (i = 0; i< 512; i++) {
		//put dummy values to avoid segfault
		pstack.pidarr[i] = -5;
	}

}


void addPid(pid_t processID) {
	pstack.pidarr[pstack.pidcount++] = processID;
}

void removepid(pid_t deadpid) {
	int removeindex;
	int i = -5;

	for (i = 0; i< pstack.pidcount; i++) {
		if (pstack.pidarr[i] == deadpid) {
			removeindex = i;
			for (i = removeindex; i < pstack.pidcount + 1; i++) {
				pstack.pidarr[i] = pstack.pidarr[i + 1];
			}
			break;
		}
	}
	pstack.pidcount--;

}




//the child process of otp_enc_d must first check to make sure it is communicating with otp_enc



void error(const char*msg) {
	perror(msg);
	exit(1);
}

int generateNewPort() {

}



//this signal will reap children processes
//SOURCE http://beej.us/net2/bgnet.html
void handle_sigchld(int sig) {
	int saved_errno = errno;
	pid_t dead;
	char * die = "dead";
	while (dead = waitpid((pid_t)(-1), 0, WNOHANG) > 0) {
		//removepid(dead);
		//write(STDOUT_FILENO, die, 4);
		numforks--;
	}
	errno = saved_errno;
}
//SOURCE https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c
int receive_int(int *num, int fd)
{
	int32_t ret;
	char *data = (char*)&ret;
	int left = sizeof(ret);
	int rc;
	do {
		rc = read(fd, data, left);
		if (rc <= 0) { /* instead of ret */
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				// use select() or epoll() to wait for the socket to be readable again
			}
			else if (errno != EINTR) {
				return -1;
			}
		}
		else {
			data += rc;
			left -= rc;
		}
	} while (left > 0);
	*num = ntohl(ret);
	return 0;
}

/*
*
*SOCKET FUNCTIONS
*
*/

//from BEEJ GUIDE
//void setUPConnection(struct addrinfo *servinfo,char*port) {
//	struct addrinfo hints;
//	struct sockaddr_storage their_addr;		//connector's address info
//	socklen_t sin_size;
//	int rv;
//	memset(&hints, 0, sizeof hints);
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_flags = AI_PASSIVE;		//use my IP
//
//
//	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
//		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
//		exit(1);
//	}
//}
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
int setUPConnection(char*port,struct addrinfo *servinfo) {

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

void sendCipherText(int sockfd, char*ciphertext, int plainSize) {
	int n;
	n = sendall(sockfd, ciphertext, &plainSize);
	if (n == -1) {
		fprintf(stderr, "ERROR sendall ciphertext \n");
		exit(1);
	}
}


//
//only in the child process will the actual encryption take place, 
//and the ciphertext be written back :

void resizeCharArr(char* carr,int size) {

	carr = malloc(size * sizeof(char));

}

int* encryption(char*plainbuffer, char*keybuffer, int plainsize, int keysize) {
	char* ciphertext;
	//add 1 for null term
	ciphertext = malloc(sizeof(char)*plainsize + 1);
	memset(ciphertext, '\0', sizeof(ciphertext));
	int *ciphernums;

	ciphernums = malloc(sizeof(int)*plainsize);

	int keysEncrypted;
	keysEncrypted = 0;
	int i;
	int j;
	int k;
	j = 0;
	k = 0;
	// go through each char of plainbuffer, 'i' is invariant
	for (i = 0; i < strlen(plainbuffer); i++) {
		int plainnum;
		int keynum;
		int total;			//total= plainchar + keychar
		int mod;
		//compare each char of plainbuffer to legalchars
		while (plainbuffer[i] != legalchars[j] && (j<27)) {
			
			j++;
		}
		plainnum = j;
		printf("plainnum is %d\n", plainnum);
		//compare each char of keybuffer to legalchars
		while (keybuffer[i] != legalchars[k] && (k<27)) {

			k++;
		}
		keynum = k;
		printf("keynum is %d\n", keynum);
		total = plainnum + keynum;
		printf("total of plainnum and keynum is %d\n", total);

		//what will mod be?
		if (total > 26) {
			int sub;
			printf("total: %d is larger then 26\n", total);
			
			mod = total - 26;
		}
		else {
			mod = total % 26;
		}
		printf("mod is %d\n", mod);
		ciphernums[i] = mod;
		
		// go through each char of legalchars

		//reset our vars
		j = 0;
		k = 0;
	}

	return ciphernums;
	
}

char* convertCipherNums(int*ciphernums, int plainSize) {
	//convert each int of ciphernums to a char
	char * ciphertext;
	ciphertext = malloc(sizeof(char)* plainSize);
	int i;
	int j;
	//go thru ciphernums
	for (i = 0; i < plainSize; i++) {
		ciphertext[i] = legalchars[ciphernums[i]];

	}
	return ciphertext;
}

void childActivity(int sockfd,char*port) {
	char message[PACKSIZE];
	memset(message, 0, sizeof message);
	char * confirm = "otp_enc";
	int n;
	char * dynamicChar;
	int keySize;
	int plainSize;
	char bufferPlain[TOTALCHARS];
	char bufferKey[TOTALCHARS];
	memset(bufferKey, 0, sizeof bufferKey);
	n = read(sockfd, message, sizeof(message));
	printf("message %s\n", message);
	fflush(stdout);
		//confirm its otp_enc before forking
	if (strcmp(message, confirm) == 0) {
		//printf("communicating with the right process\n");
		//fflush(stdout);
		n = write(sockfd, confirm, sizeof(confirm));
	}//ELSE TERMINATE EARLY
	else {
		fprintf(stderr, "Error: could not contact otp_enc_d on port %s\n", port );
		exit(2);
	}



	memset(bufferPlain, 0, sizeof bufferPlain);
	//we now know length of plainsize
	n = recv(sockfd, &plainSize, 4, 0);
	if (n == -1) {
		fprintf(stderr, "ERROR recv plain size\n");
		exit(1);
	}

	//printf("plainSize %d\n", plainSize);
	

	//need to recv plainsize file
	n = recvall(sockfd, bufferPlain, &plainSize);

	if (n == -1) {
		fprintf(stderr, "ERRR couldn't send entire plain text\n");
		exit(1);
	}

	//printf("buffer %s\n", bufferPlain);
	//fflush(stdout);
	if (n == -1) {
		fprintf(stderr, "ERROR recvall plainsize content\n");
		exit(1);

	}

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


	int *ciphernums;


	//do encryption
	ciphernums=encryption(bufferPlain, bufferKey, plainSize, keySize);
	int i;
	for (i = 0; i < plainSize; i++) {
		printf("ciphernums[%d]= %d\n", i, ciphernums[i]);
	}
	//convert ciphernums to ciphertext
	printf("\n\n\n\n");
	fflush(stdout);
	char *ciphertext;
	ciphertext = convertCipherNums(ciphernums, plainSize);
	int j;
	for (j = 0; j < plainSize; j++) {
		printf("%c", ciphertext[j]);
	}
	printf("\n\n\n\n");
	fflush(stdout);
	int cipherSize;

	cipherSize = strlen(ciphertext);
	printf("cipherSize is %d\n",cipherSize);
	fflush(stdout);

	//send size of ciphertext
	n = send(sockfd, &cipherSize, sizeof(cipherSize), 0);
	if (n == -1) {
		fprintf(stderr, "ERROR sending cipher size\n");
		exit(1);
	}


	//send ciphertext
	sendCipherText(sockfd, ciphertext, plainSize);

	_exit(0);
}




//from BEEJ GUIDE
void handleProcesses(int sockfd,char*port) {
	char * confirm = "otp_enc";

	
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
				//printf("inside child of otp_enc_d\n");
				//fflush(stdout);
				close(sockfd);// child doesn't need the listener
				//do stuff
				childActivity(new_fd,port);
			}
			//parent doesn't need this
			close(new_fd);
	}//END OF WHILE

}



/*
*
*END OF SOCKET FUNCTIONS
*
*/

// runs like otp_enc_d 51002 &
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

	//for port generation
	srand(time(0));
	struct addrinfo *servinfo;
	int sockfd;
	if (argc < 2) {
		fprintf(stderr, "Make sure you enter a port #\n", argv[0]);
		exit(1);
	}



	sockfd=setUPConnection(argv[1],servinfo );
	handleProcesses(sockfd, argv[1]);





	int portNum;

	int establishedConnectionFD;
	int charsRead;
	int socketConns[MAXPROCESSES];

	socklen_t sizeofClientInfo;
	char buffer[256];


}
