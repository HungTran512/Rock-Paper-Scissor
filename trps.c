#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

// global variables
int turns = 0;
struct playerid{
  int p;
  int t;
};
typedef struct playerid pid;
pid p1, p2;
int wins[3], throws[2];
int sig, refsig, readbuf, games;

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ref_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t throw_cv = PTHREAD_COND_INITIALIZER;
pthread_t tid1, tid2;
struct timeval tv;



/*converting numbers to RPS, rock = 0,
paper = 1 and scissors = 2*/
char *RPS(int throw){
  if(throw == 0){
    return "rock";
  }
  else if(throw == 1){
    return "paper";
  }
  else if (throw == 2) {
    return "scissors";
  }
  // if something else is passed in
  else {
    return "invalid option";
 }
}


//find winner based on throws
int getwin(int throw1, int throw2){
  if (throw1 == throw2){
		return -1;
  }
	else if ((throw1 == 0) && (throw2 == 2)){
		return 0;
  }
	else if ((throw1 == 2) && (throw2 == 0)){
		return 1;
  }
  // paper beats rock, scissors beat paper
	else{
		return (throw1 > throw2) ? 1 : 0;
  }
}
// thread function for players
void *player(void *arg) {

 pid *pp = (pid*)arg;
 pp->t = (unsigned int) pthread_self();
 //generate randome number
 gettimeofday(&tv, NULL);
 srand(tv.tv_sec + tv.tv_usec + (int)pthread_self());

 pthread_mutex_lock(&m1);
 // wait for signal
 while (readbuf == 0){
   pthread_cond_wait(&ref_cv, &m1);
 }
 // received signal
 sig = refsig;
 readbuf--;
 // unlock thread
 pthread_mutex_unlock(&m1);
 // go to critical section & throwing
 while(sig == 1){
   pthread_mutex_lock(&m2);
   throws[pp->p] = rand() % 3;
   games++;
   pthread_cond_signal(&throw_cv);
   pthread_mutex_unlock(&m2);
   pthread_mutex_lock(&m1);
   //wait for next signal
   pthread_cond_wait(&ref_cv, &m1);
   sig = refsig;
   pthread_mutex_unlock(&m1);
 }
 return NULL;
}
// referee function
void ref(int turn){
  int winner;
  // getting process ids and printing
	printf("Child 1 PID: %u\n", p1.t);
	printf("Child 2 PID: %u\n", p2.t);
  printf("Beginning %d rounds...\n", turn);
  printf("Fight\n");
  //plays ganes u
  for(int i = 0; i < turn; i++) {
    // initialte value and unlock to play
    pthread_mutex_lock(&m1);
    refsig = 1;
    readbuf = 2;
    pthread_cond_broadcast(&ref_cv);
    pthread_mutex_unlock(&m1);
    pthread_mutex_lock(&m2);
    // loop indefintely until receive signal that players have finished throwing
    while(games < 2){
      pthread_cond_wait(&throw_cv, &m2);
    }

    printf("-------------------------\n");
		printf("Round %d:\n", i+1);
		printf("Child 1 throws %s!\n", RPS(throws[0]));
		printf("Child 2 throws %s!\n", RPS(throws[1]));
    //check for winner and print out result for each game
    winner = getwin(throws[0],throws[1]);
    if (winner == -1) {
      printf("Game is a Tie!\n");
      wins[0]++;
    }
    else if (winner == 0) {
      printf("Child 1 Wins!\n");
      wins[1]++;
    }
    else if (winner == 1) {
      printf("Child 2 Wins!\n");
      wins[2]++;
    }
    games = 0;
    pthread_mutex_unlock(&m2);
  }
  pthread_mutex_lock(&m1);
  refsig = 0;
  pthread_cond_broadcast(&ref_cv);
  pthread_mutex_unlock(&m1);
}


//main function
int main(int argc, char **argv){
  p1.p = 0;
  p2.p = 1;
  for (int i = 0; i < 3; i++){
    wins[i] = 0;
  }
  for (int i = 0; i < 2; i++){
    throws[i] = 0;
  }
  // checking argument error
  if(argc != 2){
    perror("invalid agrument\n");
    exit(1);
  }
  turns = atoi(argv[1]);
  //no game played, quit.
  if(turns <= 0){
    printf("No game played\n");
    exit(0);
  }
  //thread create
  if (pthread_create(&tid1, NULL, player, &p1) != 0) {
     perror("pthread_create(1)\n");
     exit(1);
  }
  if (pthread_create(&tid2, NULL, player, &p2) != 0) {
     perror("pthread_create(2)\n");
     exit(1);
  }
  //caling referee function
  ref(turns);
  // joining thread and tally winner
  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);
  printf("------------------------\n");
	printf("------------------------\n");
	printf("Results:\n");
	printf("Child 1: %d\n", wins[1]);
	printf("Child 2: %d\n", wins[2]);
	printf("Ties   : %d\n", wins[0]);
  //print out result
  if(wins[0] == turns){
    printf("Draw!!!\n");
  }
  else if (wins[1] == wins[2]){
    printf("Draw!!!\n");
  }
  else{
    printf("child %d wins!\n", wins[1] > wins[2] ? 1:2);
  }
  //free memories
  pthread_mutex_destroy(&m1);
	pthread_mutex_destroy(&m2);
	pthread_cond_destroy(&ref_cv);
	pthread_cond_destroy(&throw_cv);
	pthread_exit(NULL);
  return 0;
}
