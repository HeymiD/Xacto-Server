#include <pthread.h>
#include "client_registry.h"
#include "csapp.h"
#include "debug.h"


struct client_registry{
    int* file_descriptors;
    int count;
    sem_t fd_sem;
    pthread_mutex_t mutex;
};

CLIENT_REGISTRY *creg_init(){

    CLIENT_REGISTRY* cr_p=malloc(sizeof(CLIENT_REGISTRY));
    cr_p->count=0;
    cr_p->file_descriptors=malloc(1024*sizeof(int));
    sem_init(&cr_p->fd_sem,0,1);
//    sem_init(&cr_p->count_sem,0,1);
    pthread_mutex_init(&cr_p->mutex,NULL);
    debug("count: %d",cr_p->count);
    return cr_p;

}
void creg_register(CLIENT_REGISTRY *cr, int fd){
    pthread_mutex_lock(&cr->mutex);
    cr->file_descriptors[cr->count]=fd;
    cr->count++;
    //V(&cr->fd_sem);
    pthread_mutex_unlock(&cr->mutex);
    debug("count: %d",cr->count);
}


void creg_fini(CLIENT_REGISTRY *cr){
    free(cr->file_descriptors);
    free(cr);
}


void creg_unregister(CLIENT_REGISTRY *cr, int fd){

    debug("Unregistering client: %d", fd);
    pthread_mutex_lock(&cr->mutex);
    int k=-1;
    for(int i=0;i<cr->count;i++){
        if(cr->file_descriptors[i]==fd){
            k=i;
        }
    }
    for(int i=0;i<cr->count;i++){
        if(i>=k){
            cr->file_descriptors[i]=cr->file_descriptors[i+1];
        }
    }
    cr->count--;
    //P(&cr->fd_sem);
    pthread_mutex_unlock(&cr->mutex);
    debug("count: %d",cr->count);
}


void creg_wait_for_empty(CLIENT_REGISTRY *cr){

    while(1){
        P(&cr->fd_sem);
        if(cr->count==0){
            break;
        }
        V(&cr->fd_sem);
    }


    return;
}


void creg_shutdown_all(CLIENT_REGISTRY *cr){
    //int cnt;
    pthread_mutex_lock(&cr->mutex);
    //cnt=cr->count;
    //pthread_mutex_unlock(&cr->mutex);
    //pthread_mutex_unlock(&cr->mutex);
    for(int i=0;i<cr->count;i++){
        //pthread_mutex_lock(&cr->mutex);
        debug("Shutting down client %d", cr->file_descriptors[i]);
        shutdown(cr->file_descriptors[i],SHUT_RDWR);
        //pthread_mutex_unlock(&cr->mutex);
    }
    //debug("count: %d",cr->count);
    pthread_mutex_unlock(&cr->mutex);
}