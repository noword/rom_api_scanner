#include "hsdb.h"

char * __cdecl GetHsSerializeDatabaseBytes(const char **patterns,
                                           const unsigned int *flags,
                                           const unsigned int *ids,
                                           unsigned int num,
                                           size_t *size)
{
    hs_database_t *     db = nullptr;
    hs_compile_error_t *compileErr;
    hs_expr_ext **      ext = nullptr;
    ue2::Grey           grey;

    grey.limitPatternLength = 1000000;
    grey.limitLiteralLength = 1000000;

    hs_error_t err = ue2::hs_compile_multi_int(patterns,
                                               flags,
                                               ids,
                                               ext,
                                               num,
                                               HS_MODE_BLOCK,
                                               nullptr,
                                               &db,
                                               &compileErr,
                                               grey);

    if (err != HS_SUCCESS)
    {
        if (compileErr->expression >= 0)
        {
            printf("%d %s\n", compileErr->expression, patterns[compileErr->expression]);
        }
        printf("%s\n", compileErr->message);
        hs_free_compile_error(compileErr);
        return nullptr;
    }

    char *bytes = nullptr;
    *size = 0;
    err   = hs_serialize_database(db, &bytes, size);
    if (err != HS_SUCCESS)
    {
        printf("ERROR: hs_serialize_database() failed with error %u\n", err);
        bytes = nullptr;
    }

    hs_free_database(db);

    return bytes;
}
