//
//  main.c
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<string.h>

#include "parser.h"
#include "exec.h"


static char cli_buffer[1024];
static int nchars = 0;
TASK_POOL_T task_pool;



void PrintPrompt(void) {
    printf("mumsh $ ");
    fflush(stdout);
}


// very strange way, use different handling function for SIGINT
void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("\n");
        cli_buffer[0] = '\0';
        nchars = 0;
        PrintPrompt();
    }
}


void signal_handler_2(int signal) {
    if (signal == SIGINT) {
        printf("\n");
    }
}


// check whether redirection and pipe are complete
int check_rdct_pipe_complete(void) {
    if (nchars == 0) {
        return 1;
    }
    // check incomplete pipe
    int i=nchars-1;
    while (i>0 && cli_buffer[i] == ' ') {
        i--;
    }
    if (cli_buffer[i] == '|' || cli_buffer[i] == '<' || cli_buffer[i] == '>') {
        return 0;
    }
    return 1;
}


// check whether quotes are complete
int check_quote_complete(void) {
    if (nchars == 0) {
        return 1;
    }
    CHAR_STACK stack;
    stack.stack_count = 0;
    for (int i=0; i<nchars; i++) {
        if (cli_buffer[i] == '\'' || cli_buffer[i] == '\"') {
            if (CharStackEmpty(&stack) || CharStackTop(&stack) != cli_buffer[i]) {
                // record
                CharStackPush(&stack, cli_buffer[i]);
            } else if (CharStackTop(&stack) == cli_buffer[i]) {
                // encounter the same, not cp, reset quote_ptr
                CharStackPop(&stack);
            }
        }
    }
    return CharStackEmpty(&stack);
}


// remove the quotes in string
void rm_quotes(void) {
    int cp_offset = 0;
    int scan_offset = 0;
    char quote = '\0';
    while (scan_offset<nchars) {
        char t = cli_buffer[scan_offset];
        if (t == '\'' || t == '\"') {
            if (quote == '\0') {
                // record and scan++
                quote = t;
            } else {
                if (quote == t) {
                    // encounter the same, not cp, reset quote_ptr
                    quote = '\0';
                } else {
                    // not the same, cp
                    if (scan_offset > cp_offset) {
                        cli_buffer[cp_offset++] = t;
                    }
                }
            }
        } else {
            // cp
            if (scan_offset > cp_offset) {
                cli_buffer[cp_offset] = t;
            }
            cp_offset++;
        }
        scan_offset += 1;
    }
    cli_buffer[cp_offset] = '\0';
}


int main() {
    InitTaskPool(&task_pool);
    
    char c_tmp;
    
    while (1) {
        // handle control+c signal on incomplete command
        signal(SIGINT, signal_handler);
        
        PrintPrompt();
        //
        while (1) {
            c_tmp = getchar();
            if (c_tmp == '\n') {
                // handle \n
                if (nchars == 0) {
                    PrintPrompt();
                    continue;
                }
                if (check_quote_complete() && check_rdct_pipe_complete()) {
                    // exec cmd
                    rm_quotes();
                    cli_buffer[nchars] = '\0';
                    break;
                } else if (!check_rdct_pipe_complete()){
                    // pipe incomplete, ignore \n
                    printf("> ");
                    continue;
                } else {
                    // quote incomplete
                    cli_buffer[nchars++] = '\n';
                    printf("> ");
                    continue;
                }
            } else if (c_tmp == EOF) {
                // handle EOF
                if (nchars == 0) {
                    printf("exit\n");
                    return 0;
                } else {
                    // clear EOF status in stdin
                    clearerr(stdin);
                    continue;
                }
            } else {
                // handle other character
                cli_buffer[nchars++] = c_tmp;
            }
        }
        
        // to handle the control+c signal when piping
        signal(SIGINT, signal_handler_2);
        
        // exec
        if (strcmp(cli_buffer, "exit") == 0) {
            printf("exit\n");
            // clean all jobs
            FreePool(&task_pool);
            return 0;
        }
        
        // jobs
        if (strcmp(cli_buffer, "jobs") == 0) {
            PrintJobs(&task_pool);
            // clear buffer
            cli_buffer[0] = '\0';
            nchars = 0;
            continue;
        }
        
        COMMAND_LIST_T* cmd_list = ParseInput(cli_buffer);
        // PrintCMDList(cmd_list);
        ExecCmdList(cmd_list);
        // PrintCMDList(cmd_list);
        // PrintExecErrMsg(ret_code);
        if (cmd_list->job_type == JOB_BACKGOUND) {
            AddTaskToPool(&task_pool, cmd_list);
            PrintBackgoundTopJobs(&task_pool);
        } else {
            FreeCmdList(cmd_list);
        }
        nchars = 0;
    }
    return 0;
}
