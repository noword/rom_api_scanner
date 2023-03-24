#include <stdio.h>
#include "argtable3.h"
#include "scanner.h"


int main(int argc, char **argv)
{
    struct arg_file* dbfile = arg_file0(NULL, NULL, "DB", "database file");
    struct arg_file *file    = arg_file0(NULL, NULL, "ROM", "rom file for scanning");
    struct arg_int* voffset = arg_int0("v", "voffset", NULL, "virtual offset");
    struct arg_lit * help    = arg_lit0(NULL, "help", "print this help and exit");
    struct arg_end * end     = arg_end(20);

    void *argtable[] = { dbfile, file, voffset, help, end };
    voffset->ival[0] = 0;
    arg_parse(argc, argv, argtable);

    if (help->count > 0 || file->count == 0)
    {
        printf("Usage: RomScanner");
        arg_print_syntax(stdout, argtable, "\n");
        arg_print_glossary_gnu(stdout, argtable);
    }
    else
    {
        Scanner scanner(*dbfile->filename);
        scanner.ScanFile(*file->filename, voffset->ival[0]);
        scanner.PrintResults();
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return 0;
}
