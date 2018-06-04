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



//pid data structure
struct pidarray {
	int pidcount;
	pid_t pidarr[512];
};


//GLOBALS
struct sigaction SIGCHLD_action;
struct pidarray pstack;



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




//this signal will reap children processes
void handle_sigchld(int sig) {
	int saved_errno = errno;
	pid_t dead;
	while (dead = waitpid((pid_t)(-1), 0, WNOHANG) > 0) {
		removepid(dead);
	}
	errno = saved_errno;
}
/*
*
*SOCKET FUNCTIONS
*
*/
struct addrinfo * addrinfoConstructor(char* port) {
	int result;

	struct addrinfo serv_addr;
	struct addrinfo * res;

	//zero out struct
	bzero((char *)&serv_addr, sizeof(serv_addr));

	serv_addr.ai_family = AF_INET;		//set what type
	serv_addr.ai_socktype = SOCK_STREAM;		//we are using TCP
												
	serv_addr.ai_flags = AI_PASSIVE;			//set ai_flags to AI_PASSIVE so we can use it with bind and connect

	result = getaddrinfo(NULL, port, &serv_addr, &res);

	if (result != 0) {
		fprintf(stderr, "Error, the port does not work.\n", gai_strerror(result));
		exit(1);
	}

	return res;
}

//initialize socket
int initSocket(struct addrinfo * res) {
	int sockfd;

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (sockfd == -1) {
		fprintf(stderr, "Error. socket initialization failed.\n");
		exit(1);
	}
	return sockfd;
}


//prepare server socket
void serverSocketPrep(int sockfd, struct addrinfo *res) {

	int bindResult;
	bindResult = bind(sockfd, res->ai_addr, res->ai_addrlen);

	if (bindResult == -1) {
		close(sockfd);
		fprintf(stderr, "Error binding socket \n");
		exit(1);
	}

	int listenResult;

	listenResult = listen(sockfd, 5);

	if (listenResult == -1) {
		close(sockfd);
		fprintf(stderr, "Error listening.\n");
		exit(1);
	}

}
/*
*
*END OF SOCKET FUNCTIONS
*
*/

// runs like otp_enc_d 51002 &
int main(int argc, char*argv[]) {

	int portNum;
	int sockfd;
	int establishedConnectionFD;
	int charsRead;
	int socketConns[MAXPROCESSES];
	socketCount = 0;
	socklen_t sizeofClientInfo;
	char buffer[256];

	struct addrinfo*res;
	struct sockaddr_storage otp_enc_addr;

	//initialize signal stuff
	/*
	**SIGNAL STUFF
	*/
	SIGCHLD_action.sa_handler = &handle_sigchld;
	sigemptyset(&SIGCHLD_action.sa_mask);
	SIGCHLD_action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	//associate SIGCHLD with handle_sigchld
	if (sigaction(SIGCHLD, &SIGCHLD_action, 0) == -1) {
		perror(0);
		exit(1);
	}

	//end of signal stuff
	/*
	** END OF SIGNAL STUFF
	*
	*/




	if (argc < 2) {
		fprintf(stderr, "Make sure you enter a port #\n", argv[0]);
		exit(1);
	}


	portNum = atoi(argv[1]);
	res = addrinfoConstructor(argv[1]);
	sockfd = initSocket(res);

	//data connection related stuff
	serverSocketPrep(sockfd, res);






	int listenStatus;
	listenStatus = listen(listenSocketFD, 5);

	//server runs forever
	while (1) {
		//variable declaration for possible child process
		pid_t spawnPid = -5;
		sizeofClientInfo = sizeof(clientAddress);

		//accept incoming connections
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeofClientInfo);
		if (establishedConnectionFD < 0) {
			error("ERROR on accept");
		}//successful connection
		//in terms of creating that child process as described above, you may either create with fork
		else {
			printf("accept connection successful\n");
			fflush(stdout);

			spawnPid = fork();
			switch (spawnPid)
			{
			case -1: {
				perror("Hull Breach!\n");
				exit(1);
				break;
			}
					 //the child process of otp_enc_d must first check to make sure it is communicating with otp_enc
			case 0: {//only in the child process the the actual encryption takes place


				break;
			}

			default:
			{
				addPid(spawnPid);
				break;
			}

			}
		}

	}
}
