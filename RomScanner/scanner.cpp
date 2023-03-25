#include <algorithm>

#include "scanner.h"

Scanner::Scanner():_hs_db(nullptr), _hs_scratch(nullptr)
{
}

Scanner::~Scanner()
{
    _Deinitialize();
}

void Scanner::_Deinitialize()
{
    if (_hs_scratch)
    {
        hs_free_scratch(_hs_scratch);
        _hs_scratch = nullptr;
    }

    if (_hs_db)
    {
        hs_free_database(_hs_db);
        _hs_db = nullptr;
    }
}

bool Scanner::LoadDatabase(const char *name)
{
    _Deinitialize();
    
    hs_error_t err = HS_INVALID;
    if (_db.Load(name))
    {
        hs_error_t  err = hs_deserialize_database(_db.GetDb(), _db.GetDbSize(), &_hs_db);
        if (err != HS_SUCCESS)
        {
            printf("ERROR: Failed to deserialize database (%d)", err);
        }

        err = hs_alloc_scratch(_hs_db, &_hs_scratch);
        if (err != HS_SUCCESS)
        {
            printf("ERROR: Unable to allocate scratch space (%d)\n", err);
            hs_free_database(_hs_db);
        }
    }
    return err == HS_SUCCESS;
}

bool Scanner::Scan(const char *name, int voffset)
{
    FILE *fp = fopen(name, "rb");
    if (!fp)
    {
        return false;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* inbuf = new char[size];
    fread(inbuf, size, 1, fp);
    fclose(fp);
    
    bool result = Scan(inbuf, size, voffset);
    
    delete[]inbuf;

    return result;
}

static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx)
{
    std::vector<ResultStruct>* results = (std::vector<ResultStruct> *)ctx;
    results->push_back({id, (unsigned int) from, (unsigned int)to, nullptr});
    return 0;
}

bool Scanner::Scan(char *buf, size_t size, int voffset)
{
    if (voffset == -1)
    {
        voffset = _db.DefaultVOffset;
    }

    _results.clear();
    hs_error_t err = hs_scan(_hs_db, buf, size, 0, _hs_scratch, eventHandler, &_results);
    if ( err != HS_SUCCESS)
    {
        printf("ERROR: Unable to scan input buffer (%d)\n", err);
        _results.clear();
    }
    else
    {
        _PostProcessResults(size , voffset);
    }
    return err == HS_SUCCESS;
}

void Scanner::PrintResults()
{
    for (const auto &r : _results)
    {
        printf("%08x  %s\n", r.start, r.name);
    }
}


void Scanner::_PostProcessResults(size_t size, int voffset)
{
    std::vector<bool> map(size, false);
    std::vector<ResultStruct> new_results;

    std::sort(_results.begin(), _results.end(), [](ResultStruct a, ResultStruct b) { return a.index < b.index; });
    for (auto& r : _results)
    {
        if (!map[r.start])
        {
            for (uint32_t i = r.start; i < r.end; i++)
            {
                map[i] = true;
            }
            r.start += voffset;
            r.end += voffset;
            r.name = _db.GetNames()[r.index].c_str();
            new_results.push_back(r);
        }
    }
    
    _results = new_results;

    std::sort(_results.begin(), _results.end(), [](ResultStruct a, ResultStruct b) { return a.start < b.start; });
}