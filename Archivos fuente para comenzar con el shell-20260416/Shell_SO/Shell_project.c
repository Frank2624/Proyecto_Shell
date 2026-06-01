//------------------------------------------------------------------------------
// Francisco Rosales Arévalo





// UNIX Shell Project
// 
// Sistemas Operativos
// Dept. Arquitectura de Computadores - UMA
// 
// To compile and run the program:
//    $ gcc Shell_project.c parse_line.c list.c job_control.c -o Shell
//    $ ./Shell          
//    ShellSO > 
//     (then type ^D to exit program)
//------------------------------------------------------------------------------

// standard headers
#include <stdio.h>          // printf, stderr, perror, fprintf
#include <stdlib.h>         // malloc, free
//#include <malloc.h>
#include <string.h>         // strcmp
#include <fcntl.h>          // open
#include <unistd.h>         // fork, execvp, tcgetpgrp, dup2, close
//#include <termios.h>
#include <signal.h>         // signal
#include <sys/wait.h>       // waitpid
//#include <sys/types.h>
#include <errno.h>          // errno

// local project headers
#include "parse_line.h"     // link with parse_line.o
#include "job_control.h"    // link with job_control.o and list.o


// -----------------------------------------------------------------------------
//                            Global data structures
// -----------------------------------------------------------------------------
// Declara aqui las variables globales que tengan que ser accedidas desde los
//  manejadores establecidos con signal() o sigaction()

    list_head_t *job_list = NULL;

// -----------------------------------------------------------------------------
// Useful functions to deal with signal handlers and signal masks
// -----------------------------------------------------------------------------
// set a handler (SIG_IGN or SIG_DFL) for signal sent by terminal
void terminal_signals(void (*func)(int))
{
    signal(SIGINT,  func); // crtl+c interrupt tecleado en el terminal
    signal(SIGQUIT, func); // ctrl+\ quit tecleado en el terminal
    signal(SIGTSTP, func); // crtl+z Stop tecleado en el terminal
    signal(SIGTTIN, func); // proceso en segundo plano quiere leer del terminal
    signal(SIGTTOU, func); // proceso en segundo plano quiere escribir en el terminal
}
// -----------------------------------------------------------------------------
// mask or unmask a given signal depending on the block argument
void mask_signal(int signal, int block)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    sigprocmask(block, &mask, NULL); // block: SIG_BLOCK/SIG_UNBLOCK
}
// -----------------------------------------------------------------------------

int subs_autovars(char **argv, int shell_pid, int last_pid, int retval) {

    for (int i=0;argv[i]!=NULL;i++){
        if (strcmp(argv[i],"$$")==0){
            free(argv[i]);                            
            argv[i] = malloc(32 * sizeof(char));         
            sprintf(argv[i], "%d", shell_pid);
        }
        else if (strcmp(argv[i],"$!")==0){
            if (last_pid!=-1){
                free(argv[i]);                            
                argv[i] = malloc(32 * sizeof(char));         
                sprintf(argv[i], "%d", last_pid);
            }
        }
        else if (strcmp(argv[i],"$?")==0){
            if (retval!=-1){
                free(argv[i]);                            
                argv[i] = malloc(32 * sizeof(char));         
                sprintf(argv[i], "%d", retval);
            }
        }
        
        else if (argv[i][0]=='$' && strlen(argv[i])>1){

            char *nombre_var = argv[i] + 1;

            char *valor= getenv(nombre_var);
            free(argv[i]);

            if (valor!=NULL){
                char *nuevo_arg=malloc((strlen(valor)+1)*sizeof(char));
                strcpy(nuevo_arg,valor);
                argv[i]=nuevo_arg;
            }
            else{
                for (int j=i;argv[j]!=NULL;j++){
                    argv[j]=argv[j+1];
                }
                i--;
            }
        }
    }
    
    
    int cont=0;
    while(argv[cont]!=NULL){
        cont++;
    }
    return cont;
}

