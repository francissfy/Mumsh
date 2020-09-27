//
//  parser.h
//  Mumsh
//
//  Created by Francis on 2020/9/16.
//  Copyright © 2020 Francis. All rights reserved.
//

#ifndef parser_h
#define parser_h

#include <string.h>


#include "syntax.h"

COMMAND_T* ParseSingleCmd(int buff_off_s, int buff_off_t);

COMMAND_LIST_T* Parse(const char* cmd_line);

#endif /* parser_h */
