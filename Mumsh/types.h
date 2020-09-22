//
//  types.h
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#ifndef types_h
#define types_h

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/********************** global enum ************************/


// the input/output type
typedef enum {
    STD_IO,
    // BUFFER_IO,
    FILE_IO,
    FILE_APPD_IO
} IO_TYPE_T;


// errors arose during parsing
typedef enum {
    PARSE_OK,
    PARSE_EMPTY_COMMAND,
    PARSE_UNKNOW_ERROR
} PARSE_ERROR_T;


// errors arose during the exec stage
typedef enum {
    EXEC_OK,
    EXEC_PIPE_ERROR,
    EXEC_FILE_NOT_EXIST,
    EXEC_FILE_PERMISSION_DENY,
    EXEC_BUFFER_OVERFLOW,
    EXEC_FORK_ERROR,
    EXEC_CHDIR_ERROR,
    EXEC_CANNOT_GET_HOME_DIR,
    EXEC_CMD_PARSE_ERROR,
    EXEC_COMMAND_NOT_FOUND,
    EXEC_TOO_MANY_FILE_IO,
    EXEC_UNKNOWN_ERROR
} EXEC_ERROR_T;

typedef enum {
    JOB_FOREGOUND,
    JOB_BACKGOUND
} JOB_TYPE_T;

typedef enum {
    JOB_RUNNING,
    JOB_DONE
} JOB_STATUS_T;


/********************** global data structure ************************/


// the config of io, including destination/source, file name
typedef struct {
    IO_TYPE_T io_type;
    // int file_count;
    // char** file_list;
    char* file;
} IO_CONFIG_T;



typedef struct {
    int argc;
    char** argv;
    IO_CONFIG_T* io_input;
    IO_CONFIG_T* io_output;
    int job_pid;
    PARSE_ERROR_T parse_error;
} COMMAND_T;


// type of a command pipeline
typedef struct {
    int cmd_count;
    int cursor;
    COMMAND_T** cmd_list;
    // support backgound jobs
    char* cmd_line;     // initial command line
    JOB_TYPE_T job_type;     // job type
    JOB_STATUS_T status;     // job running status
} COMMAND_LIST_T;




/********************** mem function ************************/

void InitIOConfig(IO_CONFIG_T* io_config);

void FreeIOConfig(IO_CONFIG_T* io_config);

void InitCmd(COMMAND_T* cmd);

void FreeCmd(COMMAND_T* cmd);

void FreeCmdList(COMMAND_LIST_T* cmd_list);

/********************** utility function ************************/

void PrintExecErrMsg(EXEC_ERROR_T err_code);

void PrintIO(IO_CONFIG_T* io_config);

void PrintCMD(COMMAND_T* cmd);

void PrintCMDList(COMMAND_LIST_T* cmd_list);

/********************** small tools ************************/

typedef struct {
    char stack[10];
    int stack_count;
} CHAR_STACK;

int CharStackEmpty(CHAR_STACK* stack);

char CharStackTop(CHAR_STACK* stack);

char CharStackPop(CHAR_STACK* stack);

void CharStackPush(CHAR_STACK* stack, char elem);

// task manager
typedef struct {
    int pool_size;
    int job_count;
    COMMAND_LIST_T* cmd_queue[20];  // at most 10 cmd list on background
} TASK_POOL_T;


void InitTaskPool(TASK_POOL_T* task_pool);

void AddTaskToPool(TASK_POOL_T* task_pool, COMMAND_LIST_T* job);

void PrintBackgoundTopJobs(TASK_POOL_T* task_pool);

void PrintJobs(TASK_POOL_T* task_pool);

void FreePool(TASK_POOL_T* task_pool);


#endif /* types_h */
