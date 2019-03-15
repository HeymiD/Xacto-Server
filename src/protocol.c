#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"
#include "csapp.h"
#include "debug.h"


int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data){

    uint32_t size_old = pkt->size;
    pkt->size=htonl(pkt->size);
    pkt->timestamp_sec=htonl(pkt->timestamp_sec);
    pkt->timestamp_nsec=htonl(pkt->timestamp_nsec);
    if(rio_writen(fd,pkt,sizeof(XACTO_PACKET))==-1){
        printf("write err\n");
        //close(fd);
        return -1;
    }
    else{
        if(size_old>0){
        if(rio_writen(fd,data,size_old)==-1){
            printf("write err payload\n");
            //close(fd);
            return -1;
            }
        }
    }



    return 0;
}

int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap){

    int left_bytes;
    if((left_bytes=rio_readn(fd,pkt,sizeof(*pkt)))==-1){
        printf("error at reading.\n");
        //close(fd);
        return -1;
    }
    else if(left_bytes==0){
        //printf("EOF 1\n");
        //debug(EOF);

        //close(fd);
        //Pthread_exit(NULL);
        return -1;
    }

    pkt->size=ntohl(pkt->size);
    pkt->timestamp_sec=ntohl(pkt->timestamp_sec);
    pkt->timestamp_nsec=ntohl(pkt->timestamp_nsec);

    if(pkt->size>0){
        //char* payload = malloc(pkt->size);
        *datap = malloc(pkt->size);
        int lb;
        if((lb=rio_readn(fd,*datap,pkt->size))==-1){
            printf("error at reading payload.\n");
            //close(fd);
            return -1;
        }
        else if(lb==0){
            //printf("EOF 2\n");
            //close(fd);
            //Pthread_exit();
            return -1;
        }
        else{// *datap = payload;
            }
        }

    //close(fd);


    return 0;
}
