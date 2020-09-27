//
//  syntax.c
//  Mumsh
//
//  Created by Francis on 2020/9/26.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "syntax.h"

// can be used any where in syntax
// 看我的骚操作
char syntax_buffer[10*64];
int syntax_count = 0;


// 分词小助手
const char* TKHelper(const char* s, int* len) {
    const char* iter = s;
    while (iter[0] == ' ' && iter[0] != '\0') {
        iter++;
    }
    int t=0;
    if (iter[0] == '>' || iter[0] == '<') {
        while (iter[t] == iter[0] && iter[t] != '\0' && t<2) {
            t++;
        }
    } else if (iter[0] == '|') {
        t = 1;
    } else {
        while (iter[t] != '>' && iter[t] != '<' && iter[t] != ' ' && iter[t] != '|' && iter[t] != '\0') {
            t++;
        }
    }
    *len = t;
    return iter;
}

// format to buffer, with per 10 char offset
void FormatToBuffer(const char* line) {
    // clean buffer
    memset(syntax_buffer, '\0', 10*64);
    syntax_count = 0;
    const char* iter = line;
    int len;
    while (*iter != '\0') {
        iter = TKHelper(iter, &len);
        char* dest = syntax_buffer+syntax_count*10;
        memcpy(dest, iter, sizeof(char)*len);
        dest[len] = '\0';
        syntax_count++;
        iter += len;
    }
    // print
    // for (int i=0; i<syntax_count; i++) {
    //     printf("%s\n", syntax_buffer+10*i);
    // }
}

int isValidFileName(const char* name) {
    if (strlen(name) == 0) {
        return 0;
    }
    if (name[0] == '>' || name[0] == '<' || name[0] == '|') {
        return 0;
    }
    return 1;
}

// single cmd content
// from buffer+10*buff_off_s to buffer+10*buff_off_t(both end included)
int SyntaxCheckerHelper(int buff_off_s, int buff_off_t, int prompt) {
    int offset =  buff_off_s;
    
    char* io_in_file = NULL;
    char* io_out_file = NULL;
    char* io_app_file = NULL;
    char* argv[10] = {NULL};
    int argc = 0;
    
    if (buff_off_t < buff_off_s) {
        if (prompt) {
            fprintf(stderr, "error: missing program\n");
        }
        return -1;
    }
    
    while (offset<=buff_off_t) {
        char* line = syntax_buffer+10*offset;
        
        if (strcmp(line, ">>")==0) {
            if (offset == syntax_count-1) {
                // ">>" is at the end
                if (prompt) {
                    fprintf(stderr, "syntax error near unexpected token EOL\n");
                }
                return -1;
            } else if (offset == buff_off_t) {
                // no name after >>
                if (prompt) {
                    fprintf(stderr, "syntax error near unexpected token `%c'\n", (line+10)[0]);
                }
                return -1;
            } else {
                // extract name
                if (isValidFileName(line+10)) {
                    if (io_out_file != NULL || io_app_file != NULL) {
                        if (prompt) {
                            fprintf(stderr, "duplicated output redirection\n");
                        }
                        return -1;
                    } else {
                        io_app_file = line+10;
                        offset++;
                    }
                } else {
                    if (prompt) {
                        fprintf(stderr, "syntax error near unexpected token `%c'\n", (line+10)[0]);
                    }
                    return -1;
                }
            }
        } else if (strcmp(line, ">") ==0) {
            if (offset == syntax_count-1) {
                // ">" is at the end
                if (prompt) {
                    fprintf(stderr, "syntax error near unexpected token EOL\n");
                }
                return -1;
            } else if (offset == buff_off_t) {
                // no name after >
                if (prompt) {
                    fprintf(stderr, "syntax error near unexpected token `%c'\n", (line+10)[0]);
                }
                return -1;
            } else {
                // extract name
                if (isValidFileName(line+10)) {
                    if (io_out_file != NULL || io_app_file != NULL) {
                        if (prompt) {
                            fprintf(stderr, "duplicated output redirection\n");
                        }
                        return -1;
                    } else {
                        io_out_file = line+10;
                        offset++;
                    }
                } else {
                    if (prompt) {
                        fprintf(stderr, "syntax error near unexpected token `%c'\n", (line+10)[0]);
                    }
                    return -1;
                }
            }
        } else if (strcmp(line, "<") ==0) {
            // common argv
            if (offset == syntax_count-1) {
                // ">" is at the end
                if (prompt) {
                    fprintf(stderr, "syntax error near unexpected token EOL\n");
                }
                return -1;
            } else if (offset == buff_off_t) {
                // no name after >
                if (prompt) {
                    fprintf(stderr, "syntax error near unexpected token `%c'\n", (line+10)[0]);
                }
                return -1;
            } else {
                // extract name
                if (isValidFileName(line+10)) {
                    if (io_in_file != NULL) {
                        if (prompt) {
                            fprintf(stderr, "duplicated output redirection\n");
                        }
                        return -1;
                    } else {
                        io_in_file = line+10;
                        offset++;
                    }
                } else {
                    if (prompt) {
                        fprintf(stderr, "syntax error near unexpected token `%c'\n", (line+10)[0]);
                    }
                    return -1;
                }
            }
        } else {
            argv[argc++] = line;
        }
        offset++;
    }
    if (argc == 0) {
        if (prompt) {
            fprintf(stderr, "error: missing program\n");
        }
        return -1;
    }
    if (buff_off_t < syntax_count-1 && (io_out_file != NULL || io_app_file != NULL)) {
        // check whether have out redirection
        if (prompt) {
            fprintf(stderr, "error: duplicated output redirection\n");
        }
        return -1;
    }
    if (buff_off_s>0 && io_in_file != NULL) {
        // check whether have input redirection
        if (prompt) {
            fprintf(stderr, "error: duplicated input redirection\n");
        }
        return -1;
    }
    return 0;
}


int SyntaxChecker(const char* cmd_line, int prompt) {
    FormatToBuffer(cmd_line);
    if (syntax_count == 0) {
        fprintf(stderr, "error: missing program\n");
        return -1;
    }
    int offset_s = 0;
    for (int i=0; i<syntax_count; i++) {
        char* line = syntax_buffer+10*i;
        if (strcmp(line, "|") == 0) {
            if (i==0) {
                fprintf(stderr, "error: missing program\n");
                return -1;
            } else if (SyntaxCheckerHelper(offset_s, i-1, prompt) == -1) {
                return -1;
            } else {
                offset_s = i+1;
            }
        }
    }
    if (SyntaxCheckerHelper(offset_s, syntax_count-1, prompt) == -1) {
        return -1;
    }
    return 0;
}


int SyntaxCheck_L(const char* cmd_line, int len, int prompt) {
    char* copy = (char*)malloc(sizeof(char)*(len+1));
    memcpy(copy, cmd_line, len+1);
    copy[len] = '\0';
    int ret = SyntaxChecker(copy, prompt);
    free(copy);
    return ret;
}
