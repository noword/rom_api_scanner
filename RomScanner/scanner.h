#pragma once
#include <vector>
#include "db.h"

struct ResultStruct
{
    uint32_t    index;
    uint32_t    start;
    uint32_t    end;
    const char *name;
    uint32_t size() const { return end - start; };
};

//using ResultStructVector = std::vector<ResultStruct>;

class Results :public std::vector<ResultStruct>
{
public:
    uint32_t CountMarchedBytes(uint32_t min_size=0)
    {
        uint32_t total = 0;
        for (const auto& r : *this)
        {
            uint32_t s = r.size();
            if (s > min_size)
            {
                total += s;
            }
        }
        return total;
    }
};

class Scanner
{
public:
    Scanner(DB *db) : _db(db) {};
    virtual ~Scanner() {};

    bool Scan(int db_index, const char *name, int voffset = -1);
    bool Scan(int db_index, const char *buf, size_t size, int voffset = -1);
    bool Scan(const char *buf, size_t size, Database *database, int voffset = -1);
    bool Scan(const char *buf, size_t size, int voffset = -1);
    bool SteppedScan(const char *buf, size_t size, int voffset = -1);

    const Results &GetResults() { return _results; };
    const std::string GetSdkName() { return _sdk_name; };
    void PrintResults();

private:
    void _PostProcessResults(size_t size, int voffset, const std::vector <std::string>&names);


    DB *_db;
    Results _results;
    std::string _sdk_name;
};
