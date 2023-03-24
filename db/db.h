#pragma once
#include <vector>
#include <string>

class DB
{
public:
    DB() :_db_bytes(nullptr), _db_size(0) {};
    virtual ~DB() {
        if (_db_bytes != nullptr)
        {
            delete[]_db_bytes;
        }
    };


    bool Load(const char *path);
    bool Save(const char *path);

    void AddName(const char* name) { _names.push_back(name); };
    const std::vector <std::string>& GetNames() { return _names; };
    
    void SetDb(const char* bytes, size_t size)
    {
        if (_db_bytes != nullptr)
        {
            delete[]_db_bytes;
        }
        _db_size = size;
        _db_bytes = new char[size];
        memcpy(_db_bytes, bytes, size);
    };

    const char* GetDb() { return _db_bytes; };
    size_t GetDbSize() { return _db_size; };

private:
    void _ZFileWrite(void* buf, size_t size, FILE* fp);
    size_t _ZFileRead(void * buf, size_t size, FILE* fp);
    std::vector <std::string> _names;
    char *_db_bytes;
    size_t _db_size;
};
