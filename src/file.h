#ifndef FILE_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "err.h"
#include "str.h"

/******************************************************************************/
/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/******************************************************************************/

#define ERR_FILE_STR_READ "failed to read file"
ErrDecl file_str_read(Str *filename, Str *content);

ErrDecl file_str_write(Str *filename, Str *content);

#define ERR_FILE_FP_READ "failed to read file"
ErrDecl file_fp_read(FILE *file, Str *content);

#define FILE_H
#endif
