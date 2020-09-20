//
//  exec.h
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#ifndef exec_h
#define exec_h

#include "types.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <sys/wait.h>

static const size_t MAX_BUFFER_SIZE = 1024;
static char buffer[1024];

EXEC_ERROR_T ExecCmd(COMMAND_T* cmd, int* pre_fd, int* cur_fd);

EXEC_ERROR_T ExecCmdList(COMMAND_LIST_T* cmd_list);

#endif /* exec_h */
