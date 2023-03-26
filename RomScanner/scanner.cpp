#include <algorithm>
#include "scanner.h"


bool Scanner::Scan(int db_index, const char *name, int voffset)
{
    FILE *fp = fopen(name, "rb");
    if (!fp)
    {
        return false;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *inbuf = new char[size];
    fread(inbuf, size, 1, fp);
    fclose(fp);

    bool result = Scan(db_index, inbuf, size, voffset);

    delete[]inbuf;

    return result;
}

static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx)
{
    std::vector <ResultStruct> *results = (std::vector <ResultStruct> *)ctx;
    results->push_back({ id, (unsigned int)from, (unsigned int)to, nullptr });
    return 0;
}

bool Scanner::_Scan(const char *buf, size_t size, Database *database, int voffset)
{
    hs_database_t *_hs_db      = nullptr;
    hs_scratch_t * _hs_scratch = nullptr;

    hs_error_t err = hs_deserialize_database(database->GetDb(), database->GetDbSize(), &_hs_db);
    if (err != HS_SUCCESS)
    {
        printf("ERROR: Failed to deserialize database (%d)", err);
        goto _SCAN_END;
    }

    err = hs_alloc_scratch(_hs_db, &_hs_scratch);
    if (err != HS_SUCCESS)
    {
        printf("ERROR: Unable to allocate scratch space (%d)\n", err);
        goto _SCAN_END;
    }

    _results.clear();
    err = hs_scan(_hs_db, buf, size, 0, _hs_scratch, eventHandler, &_results);
    if (err != HS_SUCCESS)
    {
        printf("ERROR: Unable to scan input buffer (%d)\n", err);
        _results.clear();
        goto _SCAN_END;
    }
    else
    {
        _PostProcessResults(size, voffset, database->GetNames());
        _sdk_name = database->Name;
    }

_SCAN_END:
    if (_hs_scratch != nullptr)
    {
        hs_free_scratch(_hs_scratch);
    }

    if (_hs_db != nullptr)
    {
        hs_free_database(_hs_db);
    }

    return err == HS_SUCCESS;
}

bool Scanner::Scan(int db_index, const char *buf, size_t size, int voffset)
{
    if (voffset == -1)
    {
        voffset = _db->DefaultVOffset;
    }

    if (db_index >= 0)
    {
        _Scan(buf, size, &(*_db)[db_index], voffset);
    }
    else
    {
        std::vector <ResultStruct> temp_results;

        uint32_t max_matched = 0;
        std::string sdk_name;
        for (auto& db : *_db)
        {
            _Scan(buf, size, &db, voffset);

            uint32_t total_matched = 0;
            for (auto& r : _results)
            {
                uint32_t s = r.end - r.start;
                if (s > 8)
                {
                    total_matched += s;
                }
            }

            if (total_matched > max_matched)
            {
                temp_results = _results;
                sdk_name = db.Name;
                max_matched = total_matched;
            }
        }

        _results = temp_results;
        _sdk_name = sdk_name;
    }
}

void Scanner::PrintResults()
{
    printf("; %s\n", _sdk_name.c_str());
    for (const auto&r : _results)
    {
        printf("%08x %s\n", r.start, r.name);
    }
}

void Scanner::_PostProcessResults(size_t size, int voffset, const std::vector <std::string>& names)
{
    std::vector <bool>         map(size, false);
    std::vector <ResultStruct> new_results;

    std::sort(_results.begin(), _results.end(), [](ResultStruct a, ResultStruct b) {
        return a.index < b.index;
    });

    for (auto& r : _results)
    {
        if (!map[r.start])
        {
            for (uint32_t i = r.start; i < r.end; i++)
            {
                map[i] = true;
            }
            r.start += voffset;
            r.end   += voffset;
            r.name   = names[r.index].c_str();
            new_results.push_back(r);
        }
    }

    _results = new_results;

    std::sort(_results.begin(), _results.end(), [](ResultStruct a, ResultStruct b) {
        return a.start < b.start;
    });
}
