#include <errno.h>
#include "file.h"
#include "platform.h"

/******************************************************************************/
/* PUBLIC FUNCTION IMPLEMENTATIONS ********************************************/
/******************************************************************************/

int file_fp_read(FILE *file, Str *content)
{
    if(!file) THROW("invalid filename");
    if(!content) THROW("invalid output buffer");

    /* get file length */
    fseek(file, 0, SEEK_END);
    size_t bytes_file = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    /* allocate memory */
    TRY(str_cap_ensure(content, bytes_file + 1), "couldn't increase capacity");

    /* read file */
    size_t bytes_read = fread(content->s, 1, bytes_file, file);
    //if(bytes_file != bytes_read) THROW("mismatch read vs expected bytes");
    content->s[bytes_read] = 0;
    content->l = bytes_read;
    
    /* close file outside */
    return 0;
error:
    return -1;
}

int file_str_read(Str *filename, Str *content)
{
    int err = 0;
    FILE *file = 0;
    if(!filename) THROW("invalid filename");
    if(!content) THROW("invalid output buffer");

    /* open the file */
    errno = 0;
    if(filename->l && (
                filename->s[filename->l - 1] == PLATFORM_CH_SUBDIR ||
                filename->s[filename->l - 1] == '/')) {
        THROW("won't open directories");
    }
    file = fopen(filename->s, "r");
    if(!file || errno) THROW("failed to open file named '%s'", filename->s);

    TRY(file_fp_read(file, content), ERR_FILE_FP_READ);
    /* close file */
clean:
    if(file) fclose(file);
    return err;
error: ERR_CLEAN;
}

#if 0
int file_str_write(Str *filename, Str *content)
{
    ASSERT_ERROR("implementation missing");
    return 0;
}
#endif

