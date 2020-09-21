//
//  exec.c
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "exec.h"


EXEC_ERROR_T get_cwd(char** cwd_ptr) {
    char* res = getcwd(buffer, MAX_BUFFER_SIZE);
    if (res == NULL) {
        fprintf(stderr, "ERROR: cwd: buffer overflow, max length: %ld!\n", MAX_BUFFER_SIZE);
        return EXEC_BUFFER_OVERFLOW;
    } else {
        *cwd_ptr = buffer;
    }
    return EXEC_OK;
}


EXEC_ERROR_T pwd() {
    char* wd;   // ptr to buffer
    EXEC_ERROR_T ret_code = get_cwd(&wd);
    if (ret_code == EXEC_OK) {
        printf("%s\n", wd);
    }
    return ret_code;
}


// don't acquire manual free
EXEC_ERROR_T get_hd(char** hd) {
    // home directory
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        fprintf(stderr, "ERROR: cd: cannot get home directory!\n");
        return EXEC_CANNOT_GET_HOME_DIR;
    } else {
        *hd = pw->pw_dir;
    }
    return EXEC_OK;
}


EXEC_ERROR_T cd(char* path) {
    EXEC_ERROR_T ret_code = EXEC_OK;
    
    char* abs_path;
    char* prefix;
    size_t s2 = strlen(path);
    if (path[0] == '/') {
        // absolute path, pass
        abs_path = (char*)malloc(sizeof(char)*(s2+1));
        memcpy(abs_path, path, s2+1);
    } else if (path[0] == '~') {
        // home directory prefix
        ret_code = get_hd(&prefix);
        size_t s1 = strlen(prefix);
        if (ret_code != EXEC_OK) {
            return ret_code;
        }
        abs_path = (char*)malloc(sizeof(char)*(s1+s2));
        memcpy(abs_path, prefix, s1);
        memcpy(abs_path+s1, path+1, s2);
    } else {
        ret_code = get_cwd(&prefix);
        size_t s1 = strlen(prefix);
        if (ret_code != EXEC_OK) {
            return ret_code;
        }
        abs_path = (char*)malloc(sizeof(char)*(s1+s2+2));
        memcpy(abs_path, prefix, s1);
        abs_path[s1] = '/';
        memcpy(abs_path+s1+1, path, s2+1);
    }
    
    if (chdir(abs_path) != 0) {
        ret_code = EXEC_CHDIR_ERROR;
    }
    free(abs_path);
    return ret_code;
}


void ExecCmd(COMMAND_LIST_T* cmd_list, int** pipe_list) {
    EXEC_ERROR_T ret_code = EXEC_OK;
    
    if (cmd_list->cursor == cmd_list->cmd_count) {
        return ;
    }
    COMMAND_T* cmd = cmd_list->cmd_list[cmd_list->cursor];
    
    int* pre_fd = pipe_list[cmd_list->cursor];
    int* cur_fd = pipe_list[cmd_list->cursor+1];
    
    if (cmd->parse_error != PARSE_OK) {
        ret_code = EXEC_CMD_PARSE_ERROR;
        PrintExecErrMsg(ret_code);
        return ;
    }
    
    // process cd cmd in main process
    if (strcmp(cmd->argv[0], "cd") == 0) {
        ret_code = cd(cmd->argv[1]);
        PrintExecErrMsg(ret_code);
        return ;
    }
    
    pid_t child_pid = fork();
    
    if (child_pid<0) {
        ret_code = EXEC_FORK_ERROR;
        PrintExecErrMsg(ret_code);
        return ;
    } else if (child_pid==0) {
        // child process
        
        // config input io
        int fin = -1;
        if (cmd->io_input->io_type == STD_IO) {
            close(pre_fd[1]);
            fin = pre_fd[0];
        } else {
            close(pre_fd[1]);
            fin = open(cmd->io_input->file, O_RDONLY);
        }
        if (fin<0) {
            fprintf(stderr, "ERROR: %s: file not exists!\n", cmd->io_input->file);
            ret_code = EXEC_FILE_NOT_EXIST;
            exit(ret_code);
        }
        dup2(fin, STDIN_FILENO);
        
        // config output io
        int fout = -1;
        if (cmd->io_output->io_type == STD_IO) {
            close(cur_fd[0]);
            fout = cur_fd[1];
        } else {
            close(cur_fd[0]);
            if (cmd->io_output->io_type == FILE_IO) {
                fout = open(cmd->io_output->file, O_CREAT | O_TRUNC | O_WRONLY, 0644);
            } else {
                fout = open(cmd->io_output->file, O_CREAT | O_APPEND | O_WRONLY, 0644);
            }
        }
        if (fout<0) {
            fprintf(stderr, "ERROR: %s: permission denied!\n", cmd->io_output->file);
            ret_code = EXEC_FILE_PERMISSION_DENY;
            exit(ret_code);
        }
        dup2(fout, STDOUT_FILENO);
        
        // pwd
        if (strcmp(cmd->argv[0], "pwd") == 0) {
            ret_code = pwd();
            exit(ret_code);
        }
        
        int err_code = execvp(cmd->argv[0], cmd->argv);
        if (err_code == -1) {
            ret_code = EXEC_COMMAND_NOT_FOUND;
            fprintf(stderr, "ERROR: %s: command not found!\n", cmd->argv[0]);
            exit(ret_code);
        }
    } else {
        // parent process
        close(pre_fd[0]);
        close(pre_fd[1]);
        
        // fill pid
        cmd->job_pid = child_pid;
        
        // launch next cmd before waitpid
        cmd_list->cursor += 1;
        ExecCmd(cmd_list, pipe_list);
        
        if (cmd_list->job_type == JOB_FOREGOUND) {
            int tmp;
            waitpid(child_pid, &tmp, 0);
            ret_code = (EXEC_ERROR_T)WEXITSTATUS(tmp);
        }
    }
    
    // print out error message
    if (ret_code != EXEC_OK) {
        PrintExecErrMsg(ret_code);
    }
    return ;
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
        // close the read end of pipe1
        // provide stdin to pipe
        close(pipe_list[0][0]);
        dup2(STDIN_FILENO, pipe_list[0][1]);
        // close the write end of last pipe
        close(pipe_list[cmd_list->cmd_count][1]);
        dup2(STDOUT_FILENO, pipe_list[cmd_list->cmd_count][0]);
        
        ExecCmd(cmd_list, pipe_list);
    }
    
    for (int i=0; i<(cmd_list->cmd_count+1); i++) {
        free(pipe_list[i]);
    }
    free(pipe_list);
    return ret_code;
}
