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

static const int MAX_CLI_BUFFER = 1024;
static char cli_buffer[MAX_CLI_BUFFER];


int main(int argc, const char* argv[]) {
    
    while (1) {
        printf("$ ");
        fflush(stdout);
        size_t nbytes = read(STDIN_FILENO, cli_buffer, MAX_CLI_BUFFER);
        if (nbytes<0) {
            return -1;
        }
        //
        cli_buffer[nbytes-1] = '\0';
        if (strcmp(cli_buffer, "exit") == 0) {
            return 0;
        }
        // printf("%s\n", cli_buffer);
        
        COMMAND_LIST_T* cmd_list = ParseInput(cli_buffer);
        // PrintCMDList(cmd_list);
        EXEC_ERROR_T ret_code = ExecCmdList(cmd_list);
        PrintExecErrMsg(ret_code);
        FreeCmdList(cmd_list);
    }
    
    return 0;
}