void handler_singchld(int signal){
    while(1){
        int wstatus;
        int pid_wait= waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);
         
        if (pid_wait <= 0) break;

        mask_signal(SIGCHLD, SIG_BLOCK);
        job *task= get_job_bypid(job_list,pid_wait);
        mask_signal(SIGCHLD, SIG_UNBLOCK); 

        if (task == NULL) continue;

        if (WIFEXITED(wstatus)) {
            
            printf("[%d] (%s) Terminated with status: %d\n", task->pgid,task->command,WEXITSTATUS(wstatus));
            mask_signal(SIGCHLD, SIG_BLOCK);
            del_job(job_list,task);
            free_job(task);
            mask_signal(SIGCHLD, SIG_UNBLOCK); 

        }
                    
         else if (WIFSIGNALED(wstatus)) {
            int sig = WTERMSIG(wstatus);
            printf("[%d] (%s) Signaled by signal: %d\n", task->pgid,task->command, sig);
            mask_signal(SIGCHLD, SIG_BLOCK);
            del_job(job_list,task);
            free_job(task);
            mask_signal(SIGCHLD, SIG_UNBLOCK); 
        } 
                    
        else if (WIFSTOPPED(wstatus)){
            mask_signal(SIGCHLD, SIG_BLOCK);
            task->state=STOPPED;
            mask_signal(SIGCHLD, SIG_UNBLOCK);
            printf("[%d] (%s) Stopped by signal: %d\n", task->pgid,task->command, WSTOPSIG(wstatus));
        } 
                    
        else if(WIFCONTINUED(wstatus)) {
            mask_signal(SIGCHLD, SIG_BLOCK);
            task->state=BACKGROUND;
            mask_signal(SIGCHLD, SIG_UNBLOCK);
            printf("[%d] (%s) continued\n", task->pgid,task->command);
        }
    }
}

