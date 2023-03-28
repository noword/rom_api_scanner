#include <stdio.h>
#include "argtable3.h"
#include "scanner.h"

#define VERSION    "v0.0.6"

int main(int argc, char **argv)
{
    struct arg_file *dbfile  = arg_file1(NULL, NULL, "DB", "Database file");
    struct arg_file *file    = arg_file0(NULL, NULL, "ROM", "Rom file for scanning. If not provided, display the database information");
    struct arg_int * voffset = arg_int0("o", "voffset", NULL, "Virtual offset. If not provided, the default value in the database will be used");
    struct arg_int * index   = arg_int0("i", "index", NULL, "Choose which database for scanning");
    struct arg_lit * version = arg_lit0("v", "version", "Show the version information");
    struct arg_lit * help    = arg_lit0(NULL, "help", "Print this help and exit");
    struct arg_end * end     = arg_end(20);

    void *argtable[] = { dbfile, file, voffset, index, version, help, end };
    voffset->ival[0] = -1;
    index->ival[0]   = -1;
    arg_parse(argc, argv, argtable);

    if (version->count > 0)
    {
        printf("RomScanner " VERSION "\n");
    }
    else if (help->count > 0 || dbfile->count == 0)
    {
        printf("Usage: RomScanner ");
        arg_print_syntax(stdout, argtable, "\n");
        printf("Options are:\n");
        arg_print_glossary_gnu(stdout, argtable);
    }
    else
    {
        DB db;
        db.Load(*dbfile->filename);

        if (file->count > 0)
        {
            Scanner scanner(&db);
            scanner.Scan(index->ival[0], file->filename[0], voffset->ival[0]);
            scanner.PrintResults();
        }
        else
        {
            for (size_t i = 0; i < db.size(); i++)
            {
                printf("(%llu) %s / %llu\n", i, db[i].Name.c_str(), db[i].GetNames().size());
            }
        }
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return 0;
}
