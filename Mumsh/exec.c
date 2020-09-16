//
//  exec.c
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "exec.h"

EXEC_ERROR_T ExecCmd(COMMAND_T* cmd, int* pre_fd, int* cur_fd) {
    EXEC_ERROR_T ret_code = EXEC_OK;
    
    // check command valibility
    if (cmd->io_input->file_count>1 || cmd->io_output->file_count>1) {
        ret_code = EXEC_TOO_MANY_FILE_IO;
        return ret_code;
    }
    
    pid_t child_pid = fork();
    
    if (child_pid<0) {
        ret_code = EXEC_FORK_ERROR;
        return ret_code;
    } else if (child_pid==0) {
        // child process
        
        // TODO: do pwd etc cmd manually
        
        // config input io
        int fin = -1;
        if (cmd->io_input->io_type == STD_IO) {
            close(pre_fd[1]);
            fin = pre_fd[0];
        } else {
            close(pre_fd[1]);
            fin = open(cmd->io_input->file_list[0], O_RDONLY);
        }
        if (fin<0) {
            ret_code = EXEC_FILE_NOT_EXIST;
            return ret_code;
        }
        dup2(fin, STDIN_FILENO);
        
        // config output io
        assert(cmd->io_output->file_count <= 1);
        int fout = -1;
        if (cmd->io_output->io_type == STD_IO) {
            close(cur_fd[0]);
            fout = cur_fd[1];
        } else {
            close(cur_fd[0]);
            if (cmd->io_output->io_type == FILE_IO) {
                fout = open(cmd->io_output->file_list[0], O_CREAT | O_TRUNC | O_WRONLY, 0644);
            } else {
                fout = open(cmd->io_output->file_list[0], O_CREAT | O_APPEND | O_WRONLY, 0644);
            }
        }
        if (fout<0) {
            ret_code = EXEC_FILE_PERMISSION_DENY;
            return ret_code;
        }
        dup2(fout, STDOUT_FILENO);
        
        execvp(cmd->argv[0], cmd->argv);
        ret_code = EXEC_CMD_NOT_FOUND;
        return ret_code;
    } else {
        // parent process
        close(pre_fd[0]);
        close(pre_fd[1]);
        waitpid(child_pid, NULL, 0);
    }
    return ret_code;
}


EXEC_ERROR_T ExecCmdList(COMMAND_LIST_T* cmd_list) {
    EXEC_ERROR_T ret_code = EXEC_OK;
    
    int** pipe_list = (int**)malloc(sizeof(int*)*(cmd_list->cmd_count+1));
    for (int i=0; i<(cmd_list->cmd_count+1); i++) {
        int* pipe_fd = (int*)malloc(sizeof(int)*2);
        
        if (pipe(pipe_fd)<0) {
            ret_code = EXEC_PIPE_ERROR;
        }
        pipe_list[i] = pipe_fd;
    }
    
    if (ret_code == EXEC_OK) {
        close(pipe_list[0][1]);
        close(pipe_list[cmd_list->cmd_count][1]);
        dup2(STDOUT_FILENO, pipe_list[cmd_list->cmd_count][0]);
        
        for (int i=0; i<cmd_list->cmd_count; i++) {
            ret_code = ExecCmd(cmd_list->cmd_list[i], pipe_list[i], pipe_list[i+1]);
            if (ret_code != EXEC_OK) {
                break;
            }
        }
    }
    
    for (int i=0; i<(cmd_list->cmd_count+1); i++) {
        free(pipe_list[i]);
    }
    free(pipe_list);
    return ret_code;
}
