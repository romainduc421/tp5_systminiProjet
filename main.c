
#include<stdlib.h>
#include<sys/shm.h>
#include <sys/sem.h>
#include<stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "mes_semaphores.h"
#include <ctype.h>
#include <signal.h>
#include <string.h>

/**
 * @author Duc Romain, last modified 29/12 10:37
 * @brief classement
 * Univ Lorraine
 */

//#define nb 30                              /* nbombre d’elements/2 du tableau partage */
#define SEM_ID    250

//int* tab;                                   /*Adresse du segment de memoire partagee */
int sortir;

void capitalize(char* str)
{
  while(*str){
    *str = toupper(*str);
    str++;
  }
}

/**
 * sigchldhandler : handler du fils
 * @param signo  signal-catched number
 */
void sigchldhandler(int signo)
{
  int savedErrno, status;
  pid_t childpid;
  savedErrno = errno;
  while((childpid = waitpid(-1, &status, 0)) > 0)
    fprintf(stdout, "child %d terminates\n", (int)childpid);
  errno = savedErrno;
  char* str = strdup(sys_siglist[signo]);
  capitalize(str);
  if(!str)exit(EXIT_FAILURE);
  sortir = 1;
  fprintf(stdout, "%d\t %s\n", signo,str);
  free(str);
}

/**
 * swap fonction echangeant deux elements a des indices differents d un tableau
 * @param tab  tableau
 * @param j    indice 1
 * @param k    indice 2
 */
void
swap(int* tab, int j, int k)
{
  int temp = tab[j];
  tab[j] = tab[k];
  tab[k] = temp;
}

/**
 * random_delay : permet de generer un delai tres court
 */
void
random_delay()
{
  int random_num;
  struct timespec delay;            /* used for wasting time. */

  random_num = rand() % 10;
  delay.tv_sec = 0;
  delay.tv_nsec = 10*random_num;
  nanosleep(&delay, NULL);
}

/**
 * isBalanced 1ere moitie tous les nb sont inferieurs a ceux de la seconde moitie
 * @param tab  tableau
 */
int
isBalanced(int *tab, int nb)
{
  int res = 1;
  for(int k=1; k<=nb; k++)
  {
    for(int l=nb+1; l<nb*2+1; l++)
    {
      if(tab[k]>tab[l]) res = 0;
    }
  }
  return res;
}

/**
 * minimum_pos    trouver la position du minimum d un tableau entre 2 indices
 * @param  tab                    tableau
 * @param  from_idx               borne_inf inclue
 * @param  to_idx                 borne_sup exclue
 * @return          pos
 */
int minimum_pos(int *tab, int from_idx, int to_idx)
{
  int ind_min = from_idx,k;
  for( k = from_idx; k < to_idx; k++ )
  {
    if( tab[ind_min] > tab[k] )
      ind_min = k;
  }
  return ind_min;
}

/**
 * minimum_pos    trouver la position du maximum d un tableau entre 2 indices
 * @param  tab                    tableau
 * @param  from_idx               borne_inf inclue
 * @param  to_idx                 borne_sup exclue
 * @return          pos
 */
int maximum_pos(int *tab, int from_idx, int to_idx)
{
  int ind_max = from_idx,k;
  for( k = from_idx; k < to_idx; k++ )
  {
    if( tab[ind_max] < tab[k] )
      ind_max = k;
  }
  return ind_max;
}

/**
 * displayTab affichage d'un Tableau
 * @param tab   tableau
 * @param size  taille (2N)
 */
void displayTab(int* tab,int size)
{
  printf("[ ");
  for( int k = 1; k < size+1; k++ )
  {
    printf("%d  ", tab[k]);
    if(k == size/2)
      printf("||  ");
  }

  printf("]\n" );
  fflush(stdout);
}

/**
 * fils_minimum   code du fils : processus min
 * @param tab         tableau
 * @param sem_set_id
 */
void fils_minimum(int *tab, int sem_set_id, int nb)                         /*−−−Code du fils : processus minimum−−−*/
{
  int indice_min;
  sleep(5);
  while( !isBalanced(tab,nb) )
  {
    P(sem_set_id,0);
    indice_min = minimum_pos(tab, nb+1, 2*nb+1);
    fprintf(stdout, "value min = %d, pos %d\n", tab[indice_min], indice_min );


    if( tab[ tab[0] ] >= tab[ indice_min ] )     /* Si tab[shared_min] > tab[indice_min] */
    {
      swap(tab, indice_min, tab[0]);
      V(sem_set_id,1);
      random_delay();
      //fprintf(stdout, "value min = %d, pos %d\n", tab[indice_min], indice_min );
    }

  }
  fprintf(stdout, "value min = %d, pos %d\n", tab[indice_min], indice_min );
  exit(EXIT_SUCCESS);
}

/**
 * main  debut main()
 * @return      0 if success, else another value
 */
