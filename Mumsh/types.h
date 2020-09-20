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
    PARSE_ERROR_T parse_error;
} COMMAND_T;


// type of a command pipeline
typedef struct {
    int cmd_count;
    COMMAND_T** cmd_list;
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

#endif /* types_h */
