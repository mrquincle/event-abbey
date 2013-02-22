/**
 * The event-based abbey uses setjmp, longjmp mechanisms to switch from one
 * monk to another. Switching is very quick.
 * 
 * There are three things that remain to be done. First of all, there have to 
 * be mechanisms to handle IO. Why? Because another monk can go to work after 
 * one monk blocks. Only when all monks block, the process has to be blocked.
 * This means 1.) that there should be no state info stored before IO events.
 * 2.) or that the jmp_buf is misused to communicate integers. 3.) and that 
 * there should be an "abt" like monk, that also gives control at more or less
 * frequent times to blocking monks.
 * Secondly, there should be a mechanism such that the yielding process can be
 * set to finer and coarser granular scales. Certain large tasks should be first
 * decomposed in smaller tasks. However, when they do fine for a long time, they
 * certainly do not have to yield each and every time.
 * Thirdly, and again, large tasks should be decomposed in smaller tasks. Because
 * we do not have state information, it should be in the form of calls to subtasks.
 * However, when a task dispatches another task, there are 3 important things: 1.)
 * it should not overwrite its stack space, 2.) the forest of tasks that are 
 * dispatched should not be too large, but in regard to the task buffer, 3.) those
 * subtasks just like subroutines would need to return values.   
 */
 
#include  <stdio.h>
#include  <stdlib.h>
#include  <setjmp.h>

//! Task slot is ready to be filled with new task
#define TS_FREE         0
//! Task slot is ready to be executed
#define TS_EXECUTEME    1 
//! Task slot is being executed, not necessary in cooperative environment
#define TS_EXECUTING    2 

#define MAX_COUNT 8

/**
 * The task contains a state flag, and the method to be evoked. 
 */
typedef struct taskStruct
{
  //! The state field flags if a task slot is occupied, a monk is busy, etc.  
  int state;
  //! A pointer to a function to be executed.
  void *(*func)(void *);
  //! A void pointer to the arguments of the to be executed function.
  void *context;
} Task;

typedef struct monkStruct
{
  //! Numbername
  unsigned int number;
  //! Execution context, the environment (stack pointer)
  jmp_buf environment;
} Monk;

//! An array of tasks
static Task *task;
static Monk *monk;

jmp_buf   MAIN;                    /* jump buffer for main */
jmp_buf   SCHEDULER;               /* for the scheduler    */
                         
static int taskCount, monkCount;
static int currentMonk = -1;
static int timesScheduled = MAX_COUNT;
static int value1 = 16, *p_value1 = &value1;
int value2 = 19, *p_value2 = &value2;
  
static void *monking(void *arg) 
{
  int i; int index = *(int *) arg;
  printf("Initialize monk nr. %d and jump back to start_abbey.\n", index);
  index = setjmp(monk[index].environment) - 1;
  if (index == -1)     
    longjmp(MAIN, 1);     
  printf("Iterate tasks as monk nr. %d.\n", index);
  for(i = 0; i < taskCount; i++) {
    if(task[i].state == TS_EXECUTEME) {
      printf("Execute task at slot %d.\n", i);
      task[i].func(task[i].context);
      task[i].state = TS_FREE;
      //yield with: "if (setjmp(monk[index].environment) == 0) then"
      //it then returns    
      longjmp(SCHEDULER, 1); 
    }
  }
  printf("No task found, jump back to scheduler.\n");
  longjmp(SCHEDULER, 1); 
}
                              
/*!
 * The scheduler initializes to setup a jump buffer to it. Then it jumps to the
 * first monk; 
 */
void scheduler(void)
{
  printf("Initializing scheduler.\n");
  if (setjmp(SCHEDULER) == 0)
    longjmp(MAIN, 1);
  if (timesScheduled-- == 0) {
    printf("\nJust quit when x times scheduled in this demo.\n");
    exit(0);
  }
  
  currentMonk++;
  currentMonk %= monkCount;  
  printf("Jump to monk %d.\n", currentMonk);
  longjmp(monk[currentMonk].environment, currentMonk+1);
}
                  
/*! \brief
 * The abbey initializes a number of monks and an array of tasks.
 * 
 */

int initialize_abbey(int nofMonks, int nofTasks) 
{
  printf("The abbey is initialized with %d monks and a buffer for %d tasks.\n",
    nofMonks, nofTasks);
  taskCount = nofTasks; monkCount = nofMonks;
  task = (Task *)calloc(taskCount, sizeof(Task));
  monk = (Monk *)calloc(monkCount, sizeof(Monk));
  return 0;
}

/*! \brief
 * The abbey is started: the jumping acrobatics as a monkey from monk to 
 * monk is started.
 * 
 * There are two options. 1.) Put start_abbey in a different thread. So, 
 * there is a place from where tasks can be dispatched. 2.) Evoke 
 * start_abbey with the first task to be dispatched. From that task other
 * tasks are dispatched ad infinitum.  
 */
int start_abbey() 
{
  int i = 0; //int* j = &i; 
  printf("\nThe abbey is started...\n");
  
  //create jmp_buffer for main, to have one for the scheduler too
  printf("Jump to scheduler.\n");
  if (setjmp(MAIN) == 0)   
    scheduler();
    
  printf("Initialize monks.\n");  
  for (i = 0; i < monkCount; i++) {
    monk[i].number = i;
    if (setjmp(MAIN) == 0)  
      monking(&monk[i].number);
  }  
  
  printf("Start scheduling.\n\n");
  longjmp(SCHEDULER,1);
}

int dispatch_task(void *(*func)(void *), void *context)
{
  int i;
  printf("!! Task dispatched.\n");
  for(i = 0; i < taskCount; i++) {
    if(task[i].state == TS_FREE) {
      task[i].func = func;
      task[i].context = context;
      task[i].state = TS_EXECUTEME;
      printf("Task put in buffer to be executed.\n");
      return 0;
    }
  } 
  return 1;
}

void *inc(void* context) 
{
  int value = *(int *) context;
  value++;
  printf("The value is incremented to %d.\n", value);
  return NULL;
}

/*!
 * This method is executed as a task. It is very clarifying to see that local
 * variables are not preserved. The integers value1 and value2 are global 
 * variables, and their value is preserved under all the jump actions on the 
 * stack. However, value3 is a local variable. Incrementing it gives as result
 * -1076728219 on my machine.
 */
void *real_main(void * context) {
  int value3 = 22, *p_value3 = &value3;
  printf("\nDispatch increment tasks.\n");
  dispatch_task(inc, p_value1);
  dispatch_task(inc, p_value2);
  dispatch_task(inc, p_value3);
  printf("Tasks dispatched. You can see that the actual execution of those ");
  printf("tasks is later on.\n\n");
  return NULL;
}

/*! \brief
 * A bootstrapping (pseudo-)main. Initializes and starts the abbey.  
 * 
 * In the real_main method the first task is dispatched. This task will
 * dispatch others, etc.
 */
int main() 
{
  printf("---------------------\n");
  printf("Starting the engines!\n");
  printf("---------------------\n");
  initialize_abbey(2, 4);
  printf("Dispatch main task.\n");
  dispatch_task(real_main, NULL);
  printf("Main task dispatched.\n");
  start_abbey();
  //what follows is unreachable, abbey starts scheduling forever
  printf("Hi, I will never be printed FWIW.\n");
  return 0;
}


