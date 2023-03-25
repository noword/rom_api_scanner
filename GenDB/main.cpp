#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "argtable3.h"
#include "db.h"
#include "hs.h"


void convert(const char *bin_name, const char *hs_name, int voffset)
{
    FILE *fp = fopen(bin_name, "rb");
    if (!fp)
    {
        printf("failed to open %s\n", bin_name);
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = new char[size];
    fread(buf, size, 1, fp);
    fclose(fp);

    char *p = buf;

    uint32_t *num = (uint32_t *)p;
    p   += sizeof(uint32_t);

    const char **patterns = new const char *[*num];
    const char **names    = new const char * [*num];
    unsigned *   ids      = new unsigned[*num];
    unsigned *   flags    = new unsigned[*num];

    uint32_t *n;
    for (uint32_t i = 0; i < *num; i++)
    {
        n        = (uint32_t *)p;
        p       += sizeof(uint32_t);
        names[i] = p;
        p       += *n;

        n           = (uint32_t *)p;
        p          += sizeof(uint32_t);
        patterns[i] = p;
        p          += *n;

        ids[i] = i;
        flags[i] = HS_FLAG_SOM_LEFTMOST;
    }

    hs_database_t *     db = nullptr;
    hs_compile_error_t *compileErr;
    hs_error_t          err = hs_compile_multi(patterns,
                                               flags,
                                               ids,
                                               *num,
                                               HS_MODE_BLOCK,
                                               nullptr,
                                               &db,
                                               &compileErr);


    if (err != HS_SUCCESS)
    {
        if (compileErr->expression >= 0)
        {
            printf("%d %s\n", compileErr->expression, patterns[compileErr->expression]);
            printf("%s\n", names[compileErr->expression]);
        }
        printf("%s\n", compileErr->message);
        hs_free_compile_error(compileErr);
    }

    char *bytes = nullptr;
    size = 0;
    err  = hs_serialize_database(db, &bytes, &size);
    if (err != HS_SUCCESS)
    {
        printf("ERROR: hs_serialize_database() failed with error %u\n", err);
    }
    else
    {
        DB _db;
        _db.DefaultVOffset = voffset;
        for (int i = 0; i < *num; i++)
        {
            _db.AddName(names[i]);
        }
        _db.SetDb(bytes, size);
        _db.Save(hs_name);
        free(bytes);
    }

    delete[]patterns;
    delete[]ids;
    delete[]flags;
    delete[]buf;
    fclose(fp);
}

int main(int argc, char **argv)
{
    struct arg_file *bin_file = arg_file1(NULL, NULL, "BIN", "bin file generating by collect.py");
    struct arg_file *hs_file  = arg_file1(NULL, NULL, "DB", "database file");
    struct arg_int* default_voffset = arg_int0("v", "voffset", "<int>", "set default virtual offset");
    struct arg_lit * help     = arg_lit0(NULL, "help", "print this help and exit");
    struct arg_end * end      = arg_end(20);

    void *argtable[] = { bin_file, hs_file,default_voffset, help, end };

    default_voffset->ival[0] = 0;
    arg_parse(argc, argv, argtable);

    if (help->count > 0 || bin_file->count == 0 || hs_file->count == 0)
    {
        printf("Usage: GenDB");
        arg_print_syntax(stdout, argtable, "\n");
        arg_print_glossary_gnu(stdout, argtable);
    }
    else
    {
        convert(*bin_file->filename, *hs_file->filename, default_voffset->ival[0]);
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return 0;
}
