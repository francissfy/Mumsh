//
//  types.c
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "types.h"

void InitIOConfig(IO_CONFIG_T* io_config) {
    io_config->io_type = STD_IO;
    io_config->file = NULL;
}

void FreeIOConfig(IO_CONFIG_T* io_config) {
    if (io_config == NULL) {
        return;
    }
    if (io_config->io_type != STD_IO) {
        free(io_config->file);
    }
    free(io_config);
}


void InitCmd(COMMAND_T* cmd) {
    cmd->argc = 0;
    cmd->argv = NULL;
    cmd->io_input = NULL;
    cmd->io_output = NULL;
    cmd->job_pid = 0;
    cmd->parse_error = PARSE_OK;
}


void FreeCmd(COMMAND_T* cmd) {
    if (cmd == NULL) {
        return;
    }
    for (int i=0; i<cmd->argc; i++) {
        free(cmd->argv[i]);
    }
    free(cmd->argv);
    FreeIOConfig(cmd->io_input);
    FreeIOConfig(cmd->io_output);
    free(cmd);
}

void FreeCmdList(COMMAND_LIST_T* cmd_list) {
    for (int i=0; i<cmd_list->cmd_count; i++) {
        FreeCmd(cmd_list->cmd_list[i]);
    }
    free(cmd_list->cmd_list);
    free(cmd_list->cmd_line);
    free(cmd_list);
}

void PrintExecErrMsg(EXEC_ERROR_T err_code) {
    switch (err_code) {
        case EXEC_PIPE_ERROR:
            printf("EXEC_PIPE_ERROR");
            break;
        case EXEC_FILE_NOT_EXIST:
            printf("EXEC_FILE_NOT_EXIST");
            break;
        case EXEC_FILE_PERMISSION_DENY:
            printf("EXEC_FILE_PERMISSION_DENY");
            break;
        case EXEC_BUFFER_OVERFLOW:
            printf("EXEC_BUFFER_OVERFLOW");
            break;
        case EXEC_FORK_ERROR:
            printf("EXEC_FORK_ERROR");
            break;
        case EXEC_CHDIR_ERROR:
            printf("EXEC_CHDIR_ERROR");
            break;
        case EXEC_CMD_PARSE_ERROR:
            printf("EXEC_CMD_PARSE_ERROR");
            break;
        case EXEC_COMMAND_NOT_FOUND:
            printf("EXEC_COMMAND_NOT_FOUND");
            break;
        case EXEC_CANNOT_GET_HOME_DIR:
            printf("EXEC_CANNOT_GET_HOME_DIR");
            break;
        case EXEC_TOO_MANY_FILE_IO:
            printf("EXEC_TOO_MANY_FILE_IO");
            break;
        case EXEC_UNKNOWN_ERROR:
            printf("EXEC_UNKNOWN_ERROR");
            break;
        case EXEC_OK:
            printf("EXEC_OK");
            break;
        default:
            break;
    }
    printf("\n");
}

void PrintIO(IO_CONFIG_T* io_config) {
    printf("io type: ");
    if (io_config->io_type == STD_IO) {
        printf("std_io\n");
    } else {
        if (io_config->io_type == FILE_IO) {
            printf("file_io ");
        } else if(io_config->io_type == FILE_APPD_IO) {
            printf("file_append_io ");
        }
        printf("file: %s\n", io_config->file);
    }
}


void PrintCMD(COMMAND_T* cmd) {
    if(cmd->parse_error != PARSE_OK) {
        printf("parsing error\n");
        return;
    }
    printf("args: ");
    for (int i=0; i<=cmd->argc; i++) {
        printf("%s, ", cmd->argv[i]);
    }
    printf("\ninput_io: ");
    PrintIO(cmd->io_input);
    printf("output_io: ");
    PrintIO(cmd->io_output);
    printf("job pid: %d\n", cmd->job_pid);
}

void PrintJobType(JOB_TYPE_T job_type) {
    switch (job_type) {
        case JOB_FOREGOUND:
            printf("JOB_FOREGOUND\n");
            break;
        case JOB_BACKGOUND:
            printf("JOB_BACKGOUND\n");
            break;
        default:
            break;
    }
}

void PrintJobStatus(JOB_STATUS_T status) {
    switch (status) {
        case JOB_RUNNING:
            printf("running");
            break;
        case JOB_DONE:
            printf("done");
            break;
        default:
            break;
    }
}

void PrintCMDList(COMMAND_LIST_T* cmd_list) {
    printf("#########################\n");
    printf("job type: ");
    PrintJobType(cmd_list->job_type);
    for(int i=0; i<cmd_list->cmd_count; i++) {
        printf("########### %d ###########\n", i);
        PrintCMD(cmd_list->cmd_list[i]);
    }
    printf("########### END ###########\n");
}

int CharStackEmpty(CHAR_STACK* stack) {
    return stack->stack_count == 0;
}

char CharStackTop(CHAR_STACK* stack) {
    return stack->stack[stack->stack_count-1];
}

char CharStackPop(CHAR_STACK* stack) {
    stack->stack_count -= 1;
    return stack->stack[stack->stack_count];
}

void CharStackPush(CHAR_STACK* stack, char elem) {
    stack->stack[stack->stack_count] = elem;
    stack->stack_count += 1;
}


void InitTaskPool(TASK_POOL_T* task_pool) {
    task_pool->pool_size = 20;
    task_pool->job_count = 0;
    for (int i=0; i<task_pool->pool_size; i++) {
        task_pool->cmd_queue[i] = NULL;
    }
}

void AddTaskToPool(TASK_POOL_T* task_pool, COMMAND_LIST_T* job) {
    job->status = JOB_RUNNING;
    task_pool->cmd_queue[task_pool->job_count] = job;
    task_pool->job_count++;
}


void RefreshJobPool(TASK_POOL_T* task_pool) {
    for (int i=0; i<task_pool->job_count; i++) {
        COMMAND_LIST_T* job = task_pool->cmd_queue[i];
        int is_runnning = 0;
        for (int j=0; j<job->cmd_count; j++) {
            COMMAND_T* cmd = job->cmd_list[j];
//            int code = kill(cmd->job_pid, 0);
            int code = waitpid(cmd->job_pid, NULL, WNOHANG);
            // code == pid if finished, other wise return 0
            if (code == 0) {
                // still running
                is_runnning = 1;
                break;
            }
        }
        if (is_runnning) {
            job->status = JOB_RUNNING;
        } else {
            job->status = JOB_DONE;
        }
    }
}


// [2] (32758) (32759) /bin/ls | cat &
void PrintBackgoundTopJobs(TASK_POOL_T* task_pool) {
    COMMAND_LIST_T* job = task_pool->cmd_queue[task_pool->job_count-1];
    printf("[%d] ", task_pool->job_count);
    for (int j=0; j<job->cmd_count; j++) {
        printf("(%d) ", job->cmd_list[j]->job_pid);
    }
    printf("%s\n", job->cmd_line);
}


// [1] done /bin/ls &
void PrintJobs(TASK_POOL_T* task_pool) {
    RefreshJobPool(task_pool);
    for (int i=0; i<task_pool->job_count; i++) {
        COMMAND_LIST_T* job = task_pool->cmd_queue[i];
        printf("[%d] ", i+1);
        PrintJobStatus(job->status);
        printf(" %s\n", job->cmd_line);
    }
}


void FreePool(TASK_POOL_T* task_pool) {
    for (int i=0; i<task_pool->job_count; i++) {
        FreeCmdList(task_pool->cmd_queue[i]);
    }
}
