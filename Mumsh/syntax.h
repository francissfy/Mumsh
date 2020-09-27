//
//  syntax.h
//  Mumsh
//
//  Created by Francis on 2020/9/26.
//  Copyright © 2020 Francis. All rights reserved.
//

#ifndef syntax_h
#define syntax_h

#include <stdio.h>
#include "types.h"
#include "string.h"

const char* TKHelper(const char* s, int* len);

void FormatToBuffer(const char* line);

int SyntaxCheckerHelper(int buff_off_s, int buff_off_t, int prompt);

int SyntaxChecker(const char* cmd_line, int prompt);

int SyntaxCheck_L(const char* cmd_line, int len, int prompt);

#endif /* syntax_h */
