#include <pthread.h>
#include <semaphore.h>
#include "transaction.h"
#include "csapp.h"
#include "debug.h"



TRANSACTION* trans_list_head=&trans_list;

void trans_init(void){
  trans_list_head=malloc(sizeof(TRANSACTION));
  trans_list_head->id=-1;
  trans_list_head->next=trans_list_head;
  trans_list_head->prev=trans_list_head;
}


void trans_fini(void){
  free(trans_list_head);
}

TRANSACTION *trans_create(void){
  TRANSACTION* new_trans = malloc(sizeof(TRANSACTION));
  //Add to list:
  new_trans->prev=trans_list_head->prev;
  new_trans->next=trans_list_head;
  trans_list_head->prev=new_trans;
  new_trans->prev->next=new_trans;

  new_trans->id=(new_trans->prev->id)+1;
  new_trans->status=TRANS_PENDING;
  new_trans->refcnt=1;
  new_trans->depends=malloc(sizeof(DEPENDENCY));
  new_trans->waitcnt=0;
  sem_init(&new_trans->sem,0,0);
  pthread_mutex_init(&new_trans->mutex,NULL);

  debug("Create new transaction. ID: %d",new_trans->id);

  return new_trans;

}

TRANSACTION *trans_ref(TRANSACTION *tp, char *why){
    pthread_mutex_lock(&tp->mutex);
    tp->refcnt++;
    pthread_mutex_unlock(&tp->mutex);
    debug("Reference count incremented to: %d because %s",tp->refcnt,why);
    return tp;
}


void trans_unref(TRANSACTION *tp, char *why){
pthread_mutex_lock(&tp->mutex);
    tp->refcnt--;
    if(tp->refcnt==0){
        free(tp->depends);
        free(tp);
    }
    pthread_mutex_unlock(&tp->mutex);
    debug("Reference count decremented to: %d because %s",tp->refcnt,why);
    //return bp;
}


void trans_add_dependency(TRANSACTION *tp, TRANSACTION *dtp){
  DEPENDENCY* new_dp=malloc(sizeof(DEPENDENCY));
  new_dp->trans=dtp;
  new_dp->next=NULL;
  int same=0;
  DEPENDENCY* current = tp->depends->next;
  DEPENDENCY* prev = tp->depends;
  while(current!=NULL){
    if(current->trans->id==new_dp->trans->id){
      same++;
    }
    current=current->next;
    prev=prev->next;

  }
  //if(prev->trans->id==new_dp->trans->id){
    //same++;
  //}
  if(same==0){
    prev->next=new_dp;
    trans_ref(new_dp->trans,"Dependee for another transaction");
  }

}


TRANS_STATUS trans_commit(TRANSACTION *tp){

  //trans_unref(tp,"transaction is committing");
  //TRANS_STATUS trans_commit=0;
  TRANS_STATUS abort=0;
  DEPENDENCY* current=tp->depends->next;
  while(current!=NULL){

    pthread_mutex_lock(&current->trans->mutex);
    current->trans->waitcnt++;
    pthread_mutex_unlock(&current->trans->mutex);
    P(&current->trans->sem);
      if(current->trans->status==TRANS_COMMITTED){
        V(&current->trans->sem);
        //trans_commit++;
        //break;
      }
      if(current->trans->status==TRANS_ABORTED){
        //tp->status=TRANS_ABORTED;
        abort++;
        //return TRANS_ABORTED;
      }
      //V(&current->trans->sem);


    current=current->next;
  }


  if(abort>0){
    tp->status=TRANS_ABORTED;
    trans_unref(tp,"transaction is committing");
    return TRANS_ABORTED;
  }
  else{
    tp->status=TRANS_COMMITTED;
    for(int i=0;i<=tp->waitcnt;i++){
      V(&tp->sem);
    }
    trans_unref(tp,"transaction is committing");
    return TRANS_COMMITTED;
  }

}


TRANS_STATUS trans_abort(TRANSACTION *tp){
  if(tp->status==TRANS_COMMITTED){
    //trans_unref(tp,"transaction is aborting");
    debug("Fatal Error: cannot abort a transaction already committed");
    exit(EXIT_FAILURE);
  }
  else if(tp->status==TRANS_ABORTED){
    trans_unref(tp,"transaction is aborting");
    return TRANS_ABORTED;
  }
  else{//(tp->status==TRANS_PENDING){
    tp->status=TRANS_ABORTED;
    //DEPENDENCY* current = tp->depends->next;
    //while(current!=NULL){
      //trans_abort(current->trans);
      //current=current->next;
    //}
    //trans_abort(current->trans);
    trans_unref(tp,"transaction is aborting");
    return TRANS_ABORTED;
  }


  //return TRANS_ABORTED;
}


TRANS_STATUS trans_get_status(TRANSACTION *tp){
  return tp->status;
}

void trans_show(TRANSACTION *tp){
  fprintf(stderr,"[id=%d, status=%d, refcnt=%d]\n",tp->id,tp->status,tp->refcnt);
}

void trans_show_all(void){
  TRANSACTION* current = trans_list_head->next;
  while(current->id!=-1){
    trans_show(current);
    current=current->next;
  }
}


