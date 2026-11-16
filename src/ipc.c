#include "header/common.h"
#include "header/errexit.h"
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

/*Initialize every channel in every direction with unnamed semaphores and also the indexes in ,out */
static void channel_init(channel_t* channel){
    channel->in = 0;
    channel->out = 0;
    if (sem_init(&channel->mutex, 1, 1) < 0 || sem_init(&channel->empty, 1,CAP) < 0 || sem_init(&channel->full, 1, 0) < 0){
        die("sem_init");
    }
}

static void channel_destroy(channel_t* channel){
    sem_destroy(&channel->mutex);
    sem_destroy(&channel->empty);
    sem_destroy(&channel->full);
}


/*Create all the ipc core structures needed for the project */
shared_t* ipc_create(double p, int n){
    /*clean up the memeory allocated from previous runs*/
    shm_unlink(SHM_NAME);
    /*Open the shared memory with O_CREAT and rights for read-write*/
    int fd = shm_open(SHM_NAME, O_EXCL |  O_CREAT | O_RDWR, 0600);
    if (fd < 0){ 
        die("shm_open");
    }
    /**/
    if (ftruncate(fd, sizeof(shared_t)) < 0){
        die("ftruncate");
    }
    shared_t* s = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
    if (s == MAP_FAILED){
        die("mmap");
    }
    close(fd);
    for (int k=0 ; k<2 ; k++){
        channel_init(&s->p_to_enc[k]);
        channel_init(&s->enc_to_chan[k]);
        channel_init(&s->chan_to_enc[k]);
        channel_init(&s->enc_to_p[k]);
    }

    s->p = p;
    s->n = n;

    return s;


}

/*Only usage for the proc1,2. The function purpose is for the processes to attach to the shared memory*/
shared_t* ipc_attach(void){
    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    /*Sign that indicates that process start hasnt started.*/
    if (fd < 0){return NULL;}
    shared_t* s = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (s == MAP_FAILED){
        die("mmap");
    }
    close(fd);
    return s;
}


void ipc_detach(shared_t* s){
    munmap(s, sizeof(shared_t));
}


void ipc_destroy(shared_t* s){
    for (int i=0; i<2; i++){
        channel_destroy(&s->p_to_enc[i]);
        channel_destroy(&s->enc_to_chan[i]);
        channel_destroy(&s->chan_to_enc[i]);
        channel_destroy(&s->enc_to_p[i]);
    }

    munmap(s, sizeof(shared_t));
    if (shm_unlink(SHM_NAME) < 0){
        die("shm_unlink");
    }
}

void channel_put(channel_t* channel, msg_t* m){
    if (sem_wait(&channel->empty) < 0){
        die("sem_wait");
    }
    if (sem_wait(&channel->mutex) < 0){
        die("sem_wait");
    }
    channel->buf[channel->in] = *m;
    channel->in = (channel->in + 1) % CAP;
    if (sem_post(&channel->mutex) < 0){
        die("sem_post");
    }
    if (sem_post(&channel->full) < 0){
        die("sem_post");
    }
}


void channel_get(channel_t* channel, msg_t* m){
    if (sem_wait(&channel->full) < 0){
        die("sem_wait");
    }
    *m = channel->buf[channel->out];
    channel->out = (channel->out + 1) % CAP;
    if (sem_post(&channel->empty) < 0){
        die("sem_post");
    }
}