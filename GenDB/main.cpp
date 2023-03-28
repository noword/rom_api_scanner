#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "argtable3.h"
#include "db.h"
#include "hs.h"

#define BUF_SIZE 0x100000

char * GetHsSerializeDatabaseBytes(const char **patterns,
                                   const unsigned int *flags,
                                   const unsigned int *ids,
                                   unsigned int num,
                                   size_t *size)
{
    hs_database_t *     db = nullptr;
    hs_compile_error_t *compileErr;
    hs_error_t          err = hs_compile_multi(patterns,
                                               flags,
                                               ids,
                                               num,
                                               HS_MODE_BLOCK,
                                               nullptr,
                                               &db,
                                               &compileErr);


    if (err != HS_SUCCESS)
    {
        if (compileErr->expression >= 0)
        {
            printf("%d %s\n", compileErr->expression, patterns[compileErr->expression]);
            //printf("%s\n", names[compileErr->expression]);
        }
        printf("%s\n", compileErr->message);
        hs_free_compile_error(compileErr);
        return nullptr;
    }

    char *bytes = nullptr;
    *size = 0;
    err  = hs_serialize_database(db, &bytes, size);
    if (err != HS_SUCCESS)
    {
        printf("ERROR: hs_serialize_database() failed with error %u\n", err);
        bytes = nullptr;
    }

    hs_free_database(db);

    return bytes;
}

bool ReadLine(char* line, int size, FILE* fp)
{
    if (fgets(line, size, fp) == NULL)
    {
        return false;
    }
    line[strcspn(line, "\r\n")] = 0;
    return true;
}

bool GenDatabase(const char *txt_name, Database *datebase)
{
    FILE *fp = fopen(txt_name, "r");
    if (!fp)
    {
        printf("failed to open %s\n", txt_name);
        return false;
    }

    char *buf = new char[BUF_SIZE];
    ReadLine(buf, BUF_SIZE, fp);
    datebase->Name = buf;
    
    std::vector<std::string> pattern_strs;
    while (true)
    {
        if (!ReadLine(buf, BUF_SIZE, fp))
        {
            break;
        }
        datebase->AddName(buf);

        if (!ReadLine(buf, BUF_SIZE, fp))
        {
            break;
        }
        pattern_strs.push_back(buf);
    }

    delete[]buf;
    fclose(fp);

    size_t num = datebase->GetNames().size();
    if (num != pattern_strs.size())
    {
        printf("the number of names and patterns are not same, check your input file\n");
        return false;
    }

    const char **patterns = new const char * [num];
    const char **names    = new const char * [num];
    unsigned *   ids      = new unsigned[num];
    unsigned *   flags    = new unsigned[num];

    for (uint32_t i = 0; i < num; i++)
    {
        names[i] = datebase->GetNames()[i].c_str();
        patterns[i] = pattern_strs[i].c_str();
        ids[i]   = i;
        flags[i] = HS_FLAG_SOM_LEFTMOST;
    }

    size_t size;
    char* bytes = GetHsSerializeDatabaseBytes(patterns, flags, ids, num, &size);
    if (bytes == nullptr)
    {
        return false;
    }

    datebase->SetDb(bytes, size);
    
    free(bytes);
    delete[]patterns;
    delete[]names;
    delete[]ids;
    delete[]flags;

    return true;
}

void Generate(const char *db_name, const char **txt_names, int count, int voffset)
{
    DB db;
    
    db.DefaultVOffset = voffset;
    for (int i = 0; i < count; i++)
    {
        printf("processing %s\n", txt_names[i]);
        Database database;
        GenDatabase(txt_names[i], &database);
        db.push_back(database);
    }
    db.Save(db_name);
}

int main(int argc, char **argv)
{
    struct arg_file *db_file         = arg_file1(NULL, NULL, "DB", "database file");
    struct arg_file *txt_files       = arg_filen(NULL, NULL, "TXT", 1, 255, "txt files generating by the script collect.py");
    struct arg_int * default_voffset = arg_int0("o", "voffset", "<int>", "set the default virtual offset");
    struct arg_lit * help            = arg_lit0(NULL, "help", "print this help and exit");
    struct arg_end * end             = arg_end(20);

    void *argtable[] = { db_file, txt_files, default_voffset, help, end };

    default_voffset->ival[0] = 0;
    arg_parse(argc, argv, argtable);

    if (help->count > 0 || txt_files->count == 0 || db_file->count == 0)
    {
        printf("Usage: GenDB");
        arg_print_syntax(stdout, argtable, "\n");
        arg_print_glossary_gnu(stdout, argtable);
    }
    else
    {
        Generate(*db_file->filename, txt_files->filename, txt_files->count, default_voffset->ival[0]);
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return 0;
}
