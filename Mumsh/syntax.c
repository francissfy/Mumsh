//
//  syntax.c
//  Mumsh
//
//  Created by Francis on 2020/9/26.
//  Copyright © 2020 Francis. All rights reserved.
//

#include "syntax.h"


char syntax_buffer[64*64];
int syntax_count = 0;


void rm_quotes(char* line) {
    int cp_offset = 0;
    int scan_offset = 0;
    char quote = '\0';
    while (scan_offset < (int)strlen(line)) {
        char t = line[scan_offset];
        if (t == '\'' || t == '\"') {
            if (quote == '\0') {
                // record and scan++
                quote = t;
            } else {
                if (quote == t) {
                    // encounter the same, not cp, reset quote_ptr
                    quote = '\0';
                } else {
                    // not the same, cp
                    if (scan_offset > cp_offset) {
                        line[cp_offset++] = t;
                    }
                }
            }
        } else {
            // cp
            if (scan_offset > cp_offset) {
                line[cp_offset] = t;
            }
            cp_offset++;
        }
        scan_offset += 1;
    }
    line[cp_offset] = '\0';
}


/*
 Tokenizer helper
 skip space and split to >>, >, <, |, word
 split by space, >(till space or <, space), >>(at most 2),<, |
 */

// consider:
// "echo" "<1.'txt'" >"2.""txt"
const char* TKHelper(const char* s, int* len) {
    const char* iter = s;
    while (iter[0] == ' ' && iter[0] != '\0') {
        iter++;
    }
    int t=0;
    
    if (iter[0] == '>') {
        // split >>|>
        while (iter[t] == iter[0] && t<2) {
            t++;
        }
    } else if (iter[0] == '<') {
        t=1;
    } else if (iter[0] == '|') {
        t=1;
    } else if (iter[0] == '\'' || iter[0] == '\"') {
        // compress connected strings lile "2.""txt"
        while (iter[t] == '\'' || iter[t] == '\"') {
            char tmp = iter[t];
            t++;
            while (iter[t] != tmp) {
                t++;
            }
            t++;
        }
    } else {
        while (iter[t] != ' ' && iter[t] != '<'
               && iter[t] != '>' && iter[t] != '|' && iter[t] != '\0') {
            t++;
        }
    }
    *len = t;
    return iter;
}


/*
 split the line in to sematic components
 */
void FormatToBuffer(const char* line, char* buffer, int* buffer_count) {
    int print_cpmts = 0;
    
    const char* iter = line;
    int len;
    while (*iter != '\0') {
        iter = TKHelper(iter, &len);
        if (len>0) {
            char* dest = buffer+(*buffer_count)*64;
            memcpy(dest, iter, sizeof(char)*len);
            dest[len] = '\0';
            (*buffer_count)++;
            iter += len;
        }
    }
    // print
    if (print_cpmts) {
        for (int i=0; i<*buffer_count; i++) {
            printf("%s\n", buffer+64*i);
        }
    }
}


/*
 cursor: point to the word of >>, > or <
 dst: point to char*, the name of the file,
    if no valid name is found, set to NULL and return -1
 promt: whether print the error message
 */
int RedirectionExtractor(int* cursor, char** dst1, char** dst2, int prompt, const char* type) {
    if (*cursor == syntax_count-1) {
        if (prompt) {
            fprintf(stderr, "unexpected EOL\n");
        }
        return -1;
    }
    *cursor = *cursor+1;
    char* dest = syntax_buffer + (*cursor)*64;
    rm_quotes(dest);
    
    // if ( isalpha(dest[0]) || isdigit(dest[0]) ) {
    if ( dest[0] != '>' && dest[0] != '<'  && dest[0] != '|' ) {
        if (*dst1 == NULL && *dst2 == NULL) {
            *dst1 = dest;
        } else {
            if (prompt) {
                fprintf(stderr, "error: duplicated %s redirection\n", type);
            }
            return -1;
        }
    } else {
        if (prompt) {
            fprintf(stderr, "syntax error near unexpected token `%c'\n", dest[0]);
        }
        return -1;
    }
    return 0;
}


/*
 check for single command valibility
 sematic component from buff_off_s to buff_off_t (both end included)
 */
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
        char* line = syntax_buffer+64*offset;
        if (strcmp(line, ">>")==0) {
            if (RedirectionExtractor(&offset, &io_app_file, &io_out_file, prompt, "output") != 0) {
                return -1;
            }
        } else if (strcmp(line, ">") ==0) {
            if (RedirectionExtractor(&offset, &io_out_file, &io_app_file, prompt, "output") != 0) {
                return -1;
            }
        } else if (strcmp(line, "<") ==0) {
            if (RedirectionExtractor(&offset, &io_in_file, &io_in_file, prompt, "input") != 0) {
                return -1;
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


/*
 check the valibility of the command line
 */
int SyntaxChecker(const char* cmd_line, int prompt) {
    syntax_count = 0;
    memset(syntax_buffer, '\0', 64*64);
    // if background job, remove & sign
    // make no difference, treat & as arguments
    
    FormatToBuffer(cmd_line, syntax_buffer, &syntax_count);
    if (syntax_count == 0) {
        fprintf(stderr, "error: missing program\n");
        return -1;
    }
    int offset_s = 0;
    for (int i=0; i<syntax_count; i++) {
        char* line = syntax_buffer+64*i;
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

/*
 plus an len parameter
 */
int SyntaxCheck_L(const char* cmd_line, int len, int prompt) {
    if (len == 0) {
        return 0;
    }
    char* copy = (char*)malloc(sizeof(char)*(len+1));
    memcpy(copy, cmd_line, len+1);
    copy[len] = '\0';
    int ret = SyntaxChecker(copy, prompt);
    free(copy);
    return ret;
}
