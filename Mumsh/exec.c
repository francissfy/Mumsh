//
//  exec.c
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "exec.h"


EXEC_ERROR_T pwd() {
    EXEC_ERROR_T ret_code = EXEC_OK;
    char* res = getcwd(buffer, MAX_BUFFER_SIZE);
    if (res == NULL) {
        fprintf(stderr, "ERROR: pwd-buffer overflow, max length: %ld!\n", MAX_BUFFER_SIZE);
        ret_code = EXEC_BUFFER_OVERFLOW;
    } else {
        printf("%s\n", buffer);
    }
    return ret_code;
}

EXEC_ERROR_T cd(char* path) {
    EXEC_ERROR_T ret_code = EXEC_OK;
    // process absolute path, store pwd using buffer
    char* abs_path = path;
    if (path[0] == '/') {
        if (chdir(abs_path) != 0) {
            ret_code = EXEC_CHDIR_ERROR;
        }
    } else if (path[0] == '~') {
        // home directory
        uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);
        if (pw == NULL) {
            ret_code = EXEC_CHDIR_ERROR;
        } else {
            char* hd = pw->pw_dir;
            size_t s1 = strlen(hd);
            size_t s2 = strlen(path);
            abs_path = (char*)malloc(sizeof(char)*(s1+s2));
            memcpy(abs_path, hd, s1);
            memcpy(abs_path+strlen(hd), path+1, s2);
            printf("abs for home: %s\n", abs_path);
            if (chdir(abs_path) != 0) {
                ret_code = EXEC_CHDIR_ERROR;
            }
            free(abs_path);
        }
    } else if (strcmp(path, ".")==0 || strncmp(path, "./", 2)==0) {
        // current wd
        char* res = getcwd(buffer, MAX_BUFFER_SIZE);
        if (res == NULL) {
            ret_code = EXEC_CHDIR_ERROR;
        }
        size_t s1 = strlen(buffer);
        size_t s2 = strlen(path);
        abs_path = (char*)malloc(sizeof(char)*(s1+s2));
        memcpy(abs_path, buffer, s1);
        memcpy(abs_path+s1, path+1, s2);
        printf("abs for cwd: %s\n", abs_path);
        if (chdir(abs_path) != 0) {
            ret_code = EXEC_CHDIR_ERROR;
        }
        free(abs_path);
    } else if (strncmp(path, "..", 2) == 0) {
        // parent wd
        char* res = getcwd(buffer, MAX_BUFFER_SIZE);
        if (res == NULL) {
            ret_code = EXEC_CHDIR_ERROR;
        }
        size_t s1 = strlen(buffer);
        size_t s2 = strlen(path);
        abs_path = (char*)malloc(sizeof(char)*(s1+s2+2));
        memcpy(abs_path, buffer, s1);
        abs_path[s1] = '/';
        memcpy(abs_path+s1+1, path, s2+1);
        printf("abs for parent wd: %s\n", abs_path);
        if (chdir(abs_path) != 0) {
            ret_code = EXEC_CHDIR_ERROR;
        }
        free(abs_path);
    } else {
        // omg
    }
    return ret_code;
}


EXEC_ERROR_T ExecCmd(COMMAND_T* cmd, int* pre_fd, int* cur_fd) {
    EXEC_ERROR_T ret_code = EXEC_OK;
    
    // check command valibility
    if (cmd->io_input->file_count>1 || cmd->io_output->file_count>1) {
        ret_code = EXEC_TOO_MANY_FILE_IO;
        return ret_code;
    }
    
    // process cd cmd in main process
    if (strcmp(cmd->argv[0], "cd") == 0) {
        ret_code = cd(cmd->argv[1]);
        return ret_code;
    }
    
    pid_t child_pid = fork();
    
    if (child_pid<0) {
        ret_code = EXEC_FORK_ERROR;
        return ret_code;
    } else if (child_pid==0) {
        // child process
        
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
            exit(ret_code);
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
            exit(ret_code);
        }
        dup2(fout, STDOUT_FILENO);
        
        // pwd
        if (strcmp(cmd->argv[0], "pwd") == 0) {
            ret_code = pwd();
            exit(ret_code);
        }
        
        execvp(cmd->argv[0], cmd->argv);
    } else {
        // parent process
        close(pre_fd[0]);
        close(pre_fd[1]);
        int tmp;
        waitpid(child_pid, &tmp, 0);
        ret_code = (EXEC_ERROR_T)WEXITSTATUS(tmp);
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
            COMMAND_T* cmd = cmd_list->cmd_list[i];
            if (cmd->parse_error != PARSE_OK) {
                ret_code = EXEC_CMD_PARSE_ERROR;
                break;
            }
            ret_code = ExecCmd(cmd, pipe_list[i], pipe_list[i+1]);
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
