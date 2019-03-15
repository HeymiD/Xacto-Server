#include "client_registry.h"
#include "csapp.h"
#include "transaction.h"
#include "protocol.h"
#include "server.h"
#include "debug.h"
#include "data.h"
#include "store.h"
#include <time.h>



CLIENT_REGISTRY* client_registry;
struct timespec time_values;

void *xacto_client_service(void *arg){

    int connfd = *((int *)arg);
    Free(arg);
    Pthread_detach(pthread_self());
    creg_register(client_registry,connfd);
    TRANSACTION* tr = trans_create();
    //debug("Transaction created with refcount: %d",tr->refcnt);

    while(1){
        XACTO_PACKET* pkt_rcvd=malloc(sizeof(XACTO_PACKET));
        void* data;
        void** payloads=&data;//(void*)payload;
        if(proto_recv_packet(connfd, pkt_rcvd, payloads)==0){
            if(pkt_rcvd->type==XACTO_PUT_PKT){
                debug("[%d] PUT packet recieved",connfd);
                void* data_key;
                void** payloads_key=&data_key;
                XACTO_PACKET* pkt_key=malloc(sizeof(XACTO_PACKET));
                proto_recv_packet(connfd, pkt_key, payloads_key);
                debug("[%d] Recieved key, size %0x",connfd,pkt_key->size);
                void* data_val;
                void** payloads_val=&data_val;
                XACTO_PACKET* pkt_val=malloc(sizeof(XACTO_PACKET));
                proto_recv_packet(connfd, pkt_val, payloads_val);
                debug("[%d] Recieved value, size %0x",connfd,pkt_val->size);
                BLOB* blob_key =blob_create(data_key,pkt_key->size);
                blob_ref(blob_key,"for newly create blob");
                KEY* new_key = key_create(blob_key);
                BLOB* blob_val =blob_create(data_val,pkt_val->size);
                blob_ref(blob_val,"for newly create blob");
                int trans_stat = store_put(tr,new_key,blob_val);
                store_show();
                trans_show(tr);
                XACTO_PACKET* pkt_put_ack=malloc(sizeof(XACTO_PACKET));
                pkt_put_ack->type=XACTO_REPLY_PKT;
                pkt_put_ack->status=tr->status;
                pkt_put_ack->null=0;
                pkt_put_ack->size=0;
                clock_gettime(CLOCK_REALTIME, &time_values);
                pkt_put_ack->timestamp_sec=time_values.tv_sec;
                pkt_put_ack->timestamp_nsec=time_values.tv_nsec;
                proto_send_packet(connfd, pkt_put_ack, NULL);
                free(data);
                free(data_key);
                free(data_val);
                free(pkt_rcvd);
                free(pkt_val);
                free(pkt_key);
                free(pkt_put_ack);
                //free(blob_val);
                //free(blob_key);
                if(trans_stat==TRANS_ABORTED){
                    trans_abort(tr);
                    creg_unregister(client_registry,connfd);
                    Close(connfd);
                    return NULL;
                }

            }
            if(pkt_rcvd->type==XACTO_GET_PKT){
                debug("[%d] GET packet recieved",connfd);
                void* data_key;
                void** payloads_key=&data_key;
                XACTO_PACKET* pkt_key=malloc(sizeof(XACTO_PACKET));
                proto_recv_packet(connfd, pkt_key, payloads_key);
                debug("[%d] Recieved key, size %0x",connfd,pkt_key->size);
                BLOB* blob_key =blob_create(data_key,pkt_key->size);
                //blob_ref(blob_key,"for newly create blob");
                KEY* new_key = key_create(blob_key);
                BLOB* blob_val;
                BLOB** blob_val_p=&blob_val;
                int trans_stat=store_get(tr,new_key,blob_val_p);

                store_show();
                trans_show(tr);

                XACTO_PACKET* pkt_get_ack=malloc(sizeof(XACTO_PACKET));
                pkt_get_ack->type=XACTO_REPLY_PKT;
                pkt_get_ack->status=tr->status;
                pkt_get_ack->null=0;
                pkt_get_ack->size=0;
                clock_gettime(CLOCK_REALTIME, &time_values);
                pkt_get_ack->timestamp_sec=time_values.tv_sec;
                pkt_get_ack->timestamp_nsec=time_values.tv_nsec;
                proto_send_packet(connfd, pkt_get_ack, NULL);

                free(pkt_get_ack);

                XACTO_PACKET* pkt_val=malloc(sizeof(XACTO_PACKET));
                pkt_val->type=XACTO_DATA_PKT;
                pkt_val->status=tr->status;
                if(*blob_val_p==NULL){
                    pkt_val->null=1;
                    pkt_val->size=0;
                }
                else{
                    pkt_val->size=(*blob_val_p)->size;
                    pkt_val->null=0;
                }
                clock_gettime(CLOCK_REALTIME, &time_values);
                pkt_get_ack->timestamp_sec=time_values.tv_sec;
                pkt_get_ack->timestamp_nsec=time_values.tv_nsec;
                if(pkt_val->null){
                    //char*null="[null]";
                    proto_send_packet(connfd, pkt_val, "null");
                }
                else{
                    proto_send_packet(connfd, pkt_val, (*blob_val_p)->content);
                }


                free(data);
                free(data_key);
                free(pkt_rcvd);
                free(pkt_key);
                //free(pkt_rcvd);
                free(pkt_val);
                //free((*blob_val_p)->content);
                if(trans_stat==TRANS_ABORTED){
                    trans_abort(tr);
                    creg_unregister(client_registry,connfd);
                    Close(connfd);
                    return NULL;
                }
            }
            if(pkt_rcvd->type==XACTO_COMMIT_PKT){
                debug("[%d] COMMIT packet recieved",connfd);
                trans_commit(tr);
                store_show();
                trans_show(tr);

                XACTO_PACKET* pkt_commit_ack=malloc(sizeof(XACTO_PACKET));
                pkt_commit_ack->type=XACTO_REPLY_PKT;
                pkt_commit_ack->status=tr->status;
                pkt_commit_ack->null=0;
                pkt_commit_ack->size=0;
                clock_gettime(CLOCK_REALTIME, &time_values);
                pkt_commit_ack->timestamp_sec=time_values.tv_sec;
                pkt_commit_ack->timestamp_nsec=time_values.tv_nsec;
                proto_send_packet(connfd, pkt_commit_ack, NULL);
                free(pkt_commit_ack);
                free(pkt_rcvd);
                //creg_unregister(client_registry,connfd);
                //Close(connfd);
                //return NULL;
            }
        }
        else{
            //debug("Breaking the loop");
            //trans_abort(tr);
            //trans_unref(tr,"Connection lost");
            //free(data);
            free(tr->depends);
            free(tr);
            free(pkt_rcvd);
            //free(data);
            creg_unregister(client_registry,connfd);
            Close(connfd);
            return NULL;
        }
    }

}

