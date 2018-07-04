#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <semaphore.h>

#define EAT 0
#define THINK 1
#define CHANGE 2

#define SHARED 0
#define ONE_FORK 1

#define ERROR 1

void *philosopher_main(void *id);
void eat(int id);
void think(int id);
void dawdle();
void wait(sem_t *sem);
void post(sem_t *sem);
void print_header();
void print_status();
void print_status_helper();
int check_cmd_line_args(int argc, char *argv[]);
int destroy_sems(int active);

int cycles;
sem_t forks[NUM_PHILOSOPHERS];
int forks_used[NUM_PHILOSOPHERS];
int status[NUM_PHILOSOPHERS];
sem_t print_sem;

int main(int argc, char *argv[])
{
   int i;
   int id[NUM_PHILOSOPHERS];
   pthread_t childid[NUM_PHILOSOPHERS];
   
   /* Check to make sure cmd line arguments are valid, also sets # cycles */
   if(check_cmd_line_args(argc, argv))
   {
      fprintf(stderr, "Usage: dine [# times each philo eats]\n");
      exit(ERROR);
   }
   
   /* If there are not enough philosophers to dine properly, exit */
   if (NUM_PHILOSOPHERS < 2)
   {
      fprintf(stderr, "%d philosopher(s) cannot dine. ", NUM_PHILOSOPHERS);
      fprintf(stderr, "Find more philosophers to dine.\n");
      exit(ERROR);
   }
   
   /* Print the header of the output */
   print_header();
   
   /* Initialize the semaphore that allows philosophers to call print */
   if (sem_init(&print_sem, SHARED, ONE_FORK))
   {
      fprintf(stderr, "print_sem: ");
      perror("");
      exit(ERROR);
   }
   
   /* For each Philosopher */
   for (i = 0; i < NUM_PHILOSOPHERS; i++)
   {
      /* Assign id of Philosopher */
      id[i] = i;
      
      /* Philosopher begins in the CHANGE status */
      status[i] = CHANGE;
   }
   
   /* For each fork */
   for (i = 0; i < NUM_PHILOSOPHERS; i++)
   {
      /* Assign the fork to no Philosopher, for now */
      forks_used[i] = -1;
      
      /* Initialize the fork semaphore, which allows only 1 Philosopher
       * to use this fork at once */
      if (sem_init(&forks[i], 0, 1))
      {
         fprintf(stderr, "Semaphore %d: ", i);
         perror("");
         destroy_sems(i);
         exit(ERROR);
      }
   }
   
   /* Create a new thread for each philo that redirects to philosopher_main */
   for (i = 0; i < NUM_PHILOSOPHERS; i++)
   {
      /* If creation of thread fails, inform user */
      if (pthread_create(&childid[i], NULL, philosopher_main,
         (void *) (&id[i])) == -1)
      {
         fprintf(stderr, "Child %d: ", id[i]);
         perror("");
      }
   }
   
   /* Parent waits for all the philos to finish eating */
   for (i = 0; i < NUM_PHILOSOPHERS; i++)
   {
      int e_val = pthread_join(childid[i], NULL);
      if (e_val)
      {
         fprintf(stderr, "Philosopher %d: Exited with status %d\n", i, e_val);
      }
   }
   
   /* Destroy all the semaphores, exit with an error if one or more
    * does not destroy properly */
   if (destroy_sems(NUM_PHILOSOPHERS))
   {
      exit(ERROR);
   }
   
   return 0;
}

/* Function that each child process immediately begins at */
void *philosopher_main(void *id)
{
   int i;
   int my_id = *(int*)id;
   
   /* Philosopher eats, then thinks [cycles] number of times */
   for (i = 0; i < cycles; i++)
   {
      eat(my_id);
      think(my_id);
   }
   
   return 0;
}

/* Philosopher will pick up each fork, eat, then set them back down */
void eat(int id)
{
   int right = (id + 1) % NUM_PHILOSOPHERS;
   int left = id % NUM_PHILOSOPHERS;
   sem_t *right_fork = &forks[right];
   sem_t *left_fork  = &forks[left];
   
   /* If odd philosopher, pick up left fork, then right fork */
   if (id % 2) 
   {
      /* Philosopher picks up left fork */
      wait(left_fork);
      forks_used[left] = id;
      print_status();
      
      /* Philosopher picks up right fork */
      wait(right_fork);
      forks_used[right] = id;
      print_status();
   }
   /* If even philosopher, pick up right fork, then left fork */
   else
   {
      /* Philosopher picks up right fork */
      wait(right_fork);
      forks_used[right] = id;
      print_status();
      
      /* Philosopher picks up left fork */
      wait(left_fork);
      forks_used[left] = id;
      print_status();
   }
   
   /* Philosopher has both forks and eats */
   status[id] = EAT;
   print_status();
   
   dawdle();
   
   /* Philosopher finishes eating */
   status[id] = CHANGE;
   print_status();

   /* Philosopher drops right fork */
   forks_used[right] = -1;
   post(right_fork);
   print_status();
   
   /* Philosopher drops left fork */
   forks_used[left] = -1;
   post(left_fork);
   print_status();
}

