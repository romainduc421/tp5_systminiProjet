#include"mes_semaphores.h"

int sem_creation(int * semid,int nbsemaphore)
{
  int sem_key=ftok(FILENAME_SEM,KEY_SEM);
  *semid=semget(sem_key,nbsemaphore,IPC_CREAT|0660);
  if(-1==(*semid))
  {
    perror("Erreur lors de la creation des semaphores");
    return 1;
  }
  return 0;
}
int sem_initialisation(int semid,int num_semaphore,int nbr_jetons)
{
  semval initval;
  initval.val=nbr_jetons;
  return  semctl(semid,num_semaphore,SETVAL,initval);
}
int P(int semid,int num_semaphore)
{
  struct sembuf sembuf;
  sembuf.sem_op=-1;
  sembuf.sem_num=num_semaphore;
  sembuf.sem_flg=0;
  return semop(semid,&sembuf,1);
}
int V(int semid,int num_semaphore)
{
  struct sembuf sembuf;
  sembuf.sem_op=1;
  sembuf.sem_num=num_semaphore;
  sembuf.sem_flg=0;
  return semop(semid,&sembuf,1);
}
int sem_destruction(int semid)
{
  return semctl(semid,IPC_RMID,0);
}
int sem_recup(int *semid,int nbsemaphore)
{
  int sem_key=ftok(FILENAME_SEM,KEY_SEM);
  return semget(sem_key,nbsemaphore,0);

}
