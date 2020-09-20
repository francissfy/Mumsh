//
//  parser.c
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "parser.h"


// move iter to the start of word, and return the length of the word
// the chars skipped are: '>', '<', ' ';
const char* word_extractor(const char* iter, int* len) {
    int t = 0;
    const char* i = iter;
    while (*i == '>' || *i == '<' || *i == ' ') {
        i += 1;
    }
    while (i[t] != '>' && i[t] != '<' && i[t] != ' ' && i[t] != '\0') {
        t +=1 ;
    }
    *len = t;
    return i;
}


COMMAND_T* ParseCmd(const char* line) {
    COMMAND_T* cmd = (COMMAND_T*)malloc(sizeof(COMMAND_T));
    InitCmd(cmd);
    
    // init io
    cmd->io_input = (IO_CONFIG_T*)malloc(sizeof(IO_CONFIG_T));
    InitIOConfig(cmd->io_input);
    cmd->io_output = (IO_CONFIG_T*)malloc(sizeof(IO_CONFIG_T));
    InitIOConfig(cmd->io_output);
    
    // scan through
    const char* iter = line;
    int tmp=0;
    char* argv[10] = {NULL};
    int argc = 0;
    
    while (iter[0] != '\0') {
        if (iter[0] == '>') {
            if (iter[1] == '>') {
                // >>
                cmd->io_output->io_type = FILE_APPD_IO;
            } else {
                // >
                cmd->io_output->io_type = FILE_IO;
            }
            iter = word_extractor(iter, &tmp);
            char* file_name = (char*)malloc(sizeof(char)*(tmp+1));
            memcpy(file_name, iter, sizeof(char)*tmp);
            file_name[tmp] = '\0';
            cmd->io_output->file = file_name;
        } else if (iter[0] == '<') {
            // <
            iter = word_extractor(iter, &tmp);
            char* file_name = (char*)malloc(sizeof(char)*(tmp+1));
            memcpy(file_name, iter, sizeof(char)*tmp);
            file_name[tmp] = '\0';
            cmd->io_input->file = file_name;
            cmd->io_input->io_type = FILE_IO;
        } else if (iter[0] == ' ') {
            tmp = 1;
            // pass
        } else {
            // common argv
            iter = word_extractor(iter, &tmp);
            char* arg = (char*)malloc(sizeof(char)*(tmp+1));
            memcpy(arg, iter, sizeof(char)*tmp);
            arg[tmp] = '\0';
            argv[argc] = arg;
            argc += 1;
        }
        iter += tmp;
    }
    cmd->argc = argc;
    cmd->argv = (char**)malloc(sizeof(char*)*(argc+1));
    memcpy(cmd->argv, argv, sizeof(char*)*argc);
    cmd->argv[argc] = NULL;
    return cmd;
}


COMMAND_LIST_T* ParseInput(const char* cmd_input) {
    char* line = (char*)malloc(sizeof(char)*(strlen(cmd_input)+1));
    strcpy(line, cmd_input);
    // make a copy
    COMMAND_LIST_T* cmd_list = (COMMAND_LIST_T*)malloc(sizeof(COMMAND_LIST_T));
    
    int pipe_op_count = 0;
    for (int i=0; i<(int)strlen(line); i++) {
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
