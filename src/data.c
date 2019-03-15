#include <stdlib.h>
#include <pthread.h>
#include "data.h"
#include <string.h>
#include "debug.h"


BLOB *blob_create(char *content, size_t size){

    BLOB* blob=malloc(sizeof(BLOB));
    blob->size=size;
    blob->refcnt=1;
    //debug("creating blob...");
    blob->content=malloc(size);
    memcpy(blob->content,content,size);
    //blob->content=content;
    //debug("creating after memcopy1");
    blob->prefix=malloc(size+1);
    memcpy(blob->prefix,content,size);
    //blob->prefix=content;
    //debug("creating after memcopy2");
    blob->prefix[size]='\0';
    pthread_mutex_init(&blob->mutex,NULL);
    debug("Blob created. content %s, size: %lu",blob->prefix,blob->size);
    return blob;

}


BLOB *blob_ref(BLOB *bp, char *why){
    pthread_mutex_lock(&bp->mutex);
    bp->refcnt++;
    pthread_mutex_unlock(&bp->mutex);
    debug("Reference count incremented to: %d because %s",bp->refcnt,why);
    return bp;
}


void blob_unref(BLOB *bp, char *why){
    pthread_mutex_lock(&bp->mutex);
    bp->refcnt--;
    pthread_mutex_unlock(&bp->mutex);
    if(bp->refcnt==0){
        free(bp->content);
        free(bp->prefix);
        free(bp);
    }
    debug("Reference count decremented to: %d because %s",bp->refcnt,why);
    //return bp;
}


int blob_compare(BLOB *bp1, BLOB *bp2){

    if(bp1->size!=bp2->size){
        debug("%s and %s are not the same",bp1->prefix,bp2->prefix);
        return 1;
    }
    else{
        return memcmp(bp1->content,bp2->content,bp1->size);
    }

}


int blob_hash(BLOB *bp){
    //debug("hashing...");
    int hash=0;
    for(int i=0;i<bp->size;i++){
        hash=(hash + (*(i+bp->content)*i))%2584174561;
        i++;
    }
    return (hash);
}


KEY *key_create(BLOB *bp){
    KEY* key=malloc(sizeof(KEY));
    key->blob=bp;
    key->hash=blob_hash(bp);
    debug("Key created. hash: %0x,content: [%s]",key->hash,key->blob->prefix);
    return key;
}


void key_dispose(KEY *kp){

    pthread_mutex_lock(&kp->blob->mutex);
    kp->blob->refcnt--;
    pthread_mutex_unlock(&kp->blob->mutex);
    free(kp);
    debug("Key disposed");

}

int key_compare(KEY *kp1, KEY *kp2){
    if(kp1->hash == kp2->hash){
        debug("hashes are the same.");
        return blob_compare(kp1->blob,kp2->blob);
    }
    else{
        debug("hashes are different.");
        return kp1->hash-kp2->hash;
    }
}



VERSION *version_create(TRANSACTION *tp, BLOB *bp){
    debug("creating a new version...");
    VERSION* version = malloc(sizeof(VERSION));
    pthread_mutex_lock(&tp->mutex);
    tp->refcnt++;
    version->creator=tp;
    version->blob=bp;
    version->next=NULL;
    version->prev=NULL;
    pthread_mutex_unlock(&tp->mutex);

    return version;

}


void version_dispose(VERSION *vp){
    pthread_mutex_lock(&vp->creator->mutex);
    vp->creator->refcnt--;
    pthread_mutex_unlock(&vp->creator->mutex);
    pthread_mutex_lock(&vp->blob->mutex);
    vp->blob->refcnt--;
    pthread_mutex_unlock(&vp->blob->mutex);
    free(vp);
    debug("version disposed.");
}



