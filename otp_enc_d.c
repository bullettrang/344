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


//initialize socket
int initSocket() {
	int sockfd;

	 sockfd= socket(AF_INET, SOCK_STREAM, 0);

	 if (sockfd < 0) {
		 error("ERROR opening socket");
	 }
	return sockfd;
}


//prepare server socket
void serverSocketPrep(int sockfd, struct sockaddr_in serverAddress) {

	int bindResult;
	if (bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR on binding");
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

	if (argc < 2) {
		fprintf(stderr, "Make sure you enter a port #\n", argv[0]);
		exit(1);
	}


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

	int portNum;
	int sockfd;
	int establishedConnectionFD;
	int charsRead;
	int socketConns[MAXPROCESSES];

	socklen_t sizeofClientInfo;
	char buffer[256];
	//sockaddr_in declarations and inits
	struct sockaddr_in serverAddress, clientAddress;
	//SOCKET INITS
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	portNum = atoi(argv[1]);					//change char into int
	serverAddress.sin_family = AF_INET;			//create a network capable socket
	serverAddress.sin_port = htons(portNum);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	//END OF SOCKET INITS

	sockfd = initSocket();


	serverSocketPrep(sockfd, serverAddress);










	int listenStatus;
	listenStatus = listen(sockfd, 5);

	//server runs forever
	while (1) {
		//variable declaration for possible child process
		pid_t spawnPid = -5;
		sizeofClientInfo = sizeof(clientAddress);

		//accept incoming connections
		establishedConnectionFD = accept(sockfd, (struct sockaddr *)&clientAddress, &sizeofClientInfo);
		if (establishedConnectionFD < 0) {
			error("ERROR on accept");
		}//successful connection
		//in terms of creating that child process as described above, you may either create with fork
		else {
			printf("accept connection successful\n");
			fflush(stdout);
			char message[500];
			memset(message, '\0', sizeof(message));
			int n;
			//try to receive "otp_enc" so we know its the correct program
			char*confirm = "otp_enc";
			//n = read(establishedConnectionFD, message, sizeof(message));

			//n = write(establishedConnectionFD, returnMsg, strlen(returnMsg));
			n = read(establishedConnectionFD, message, sizeof(message));
			if (strcmp(message, confirm) == 0) {
				printf("This is the right program\n");
				fflush(stdout);
			}
			else {
				printf("This is the wrong program\n");
				fflush(stdout);

			}

			
			//spawnPid = fork();
			//switch (spawnPid)
			//{
			//	case -1: {
			//		perror("Hull Breach!\n");
			//		exit(1);
			//		break;
			//	}
			//		 //the child process of otp_enc_d must first check to make sure it is communicating with otp_enc
			//	case 0: {//only in the child process the the actual encryption takes place


			//		break;
			//	}

			//	default:
			//	{
			//		addPid(spawnPid);
			//		break;
			//	}

			//}//END OF SWITCH FORK
		}

	}
}
