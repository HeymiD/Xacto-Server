#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include "csapp.h"
#include "server.h"

static void terminate(int status);
static void signal_handler(int sig);
static void *serve_client_thread(void *vargp);




CLIENT_REGISTRY *client_registry;

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    int optval;
    char* hostname="";
    char* port="";
    int prompt=1;
    while(optind<argc){
        if((optval = getopt(argc, argv, "h:p:q")) != -1){
            if(argc<3){
                optval='?';
            }
            if(argc==3){
                if(optval!='p'){
                    optval='?';
                }
            }
            switch (optval){
                case 'h':
                    hostname = optarg;
                    break;
                case 'p':
                    port = optarg;
                    break;
                case 'q':
                    prompt=0;
                    break;
                default: /* '?' */
                   fprintf(stderr, "Usage: %s -p <portnumber> [-h <hostname>] [-q] \n",
                           argv[0]);
                   exit(EXIT_FAILURE);
            }
        }
    }
    if(strcmp(port,"")==0){
        fprintf(stderr, "Usage: %s -p <portnumber> [-h <hostname>] [-q] \n",
                           argv[0]);
        exit(EXIT_FAILURE);
    }


    struct sigaction new_action;
    new_action.sa_handler = signal_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGHUP,&new_action,NULL);

    fprintf(stdout, "Hostname: %s\n", hostname);
    fprintf(stdout, "Port: %s\n", port);
    fprintf(stdout, "Prompt: %d\n", prompt);

    //return 1;

    // Perform required initializations of the client_registry,
    // transaction manager, and object store.
    client_registry = creg_init();
    trans_init();
    store_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function xacto_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.



    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen=sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd,
        (SA *) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, serve_client_thread, connfdp);
        //Pthread_join(tid,NULL);

    }



    fprintf(stderr, "You have to finish implementing main() "
	    "before the Xacto server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    trans_fini();
    store_fini();

    debug("Xacto server terminating");
    exit(status);
}
void signal_handler(int sig)
{
    terminate(EXIT_SUCCESS);
}

void *serve_client_thread(void *vargp)
{

 //int connfd = *((int *)vargp);
 //Pthread_detach(pthread_self());
 xacto_client_service(vargp);
 //Free(vargp);
 //echo(connfd);
 //Close(connfd);
 return NULL;
}