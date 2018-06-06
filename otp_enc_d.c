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
//pid data structure
struct pidarray {
	int pidcount;
	pid_t pidarr[512];
};


//GLOBALS
struct sigaction SIGCHLD_action;
struct pidarray pstack;
int numforks;


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
//from BEEJ GUIDE
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
//from BEEJ GUIDE
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
//
//only in the child process will the actual encryption take place, 
//and the ciphertext be written back :
void childActivity(int sockfd) {

	//end of signal stuff


	int n;
	int plainSize;
	char buffer[TOTALCHARS];
	memset(buffer, 0, sizeof buffer);
	//we now know length of plainsize
	n = recv(sockfd, &plainSize, 4, 0);
	if (n == -1) {
		fprintf(stderr, "ERROR recv plain size\n");
		exit(1);
	}

	printf("plainSize %d\n", plainSize);
	

	//need to recv plainsize file
	n = recvall(sockfd, buffer, &plainSize);
	printf("strlen(buffer) %d\n", buffer);
	if (n == -1) {
		fprintf(stderr, "ERROR recvall plainsize content\n");
		exit(1);

	}
	else {
		printf("plain file received is %s\n", buffer);
	}
	
}
//from BEEJ GUIDE
void handleProcesses(int sockfd) {
	char * confirm = "otp_enc";

	
	char s[INET6_ADDRSTRLEN];
	socklen_t sin_size;
	struct sockaddr_storage their_addr; // connector's address information
	int new_fd;
	pid_t spawn;
	
	numforks = 0;
	while (1) {
		char message[PACKSIZE];
		memset(message, 0, sizeof message);
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
		int n;
		n = read(new_fd, message, sizeof(message));
		printf("message %s\n", message);
		//confirm its otp_enc before forking
		if (strcmp(message, confirm) == 0 && (numforks<5)) {
			//inc numforks
			numforks++;
			printf("communicating with the right process\n");
			fflush(stdout);

			
			n = write(new_fd, confirm, sizeof(confirm));

			if (spawn = fork() == 0) {
				printf("inside child of otp_enc_d\n");
				fflush(stdout);

				close(sockfd);

				//do stuff
				childActivity(new_fd);

				_exit(0);
			}
			
			//parent doesn't need this
			close(new_fd);
		}
		else {
			if (numforks > 5) {
				printf("Too many requests\n");
				fflush(stdout);
				continue;
			}
			char * error = "error";
			int n;
			n = write(new_fd, error, sizeof(error));
			continue;
		}


		
	}
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


	//initialize signal stuff
	/*
	**SIGNAL STUFF
	*/

	/*
	** END OF SIGNAL STUFF
	*
	*/


	sockfd=setUPConnection(argv[1],servinfo );
	handleProcesses(sockfd);

	//sockfd =bindLoopSockets(servinfo);



	int portNum;

	int establishedConnectionFD;
	int charsRead;
	int socketConns[MAXPROCESSES];

	socklen_t sizeofClientInfo;
	char buffer[256];
	//sockaddr_in declarations and inits
	//struct sockaddr_in serverAddress, clientAddress;
	//memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	//portNum = atoi(argv[1]);					//change char into int
	//serverAddress.sin_family = AF_INET;			//create a network capable socket
	//serverAddress.sin_port = htons(portNum);
	//serverAddress.sin_addr.s_addr = INADDR_ANY;
	//END OF SOCKET INITS

	//sockfd = initSocket();


	//serverSocketPrep(sockfd, serverAddress);










	//int listenStatus;
	//listenStatus = listen(sockfd, 5);

	//server runs forever
	//while (1) {
	//	//variable declaration for possible child process
	//	pid_t spawnPid = -5;
	//	sizeofClientInfo = sizeof(clientAddress);

	//	//accept incoming connections
	//	establishedConnectionFD = accept(sockfd, (struct sockaddr *)&clientAddress, &sizeofClientInfo);
	//	if (establishedConnectionFD < 0) {
	//		error("ERROR on accept");
	//	}//successful connection
	//	//in terms of creating that child process as described above, you may either create with fork
	//	else {

	//		printf("accept connection successful\n");
	//		fflush(stdout);
	//		char message[500];
	//		memset(message, '\0', sizeof(message));
	//		int n;
	//		//try to receive "otp_enc" so we know its the correct program
	//		char*confirm = "otp_enc";
	//		n = read(establishedConnectionFD, message, sizeof(message));
	//		if (strcmp(message, confirm) == 0) {
	//			printf("This is the right program\n");
	//			fflush(stdout);
	//		}
	//		else {//HANDLES WRONG PROGRAM
	//			printf("This is the wrong program\n");
	//			fflush(stdout);

	//		}

	//	}

	//}




}