/* Philosopher will think */
void think(int id)
{
   /* Philosopher begins thinking */
   status[id] = THINK;
   print_status();
   
   dawdle();
   
   /* Philosopher stops thinking */
   status[id] = CHANGE;
   print_status();
}

/* sem_wait helper function */
void wait(sem_t *sem)
{
   if (sem_wait(sem))
   {
      fprintf(stderr, "Sem: ");
      perror("");
      destroy_sems(NUM_PHILOSOPHERS);
      exit(ERROR);
   }
}

/* sem_post helper function */
void post(sem_t *sem)
{
   if (sem_post(sem))
   {
      fprintf(stderr, "Sem: ");
      perror("");
      destroy_sems(NUM_PHILOSOPHERS);
      exit(ERROR);
   }
}

/* Philosopher will wait a random amount of time while thinking or eating */
void dawdle()
{
   struct timespec tv;
   int msec = (int)(((double)random() / RAND_MAX) * 1000);
   tv.tv_sec = 0;
   tv.tv_nsec = 1000000 * msec;
   
   if (-1 == nanosleep(&tv, NULL)) 
   {
      perror("nanosleep");
   }
}

/* Prints the header of the output */
void print_header()
{
   int i;
   int k;
   
   /* Print top border, getting wider as NUM_PHILOSOPHERS increases */
   for (i = 0; i < NUM_PHILOSOPHERS; i++)
   {
      printf("|========");
      for (k = 0; k < NUM_PHILOSOPHERS; k++)
      {
         printf("=");
      }
   }
   
   printf("|\n");
   
   /* Print Philosopher designations, from A - Z, then ASCII chars after Z */
   for (i = 0; i < NUM_PHILOSOPHERS; i++)
   {
      printf("|");
      for (k = 0; k < NUM_PHILOSOPHERS / 2 + (NUM_PHILOSOPHERS % 2); k++)
      {
         printf(" ");
      }
      printf("   %c    ", i + 65);
      for (k = 0; k < NUM_PHILOSOPHERS / 2; k++)
      {
         printf(" ");
      }
   }
   
   printf("|\n");
   
   /* Print bottom border, getting wider as NUM_PHILOSOPHERS increases */
   for (i = 0; i < NUM_PHILOSOPHERS; i++)
   {
      printf("|========");
      for (k = 0; k < NUM_PHILOSOPHERS; k++)
      {
         printf("=");
      }
   }
   
   printf("|\n");
}

/* Waits if a thread is already printing, then prints status */
void print_status()
{
   wait(&print_sem);
   print_status_helper();
   post(&print_sem);
}

/* Prints status of all the Philosophers and forks */
void print_status_helper()
{
   int t_id;
   int f_id;
   
   /* For each Philosopher, print forks being held and whether they
    * are eating, thinking, or changing */
   for (t_id = 0; t_id < NUM_PHILOSOPHERS; t_id++)
   {  
      printf("| ");
      
      /* For each fork, print whether this Philosopher is holding it */
      for (f_id = 0; f_id < NUM_PHILOSOPHERS; f_id++)
      {
         if(forks_used[f_id] == t_id)
         {
            printf("%d", f_id % 10);
         }
         else
         {
            printf("-");
         }
      }
      
      /* Philosopher is eating */
      if (status[t_id] == EAT)
      {
         printf(" Eat   ");
      }
      
      /* Philosopher is thinking */
      else if (status[t_id] == THINK)
      {
         printf(" Think ");
      }
      
      /* Philosopher is changing */
      else
      {
         printf("       ");
      }
   }
   printf("|\n");
}

/* Make sure command line args are valid */
int check_cmd_line_args(int argc, char *argv[])
{
   /* If there are no arguments, cycles defaults to 1 */
   if (argc <= 1)
   {
      cycles = 1;
   }
   /* If there are 2 arguments, set cycles to user defined value */
   else if (argc == 2)
   {
      /* If the argument is not an integer, exit */
      if (!sscanf(argv[1], "%d", &cycles))
      {
         return 1;
      }
   }
   /* There should not be more than 2 arguments */
   else
   {
      return 1;
   }
   /* return successfully */
   return 0;
}

/* Destroy all the semaphores */
int destroy_sems(int active)
{
   int i;
   int ret = 0;
   
   /* Destroy each fork semaphore, even if the previous one fails to die */
   for (i = 0; i < active; i++)
   {
      if (sem_destroy(&forks[i]))
      {
         ret = 1;
         fprintf(stderr, "Philo semaphore %d: ", i);
         perror("");
      }
   }
   
   /* Destroy the print semaphore */
   if (sem_destroy(&print_sem))
   {
      ret = 1;
      fprintf(stderr, "Print semaphore: ");
      perror("");
   }
   
   return ret;
}
