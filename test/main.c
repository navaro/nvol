
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdbool.h>
#include <getopt.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>


#include "shell/corshell.h"
#include "registry/registry.h"

#define SHELL_VERSION_STR       "Navaro corshell Demo v '" __DATE__ "'"
#define SHELL_PROMPT            "# >"

static int32_t      corshell_out(void* ctx, uint32_t out, const char* str) ;
static int32_t      get_line (char * buffer, uint32_t len) ;

/*
 * Declare commands for use with the shell. Use the CORSHELL_CMD_DECL for the
 * commands to be accessible from the command shell interface.
 */
CORSHELL_CMD_DECL("ls", corshell_ls, "");
CORSHELL_CMD_DECL("cd", corshell_cd, "<path>");
CORSHELL_CMD_DECL("source", corshell_source, "<file>");
CORSHELL_CMD_DECL(".", corshell_source2, "<file>");
CORSHELL_CMD_DECL("cat", corshell_cat, "<file>");
CORSHELL_CMD_DECL("pwd", corshell_pwd, "");
CORSHELL_CMD_DECL("echo", corshell_echo, "[string]");
CORSHELL_CMD_DECL("exit", corshell_exit, "[string]");

/*
 * Run until the exit flag is set.
 */
static bool     _shell_exit = false ;

int
main(int argc, char* argv[])
{
    /*
     * Initialise the shell.
     */
    corshell_init () ;
    /*
     * Initialise the registry.
     */
    registry_init () ;
    registry_start () ;

    /*
     * Just add one registry entry for testing purposes.
     */
    registry_value_set ("test", "123", strlen("123") + 1) ;

    /*
     * Print startup and help text.
     */
    printf ("%s\r\n\r\n", SHELL_VERSION_STR) ;
    printf ("use 'help' or '?' for help.\r\n") ;
    printf ("%s", SHELL_PROMPT) ;

    /*
     * Now process the input from the command line as shell commands until
     * the "exit" command was executed.
     */
    do {
        char line[512];
        int len = get_line (line, 512) ;

        if (len > 0) {
            corshell_script_run (0, corshell_out, "", line, len) ;
            printf (SHELL_PROMPT) ;

        }



    } while (!_shell_exit) ;

    return 0;
}


static int32_t
corshell_out (void* ctx, uint32_t out, const char* str)
{
    if (str && (out >= CORSHELL_OUT_STD)) {
        printf ("%s", str) ;

    }

    return  CORSHELL_CMD_E_OK ;
}

static int32_t
get_line (char * buffer, uint32_t len)
{
    uint32_t i = 0 ;

    for (i=0; i<len; i++) {
        buffer[i] = getc (stdin) ;
        if (buffer[i] == '\n') break ;
        if (buffer[i] < 0) break ;
    }

    return i ;
}

static int32_t
corshell_ls (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    /* Here we will list the directory. */
    const char *dir = "." ;
    struct dirent *d;
    DIR *dh = opendir(dir);
    if (!dh) {
        if (errno == ENOENT) {
            corshell_print(ctx, CORSHELL_OUT_ERR, shell_out,
                    "Directory doesn't exist");
        }
        else {
            corshell_print(ctx, CORSHELL_OUT_ERR, shell_out,
                    "Unable to read directory");
        }
        return CORSHELL_CMD_E_FAIL ;
    }
    /* While the next entry is not readable we will print directory files */
    while ((d = readdir(dh)) != NULL) {
        corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
                "%s\r\n", d->d_name);
    }

    closedir(dh);

    return CORSHELL_CMD_E_OK ;
}

static int32_t
corshell_cd (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    /* Change the current directory. */
    if (argc < 2) {
        return CORSHELL_CMD_E_PARMS ;
    }

    if (chdir(argv[1]) != 0) {
        corshell_print(ctx, CORSHELL_OUT_ERR, shell_out,
                "failed");
        return CORSHELL_CMD_E_FAIL ;

    }


    return CORSHELL_CMD_E_OK ;
}

static int32_t
corshell_pwd (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    /* Print the current directory. */
    char buffer[256] ;
    char * pbuffer = getcwd(buffer, 256);
    if (!pbuffer) {
        corshell_print(ctx, CORSHELL_OUT_ERR, shell_out,
                "unable to get current directory.\r\n");
        return CORSHELL_CMD_E_FAIL ;
    }

    corshell_print(ctx, CORSHELL_OUT_ERR, shell_out,
            "%s\r\n", pbuffer);

    return CORSHELL_CMD_E_OK ;
}


static int32_t
read_file (void* ctx, CORSHELL_OUT_FP shell_out, const char * filename,
            char ** pbuffer)
{
    *pbuffer = NULL ;

    /*
     * Read the script specified on the command line.
     */
    FILE * fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        corshell_print(ctx, CORSHELL_OUT_ERR, shell_out,
                "unable to open file \"%s\" for read.\r\n", filename);
         return CORSHELL_CMD_E_NOT_FOUND ;

    }
    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    if (!sz) {
        fclose(fp);
        return CORSHELL_CMD_E_EOF ;
    }

    char * buffer = malloc (sz + 1 ) ;
    if (!buffer) {
        corshell_print(ctx, CORSHELL_OUT_ERR, shell_out,
                "out of memory.\r\n");
        fclose(fp);
         return CORSHELL_CMD_E_MEMORY ;

    }
    long num = fread( buffer, 1, sz, fp );
    if (!num) {
        corshell_print(ctx, CORSHELL_OUT_ERR, shell_out,
                "unable to read file \"%s\".\r\n", filename);
        fclose(fp);
        free (buffer) ;
         return CORSHELL_CMD_E_FAIL ;

    }
    fclose(fp);

    buffer[num] = '\0' ;
    *pbuffer = buffer ;

    return sz ;
}

static int32_t
corshell_source (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    int32_t res  ;

    if (argc < 2) {
        return CORSHELL_CMD_E_PARMS ;
    }

    char * buffer ;

    res = read_file(ctx, shell_out, argv[1], &buffer) ;
    if (res > 0) {

        /*
         * Run the script read from the file.
         */
        res = corshell_script_run (0, corshell_out, "", buffer, res) ;

        free (buffer) ;

    }

    return res ;

}

static int32_t
corshell_source2 (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    return corshell_source(ctx, shell_out, argv, argc) ;
}

static int32_t
corshell_cat (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    int32_t res  ;

    if (argc < 2) {
        return CORSHELL_CMD_E_PARMS ;
    }

    char * buffer ;

    res = read_file(ctx, shell_out, argv[1], &buffer) ;
    if (res > 0) {

        shell_out (ctx, CORSHELL_OUT_STD, buffer) ;
        shell_out (ctx, CORSHELL_OUT_STD, "\r\n") ;
        free (buffer) ;
        res = CORSHELL_CMD_E_OK ;

    }

    return res ;
}

static int32_t
corshell_echo (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    /*
     * Echo the first argument. Can be used to demostrate the string
     * substitution for registry strings, eg. "echo [test]"
     */
    if (argc < 2) {
        printf ("\r\n") ;

    } else {
        printf ("%s\r\n", argv[1]) ;

    }

    return CORSHELL_CMD_E_OK ;

}

static int32_t
corshell_exit (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    _shell_exit = true ;

    return CORSHELL_CMD_E_OK ;
}



