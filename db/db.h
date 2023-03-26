#pragma once
#include <vector>
#include <string>


// --- DB struct ---
// Offset  Size    Description
// 0       4       number of database
// 4       4       default virtual offset
// 8       ?       [database 0]
// ?       ?       [database 1]
// ...
//
//
// --- database struct ---
// Offset  Size    Description
// 0       4       length of name string with null terminator
// 4       length  name string
// ?       ?       compressed names block
// ?       ?       compressed hscan database block
//
//
// --- compressed block ---
// Offset  Size    Description
// 0       4       size
// 4       4       zsize
// 8       zsize   zbuf
//
//
// --- names struct ---
// Offset  Size    Description
// 0       4       number of names
// 4       4       length of first name string with null terminator
// 8       length  name string
// ?       ?       length of second
// ?       ?       name string
// ...


class Database
{
public:
    Database() {};
    Database(const char *path) : Database() { Load(path); };
    Database(FILE *fp) : Database() { Load(fp); };
    virtual ~Database()    {};

    bool Load(const char *path);
    bool Save(const char *path);
    bool Load(FILE *fp);
    bool Save(FILE *fp);

    void AddName(const char *name) { _names.push_back(name); };
    const std::vector <std::string>& GetNames() { return _names; };

    void SetDb(const char *bytes, size_t size)
    {
        _db_bytes.assign(bytes, size);
    };

    const char * GetDb() { return _db_bytes.c_str(); };
    size_t GetDbSize() { return _db_bytes.size(); };

    std::string Name;

private:
    size_t _ZFileRead(void *buf, size_t size, FILE *fp);
    void _ZFileWrite(void *buf, size_t size, FILE *fp);

    std::vector <std::string> _names;
    std::string _db_bytes;
};

class DB : public std::vector <Database>
{
public:
    uint32_t DefaultVOffset;
    bool Load(const char *path);
    bool Save(const char *path);
};
