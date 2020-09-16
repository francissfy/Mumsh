//
//  parser.h
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#ifndef parser_h
#define parser_h

#include "types.h"
#include <string.h>

COMMAND_T* ParseCmd(const char* cmd_line);

COMMAND_LIST_T* ParseInput(const char* cmd_input);

#endif /* parser_h */
