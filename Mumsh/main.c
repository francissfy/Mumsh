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


static char cli_input[MAX_CLI_LEN];


int main(int argc, const char* argv[]) {
    char line[] = "echo 123 | grep 1 | grep 1 | grep 1 >> 2.txt";
    COMMAND_LIST_T* cmd_list = ParseInput(line);
    PrintCMDList(cmd_list);
    
    EXEC_ERROR_T ret_code = ExecCmdList(cmd_list);
    PrintExecErrMsg(ret_code);
    
    return 0;
}
