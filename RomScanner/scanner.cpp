#include <algorithm>
#include <unordered_map>
#include "scanner.h"
#include "hs.h"

bool Scanner::Scan(int db_index, const char *name, int voffset)
{
    FILE *fp = fopen(name, "rb");
    if (!fp)
    {
        printf("failed to open %s\n", name);
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

bool Scanner::Scan(int db_index, const char *buf, size_t size, int voffset)
{
    bool ret = false;
    if (voffset == -1)
    {
        voffset = _db->DefaultVOffset;
    }

    if (db_index >= 0)
    {
        ret = Scan(buf, size, &(*_db)[db_index], voffset);
    }
    else if (_db->IsStepped())
    {
        ret = SteppedScan(buf, size, voffset);
    }
    else
    {
        ret = Scan(buf, size, voffset);
    }

    return ret;
}

static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx)
{
    std::vector <ResultStruct> *results = (std::vector <ResultStruct> *)ctx;
    results->push_back({ (int32_t)id,    //index
                         (uint32_t)from, //start
                         (uint32_t)to    //end
                       });
    return 0;
}

bool Scanner::Scan(const char *buf, size_t size, Database *database, int voffset)
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
        _PostProcessResults(size, voffset, &database->GetNames());
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

bool Scanner::Scan(const char *buf, size_t size, int voffset)
{
    bool    ret = false;
    Results temp_results;

    uint32_t    max_matched = 0;
    std::string sdk_name;
    for (auto& db : *_db)
    {
        ret = Scan(buf, size, &db, voffset);
        if (!ret)
        {
            break;
        }

        uint32_t total_matched = _results.CountMarchedBytes(8);
        if (total_matched > max_matched)
        {
            temp_results = _results;
            sdk_name     = db.Name;
            max_matched  = total_matched;
        }
    }

    if (ret)
    {
        _results  = temp_results;
        _sdk_name = sdk_name;
    }
    return true;
}

bool Scanner::SteppedScan(const char *buf, size_t size, int voffset)
{
    // collect all results
    std::vector <Results> all_results;
    for (auto& db : *_db)
    {
        if (!Scan(buf, size, &db, voffset))
        {
            _results.clear();
            return false;
        }
        all_results.push_back(_results);
    }

    // find the best matched libs
    _sdk_name.clear();
    std::unordered_map <uint64_t, std::string> result_map;
    for (auto& level : _db->GetLevels())
    {
        int      best        = -1;
        uint32_t max_matched = 0;
        for (int i = level.start; i < level.end; i++)
        {
            Results *result        = &all_results[i];
            uint32_t total_matched = result->CountMarchedBytes(32);
            if (total_matched > max_matched)
            {
                max_matched = total_matched;
                best        = i;
            }
        }
        if (best > -1)
        {
            Results *result = &all_results[best];
            for (const auto& r : *result)
            {
                uint64_t key  = r.key();
                auto     iter = result_map.find(key);
                if (iter == result_map.end())
                {
                    result_map[key] = r.name;
                }
                else
                {
                    if (result_map[key].find(r.name) == std::string::npos)
                    {
                        result_map[key] += " / " + r.name;
                    }
                }
            }
            if (_sdk_name.size() == 0)
            {
                _sdk_name = (*_db)[best].Name;
            }
            else
            {
                _sdk_name += " + " + (*_db)[best].Name;
            }
        }
    }

    // merge results
    _results.clear();
    for (const auto&iter : result_map)
    {
        _results.push_back({ -1,                                  //index
                             (uint32_t)(iter.first >> 32),        //start
                             (uint32_t)(iter.first & 0xffffffff), //end
                             iter.second                          //name
                           });
    }

    _PostProcessResults(size, voffset, nullptr);
    return true;
}

void Scanner::PrintResults()
{
    printf("; %s\n", _sdk_name.c_str());
    for (const auto&r : _results)
    {
        printf("%08x %s\n", r.start, r.name.c_str());
    }
}

void Scanner::_PostProcessResults(size_t size, int voffset, const std::vector <std::string> *names)
{
    std::vector <bool> bytes_map(size, false);
    Results            new_results;

    std::sort(_results.begin(), _results.end(), [](ResultStruct a, ResultStruct b) {
        return a.size() > b.size();
    });

    for (auto& r : _results)
    {
        if (!bytes_map[r.start])
        {
            for (uint32_t i = r.start; i < r.end; i++)
            {
                bytes_map[i] = true;
            }
            r.start += voffset;
            r.end   += voffset;
            if (r.index > 0)
            {
                r.name = (*names)[r.index].c_str();
            }
            new_results.push_back(r);
        }
    }

    _results = new_results;

    std::sort(_results.begin(), _results.end(), [](ResultStruct a, ResultStruct b) {
        return a.start < b.start;
    });
}