int main(int argc, char* argv[])
{
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART|SA_SIGINFO;
  sa.sa_handler = sigchldhandler;
  if(sigaction (SIGCHLD, &sa, NULL) == -1){
    perror("sigaction");
  }
  sortir = 0;
  int* tab;
  srand(time(NULL));
  int id_shm,                               /* Identificateur du segment de memoire partagee */
  i, sem_set_id;
  struct shmid_ds shm_desc ;

  if(argc != 2)
  {
    fprintf(stderr,"usage : ./main 'SIZE_HalfArray'");
    exit(-2);
  }
  int nb = atoi(argv[1]);


  /* Creation d’un segment de memoire partagee, de taille 2nb+1 entiers,
  * le 1er element (indice 0) sert a l'echange (shared_min)
  *les 2nb elements suivants sont le tableau partage (de l'indice 1 inclu à 2nb+1 exclu)
  */

  id_shm = shmget( IPC_PRIVATE, (2*nb+1)*sizeof(int), IPC_CREAT|0666);
  /* Attachement du segment partage a l’adresse tab*/
  tab = (int*)shmat(id_shm,NULL,0);
  if(!tab){
    perror("error shmat (main)\n");
    exit(1);
  }

  if(tab == (void*)-1)
  {
    fprintf( stderr, "Error while memory-sharing\t");
    if( errno == EINVAL )
      fprintf( stderr, "Invalid identifier \n");
    else if( errno == EACCES )
      fprintf( stderr, "Forbidden access \n" );
    else if( errno == ENOMEM )
      fprintf( stderr, "Out of memory \n" );
    else
      fprintf( stdout, "Unknown error \n" );
    return EXIT_FAILURE;
  }
  for( i = 1; i < 2*nb+1; i++ )                        /* Initialisation du tableau (random) */
    tab[i] = (int)100*((float)rand()/RAND_MAX);

  printf("Before : \n");
  displayTab(tab,2*nb);
  fflush(stdout);

  if((sem_set_id = semget(SEM_ID, 1, IPC_CREAT|0600))==-1){
    perror("Error : semget(main)\n");
    exit(1);
  }

  if(sem_initialisation(sem_set_id, 0, 1)==-1){
    perror("Error : semctl(main)\n");
    exit(1);
  }

  int pid_fils = fork(), status;
  if( pid_fils == 0 ){                           /* Creation du fil (processus minimum) */
    fils_minimum(tab, sem_set_id, nb);
    exit(0);
  }
  else if( pid_fils == -1 )
  {
    perror("fork failed");
    exit(EXIT_FAILURE);
  }


  int indice_max=0;
  while( !sortir )                      /* Partie "processus maximum" */
  {
    sleep(2);
    V(sem_set_id,0);

    indice_max = maximum_pos( tab, 1, nb+1 ) ;
    tab[0] = indice_max;
    P(sem_set_id,1);

    fprintf(stdout, "value max = %d, pos %d\n", tab[indice_max], indice_max );

  }
  fprintf(stdout, "Parent : %d has finished (root_bash : %d)\n",getpid(), getppid());
  random_delay();
  printf("After : \n");
  fflush(stdout);
  displayTab(tab,2*nb);

  waitpid(pid_fils, &status, 0);
  sem_destruction(sem_set_id);
  if(shmdt(tab)==-1)  perror("error shmdt(main)\n");                               /* Detachement du segment partage */
  if(shmctl(id_shm,IPC_RMID,&shm_desc)==-1)
    perror("error shmctl(main)\n");             /* Destruction du segment partage */
  return EXIT_SUCCESS;
}

/** OUTPUT
./main 10
Before :
[ 2  38  60  81  36  25  93  85  14  9  ||  80  38  19  97  90  32  38  38  53  21  ]
value max = 93, pos 7
value max = 93, pos 7
value min = 19, pos 13
value min = 21, pos 20
value min = 21, pos 20
value max = 85, pos 8
value min = 21, pos 20
value max = 81, pos 4
value min = 32, pos 16
value max = 60, pos 3
value min = 38, pos 12
value min = 60, pos 12
child 16243 terminates
17	 CHILD EXITED
value max = 38, pos 2
Parent : 16242 has finished (root_bash : 15902)
After :
[ 2  38  38  32  36  25  19  21  14  9  ||  80  60  93  97  90  81  38  38  53  85  ]



./main 35
Before :
[ 10  16  94  36  84  50  86  78  6  56  13  54  32  56  43  98  74  18  62  12  49  49  8  9  79  59  29  10  31  85  58  41  2  52  78  ||  87  2  65  66  9  22  79  63  54  36  7  52  11  25  15  23  75  65  32  84  45  91  13  55  22  99  13  64  1  66  43  89  69  8  55  ]
value max = 98, pos 16
value max = 98, pos 16
value min = 1, pos 64
value min = 2, pos 37
value min = 2, pos 37
value max = 94, pos 3
value min = 2, pos 37
value max = 86, pos 7
value min = 7, pos 46
value max = 85, pos 30
value min = 8, pos 69
value max = 84, pos 5
value min = 9, pos 40
value max = 79, pos 25
value min = 11, pos 48
value max = 78, pos 8
value min = 13, pos 58
value max = 78, pos 35
value min = 13, pos 62
value max = 74, pos 17
value min = 15, pos 50
value max = 62, pos 19
value min = 22, pos 41
value max = 59, pos 26
value min = 22, pos 60
value max = 58, pos 31
value min = 23, pos 51
value max = 56, pos 10
value min = 25, pos 49
value max = 56, pos 14
value min = 32, pos 54
value max = 54, pos 12
value min = 36, pos 45
value max = 52, pos 34
value min = 43, pos 66
value max = 50, pos 6
value min = 45, pos 56
value min = 50, pos 56
child 10770 terminates
17	 CHILD EXITED
value max = 49, pos 21
Parent : 10769 has finished (root_bash : 10684)
After :
[ 10  16  2  36  9  45  7  13  6  25  13  36  32  32  43  1  15  18  22  12  49  49  8  9  11  22  29  10  31  8  23  41  2  43  13  ||  87  94  65  66  84  62  79  63  54  54  86  52  79  56  74  58  75  65  56  84  50  91  78  55  59  99  78  64  98  66  52  89  69  85  55  ]

*/
