#pragma once
#include <vector>
#include <string>

// --- zdb format ---
// ZDB_HEADER
// compressed points block
// compressed strings block
// compressed hyperscan database block
//
// --- compress block ---
// uint32_t size
// uint32_t zsize
// char*    zbuf

struct ZDB_HEADER
{
    uint32_t num;
    uint32_t voffset;
    uint32_t points_offset;
    uint32_t strings_offset;
    uint32_t dbs_offset;
};

struct DATABASE_HEADER
{
    uint32_t name_offset;
    uint32_t num;
    uint32_t db_offset;
    uint32_t db_size;
};

class Database
{
public:
    void AddName(const char *name) { _names.push_back(name); };
    void SetDb(const char* bytes, size_t size)
    {
        _db_bytes.assign(bytes, size);
    };

    const std::vector <std::string>& GetNames() { return _names; };
    const char * const GetDb() { return _db_bytes.c_str(); };
    size_t GetDbSize() { return _db_bytes.size(); };
    std::string Name;

private:
    std::vector <std::string> _names;
    std::string _db_bytes;
};


class DB : public std::vector <Database>
{
public:
    uint32_t DefaultVOffset;
    bool Load(const char *path);
    bool Save(const char *path);

private:
    void _GetSerializedBufSize(size_t *points_size, size_t *strings_size, size_t *hsdbs_size);
    void _SerializeToMemory(char *points, char *strings, char *hsdbs, size_t *strings_size);
};
