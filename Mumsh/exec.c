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
    // printf("abs_path:\n%s\n", abs_path);
    if (chdir(abs_path) != 0) {
        ret_code = EXEC_CHDIR_ERROR;
    }
    free(abs_path);
    return ret_code;
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        fprintf(stderr, "SIGINT: signal interrupt!\n");
        exit(-1);
    }
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
            fprintf(stderr, "ERROR: %s: file not exists!\n", cmd->io_input->file_list[0]);
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
            fprintf(stderr, "ERROR: %s: permission denied!\n", cmd->io_output->file_list[0]);
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
        // handle control c signal
        // somehow meaningless
        signal(SIGINT, signal_handler);
        
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
                fprintf(stderr, "ERROR: command parsing error!\n");
                ret_code = EXEC_CMD_PARSE_ERROR;
                break;
            }
            ret_code = ExecCmd(cmd, pipe_list[i], pipe_list[i+1]);
            if (ret_code != EXEC_OK) {
                // error msg have been printed in ExecCmd
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
