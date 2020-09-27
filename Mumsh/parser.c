//
//  parser.c
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "parser.h"

char parser_buffer[64*64];
int parser_cmpt_count = 0;



COMMAND_T* ParseSingleCmd(int buff_off_s, int buff_off_t) {
    COMMAND_T* cmd = (COMMAND_T*)malloc(sizeof(COMMAND_T));
    InitCmd(cmd);

    // init io
    cmd->io_input = (IO_CONFIG_T*)malloc(sizeof(IO_CONFIG_T));
    InitIOConfig(cmd->io_input);
    cmd->io_output = (IO_CONFIG_T*)malloc(sizeof(IO_CONFIG_T));
    InitIOConfig(cmd->io_output);
    
    
    int offset =  buff_off_s;
    
    char* argv[10] = {NULL};
    int argc = 0;
    
    while (offset<=buff_off_t) {
        char* line = parser_buffer+64*offset;
        if (strcmp(line, ">>")==0) {
            offset++;
            char* dst = line+64;
            rm_quotes(dst);
            cmd->io_output->io_type = FILE_APPD_IO;
            cmd->io_output->file = (char*)malloc(sizeof(char)*(strlen(dst)+1));
            memcpy(cmd->io_output->file, dst, strlen(dst)+1);
        } else if (strcmp(line, ">") ==0) {
            offset++;
            char* dst = line+64;
            rm_quotes(dst);
            cmd->io_output->io_type = FILE_IO;
            cmd->io_output->file = (char*)malloc(sizeof(char)*(strlen(dst)+1));
            memcpy(cmd->io_output->file, dst, strlen(dst)+1);
        } else if (strcmp(line, "<") ==0) {
            offset++;
            char* dst = line+64;
            rm_quotes(dst);
            cmd->io_input->io_type = FILE_IO;
            cmd->io_input->file = (char*)malloc(sizeof(char)*(strlen(dst)+1));
            memcpy(cmd->io_input->file, dst, strlen(dst)+1);
        } else {
            rm_quotes(line);
            char* t = (char*)malloc(sizeof(char)*(strlen(line)+1));
            memcpy(t, line, strlen(line)+1);
            argv[argc++] = t;
        }
        offset++;
    }
    cmd->argc = argc;
    cmd->argv = (char**)malloc(sizeof(char*)*(argc+1));
    memcpy(cmd->argv, argv, sizeof(char*)*argc);
    cmd->argv[argc] = NULL;
    return cmd;
}


COMMAND_LIST_T* Parse(const char* cmd_line) {
    char* line = (char*)malloc(sizeof(char)*(strlen(cmd_line)+1));
    memcpy(line, cmd_line, strlen(cmd_line)+1);
    
    COMMAND_LIST_T* cmd_list = (COMMAND_LIST_T*)malloc(sizeof(COMMAND_LIST_T));
    cmd_list->cmd_line = (char*)malloc(sizeof(char)*(strlen(cmd_line)+1));
    memcpy(cmd_list->cmd_line, cmd_line, strlen(cmd_line)+1);
    
    int i=(int)strlen(cmd_line)-1;
    while (i>0 && line[i] == ' ') {
        i--;
    }
    
    if (line[i] == '&') {
        line[i] = '\0';
        cmd_list->job_type = JOB_BACKGOUND;
    } else {
        cmd_list->job_type = JOB_FOREGOUND;
    }
    
    parser_cmpt_count = 0;
    memset(parser_buffer, '\0', 64*64);
    FormatToBuffer(line, parser_buffer, &parser_cmpt_count);
    
    // parse from buffer
    int pipe_op_count = 0;
    for (int i=0; i<parser_cmpt_count; i++) {
        pipe_op_count += (strcmp(parser_buffer+64*i, "|") == 0);
    }

    cmd_list->cmd_count = pipe_op_count+1;
    cmd_list->cmd_list = (COMMAND_T**)malloc(sizeof(COMMAND_T*)*cmd_list->cmd_count);

    
    int cmd_iter = 0;
    int offset_s = 0;
    for (int i=0; i<parser_cmpt_count; i++) {
        char* line = parser_buffer+64*i;
        if (strcmp(line, "|") == 0) {
            cmd_list->cmd_list[cmd_iter++] = ParseSingleCmd(offset_s, i-1);
            offset_s = i+1;
        }
    }
    cmd_list->cmd_list[cmd_iter++] = ParseSingleCmd(offset_s, parser_cmpt_count-1);
    
    free(line);
    cmd_list->cursor = 0;
    cmd_list->status = JOB_RUNNING;
    return cmd_list;
}
