//
//  parser.c
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "parser.h"


COMMAND_T* ParseCmd(const char* cmd_line) {
    COMMAND_T* cmd = (COMMAND_T*)malloc(sizeof(COMMAND_T));
    cmd->parse_error = PARSE_OK;
    
    // check for empty cmd
    if (strlen(cmd_line) == 0) {
        cmd->parse_error = PARSE_EMPTY_COMMAND;
        return cmd;
    }
    
    // make a copy of cmd_line, line will later be modified by strtok
    char* line = (char*)malloc(sizeof(char)*(strlen(cmd_line)+1));
    strcpy(line, cmd_line);
    
    // spliting cmd and do check
    int arg_list_count = 0;
    char* arg_list_tmp[20] = {};
    const char delim[] = " ";
    char* ttk = strtok(line, delim);
    while (ttk) {
        char* t = (char*)malloc(sizeof(char)*(strlen(ttk)+1));
        strcpy(t, ttk);
        arg_list_tmp[arg_list_count] = t;
        arg_list_count += 1;
        ttk = strtok(NULL, delim);
    }
    
    int iter = 0;
    iter++;
    while (iter<arg_list_count && strpbrk(arg_list_tmp[iter], ">><") == NULL) {
        iter += 1;
    }
    cmd->argc = iter;
    cmd->argv = (char**)malloc(sizeof(char*)*(cmd->argc+1));
    memcpy(cmd->argv, arg_list_tmp, sizeof(char*)*cmd->argc);
    cmd->argv[cmd->argc] = NULL;
    // init io config
    cmd->io_input = (IO_CONFIG_T*)malloc(sizeof(IO_CONFIG_T));
    InitIOConfig(cmd->io_input);
    cmd->io_output = (IO_CONFIG_T*)malloc(sizeof(IO_CONFIG_T));
    InitIOConfig(cmd->io_output);
    // check for redirection
    if (iter < arg_list_count) {
        int input_sign_pos = arg_list_count;
        int output_sign_pos = arg_list_count;
        int output_append_sign_pos = arg_list_count;
        int t = iter;
        while(t<arg_list_count) {
            if(strcmp(arg_list_tmp[t], "<") == 0) {
                input_sign_pos = t;
            } else if(strcmp(arg_list_tmp[t], ">") == 0) {
                output_sign_pos = t;
            } else if(strcmp(arg_list_tmp[t], ">>") == 0) {
                output_append_sign_pos = t;
            }
            t++;
        }
        // copy io info
        int input_file_count = output_append_sign_pos+output_sign_pos-arg_list_count-input_sign_pos-1;
        if(input_file_count>0) {
            cmd->io_input->io_type = FILE_IO;
            cmd->io_input->file_count = input_file_count;
            cmd->io_input->file_list = (char**)malloc(sizeof(char*)*cmd->io_input->file_count);
            memcpy(cmd->io_input->file_list, arg_list_tmp+input_sign_pos+1, sizeof(char*)*cmd->io_input->file_count);
        }
        if (output_sign_pos < arg_list_count) {
            cmd->io_output->io_type = FILE_IO;
            cmd->io_output->file_count = arg_list_count-output_sign_pos-1;
            cmd->io_output->file_list = (char**)malloc(sizeof(char*)*cmd->io_output->file_count);
            memcpy(cmd->io_output->file_list, arg_list_tmp+output_sign_pos+1, sizeof(char*)*cmd->io_output->file_count);
        } else if(output_append_sign_pos < arg_list_count) {
            cmd->io_output->io_type = FILE_APPD_IO;
            cmd->io_output->file_count = arg_list_count-output_append_sign_pos-1;
            cmd->io_output->file_list = (char**)malloc(sizeof(char*)*cmd->io_output->file_count);
            memcpy(cmd->io_output->file_list, arg_list_tmp+output_append_sign_pos+1, sizeof(char*)*cmd->io_output->file_count);
        } else {
            // pass
        }
        // clean temporary memory
        if (input_sign_pos<arg_list_count) {
            free(arg_list_tmp[input_sign_pos]);
        }
        if (output_sign_pos<arg_list_count) {
            free(arg_list_tmp[output_sign_pos]);
        }
        if (output_append_sign_pos<arg_list_count) {
            free(arg_list_tmp[output_append_sign_pos]);
        }
    }
    
    free(line);
    return cmd;
}



COMMAND_LIST_T* ParseInput(const char* cmd_input) {
    char* line = (char*)malloc(sizeof(char)*(strlen(cmd_input)+1));
    strcpy(line, cmd_input);
    // make a copy
    COMMAND_LIST_T* cmd_list = (COMMAND_LIST_T*)malloc(sizeof(COMMAND_LIST_T));
    
    int pipe_op_count = 0;
    for (int i=0; i<strlen(line); i++) {
        pipe_op_count += (line[i] == '|');
    }
    
    cmd_list->cmd_count = pipe_op_count+1;
    cmd_list->cmd_list = (COMMAND_T**)malloc(sizeof(COMMAND_T*)*cmd_list->cmd_count);
    
    int cmd_iter = 0;
    char* iter = line;
    while (1) {
        char* tmp = strchr(iter, '|');
        if (tmp == NULL) {
            cmd_list->cmd_list[cmd_iter] = ParseCmd(iter);
            break;
        }
        *tmp = '\0';
        cmd_list->cmd_list[cmd_iter] = ParseCmd(iter);
        cmd_iter += 1;
        iter = tmp+1;
    }
    free(line);
    return cmd_list;
}