// -----------------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------------
int main(void)
{
    char **argv = NULL;
    int argc;
    job_list=new_list("Jobs");
    // probably useful variables:
    int background;             // equals 1 if a command is followed by '&'
    int pid_fork, pid_wait;     // pid for created and waited process
    int wstatus;                // status returned by waitpid
    char *file_in, *file_out;   // for redirections
    int shell_pid=getpid();
    int last_pid=-1;
    int retval=-1;
    signal(SIGCHLD,handler_singchld);

    terminal_signals(SIG_IGN);
    while (1) {
        free_argv(argv);
        int ret = get_command("ShellSO > ", &argc, &argv);
        if (ret == -1) exit(EXIT_FAILURE);      // error in read(2)
        if (ret == 0) break;                    // finish loop if ^D (eof)
        if (argc == 0) continue;                // empty command: next iteration
        argc = parse_comments(argv);
        if (argc == 0) continue; // empty command after parsing comment #
        argc = parse_background(argv, &background);
        if (argc == 0) continue; // empty command after parsing background &
        argc=subs_autovars(argv,shell_pid,last_pid,retval);
        argc = parse_redirections(argv,  &file_in, &file_out);
        if (argc == 0) continue; // empty command after parsing redirections
        
        if (strcmp(argv[0],"quit")==0) break;// comando interno 'quit'
        if (strcmp(argv[0],"exit")==0) {// comando interno 'exit retval'
          int retval=0;
          if (argv[1]==NULL) retval=0;
          else retval=atoi(argv[1]);
          exit(retval);
        
        }
        
        if (strcmp(argv[0],"cd")==0){
            char *dir;

            if (argv[1]==NULL) dir=getenv("HOME");
            else dir=argv[1];

            if (chdir(dir)!=0) perror(argv[0]);
            
            continue;
        }
        if (strcmp(argv[0],"jobs")==0){
            mask_signal(SIGCHLD, SIG_BLOCK);
            print_job_list(job_list);
            mask_signal(SIGCHLD, SIG_UNBLOCK); 

            continue;
        }
        
        if (strcmp(argv[0],"bg")==0){
            job *task;
            mask_signal(SIGCHLD, SIG_BLOCK);
            if (argv[1]==NULL) task=get_item_bypos(job_list,1);
            
            else task= get_item_bypos(job_list,atoi(argv[1]));

            mask_signal(SIGCHLD, SIG_UNBLOCK); 
            
            if (task==NULL) perror("Not found");

            else{
                mask_signal(SIGCHLD, SIG_BLOCK);
                if (task->state == BACKGROUND){
                    printf("[%d] (%s) Already in BACKGROUND", task->pgid,task->command);
                }
                else if (task->state==STOPPED) {
                    task->state=BACKGROUND;
                    killpg(task->pgid,SIGCONT);
                    printf("[%d] (%s) Running in Background\n", task->pgid,task->command);
                }
                mask_signal(SIGCHLD, SIG_UNBLOCK);
            }
            continue;
        }
        
        if (strcmp(argv[0],"fg")==0){
            job *task;
            mask_signal(SIGCHLD, SIG_BLOCK);
            if (argv[1]==NULL) task=get_item_bypos(job_list,1);
            
            else task= get_item_bypos(job_list,atoi(argv[1]));

            mask_signal(SIGCHLD, SIG_UNBLOCK); 
            
            if (task==NULL) perror("Not found");

            else{
                mask_signal(SIGCHLD, SIG_BLOCK);
                task->state=FOREGROUND;
                
                tcsetpgrp(STDIN_FILENO,task->pgid);
                killpg(task->pgid,SIGCONT);
                
                
                pid_wait = waitpid(task->pgid, &wstatus, WUNTRACED);
                tcsetpgrp(STDIN_FILENO,getpid());
                
                if (pid_wait == -1) perror("waitpid");
                
                if (WIFEXITED(wstatus)) {
                    retval = WEXITSTATUS(wstatus);
                    printf("[%d] (%s) Terminated with status: %d\n", task->pgid, task->command, retval);
                    del_job(job_list,task);
                    free_job(task);
                    
                }
                    
                else if (WIFSIGNALED(wstatus)) {
                    int sig = WTERMSIG(wstatus);
                    printf("[%d] (%s) Signaled by signal: %d\n", task->pgid, task->command, sig);
                } 
                    
                else if (WIFSTOPPED(wstatus)){
                    task->state=STOPPED;
                    printf("[%d] (%s) Stopped by signal: %d\n", task->pgid, task->command, WSTOPSIG(wstatus));
                } 
                
                else printf("[%d] (%s) other\n", task->pgid, task->command);
                
                mask_signal(SIGCHLD, SIG_UNBLOCK);
                        
            }
            continue;
        }
        // the steps are:
        // (1) fork a child process using fork()
     	pid_fork=fork();
        switch(pid_fork){
        case -1: perror("fork"); continue;
        
        case 0: //child
        // (2) the child process will invoke execvp()
                setpgid(getpid(),getpid());
                
                if (!background)tcsetpgrp(STDIN_FILENO,getpid());
                terminal_signals(SIG_DFL);

                //Redirections
                if (file_in!=NULL){
                    int f_in=open(file_in,O_RDONLY);

                    if(f_in==-1){
                        perror(file_in);
                        exit(EXIT_FAILURE);
                    }

                    dup2(f_in,STDIN_FILENO);
                    close(f_in);


                }
                if (file_out!=NULL){
                    int f_out=open(file_out,O_CREAT|O_WRONLY|O_TRUNC,0664);

                    if(f_out==-1){
                        perror(file_out);
                        exit(EXIT_FAILURE);
                    }
                    dup2(f_out,STDOUT_FILENO);
                    close(f_out);
                }

                execvp(argv[0],argv);  
                perror(argv[0]);
                exit(EXIT_FAILURE);
        
        default://parent
                setpgid(pid_fork,pid_fork);
                last_pid = pid_fork;
        // (3) if background == 0, the parent will wait, otherwise
                if (!background) {
                    pid_wait = waitpid(pid_fork, &wstatus, WUNTRACED);
                    tcsetpgrp(STDIN_FILENO,getpid());
                    
                    if (pid_wait == -1) perror("waitpid");
        // (4) Shell shows a status message for processed command 
                    if (WIFEXITED(wstatus)) {
                        retval = WEXITSTATUS(wstatus);
                        printf("[%d] (%s) Terminated with status: %d\n", pid_fork, argv[0], retval);
                    }
                    
                    else if (WIFSIGNALED(wstatus)) {
                        int sig = WTERMSIG(wstatus);
                        printf("[%d] (%s) Signaled by signal: %d\n", pid_fork, argv[0], sig);
                    } 
                    
                    else if (WIFSTOPPED(wstatus)){
                        job *task= new_job(pid_fork,argv[0],STOPPED);
                        mask_signal(SIGCHLD, SIG_BLOCK);
                        add_job(job_list,task);
                        mask_signal(SIGCHLD, SIG_UNBLOCK); 
                        printf("[%d] (%s) Stopped by signal: %d\n", pid_fork, argv[0], WSTOPSIG(wstatus));
                    } 
                    
                    else printf("[%d] (%s) other\n", pid_fork, argv[0]);
                    

               } else{
                    job *task= new_job(pid_fork,argv[0],BACKGROUND);
                    mask_signal(SIGCHLD, SIG_BLOCK);
                    add_job(job_list,task);
                    mask_signal(SIGCHLD, SIG_UNBLOCK); 
                   printf("[%d] (%s) Running in Background\n", pid_fork, argv[0]);
               } 
            
        // (5) loop ret
        }
    } // end while
    printf("\nBye\n");
    return 0;
}



