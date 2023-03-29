#include <stdio.h>
#include <stdint.h>
#include <unordered_map>
#include <string>
#include "db.h"
#include "lz4hc.h"



uint32_t _ZFileRead(void *buf, size_t size, FILE *fp)
{
    uint32_t _size;
    uint32_t _zsize;
    fread(&_size, sizeof(uint32_t), 1, fp);

    if (size < _size)
    {
        fseek(fp, -int(sizeof(uint32_t)), SEEK_CUR);
        return _size;
    }

    fread(&_zsize, sizeof(uint32_t), 1, fp);
    char *zbuf = new char[_zsize];
    fread(zbuf, _zsize, 1, fp);

    LZ4_decompress_safe(zbuf, (char *)buf, _zsize, size);

    delete[]zbuf;
    return _size;
}

// Don't forget delete[] !!!
char * _ZFileRead(FILE *fp)
{
    uint32_t size = _ZFileRead(nullptr, 0, fp);
    char *   buf  = new char[size];
    _ZFileRead(buf, size, fp);
    return buf;
}

void _ZFileWrite(void *buf, size_t size, FILE *fp)
{
    size_t zsize = LZ4_compressBound(size);
    char * zbuf  = new char[zsize];

    zsize = LZ4_compress_HC((const char *)buf, zbuf, size, zsize, LZ4HC_CLEVEL_MAX);

    fwrite(&size, sizeof(uint32_t), 1, fp);
    fwrite(&zsize, sizeof(uint32_t), 1, fp);
    fwrite(zbuf, zsize, 1, fp);

    delete[]zbuf;
}

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

bool DB::Load(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        return false;
    }

    ZDB_HEADER zdbheader;
    fread(&zdbheader, sizeof(ZDB_HEADER), 1, fp);

    fseek(fp, zdbheader.points_offset, SEEK_SET);
    char *    pbuf   = _ZFileRead(fp);
    uint32_t *points = (uint32_t *)pbuf;

    fseek(fp, zdbheader.strings_offset, SEEK_SET);
    char *strings = _ZFileRead(fp);

    fseek(fp, zdbheader.dbs_offset, SEEK_SET);
    char *hsdbs = _ZFileRead(fp);

    for (uint32_t i = 0; i < zdbheader.num; i++)
    {
        Database         db;
        DATABASE_HEADER *header = (DATABASE_HEADER *)points;
        points += sizeof(DATABASE_HEADER) / sizeof(uint32_t);
        db.Name = strings + header->name_offset;
        db.SetDb(hsdbs + header->db_offset, header->db_size);
        for (uint32_t j = 0; j < header->num; j++)
        {
            db.AddName(strings + *points++);
        }
        push_back(db);
    }

    delete[]pbuf;
    delete[]strings;
    delete[]hsdbs;

    fclose(fp);

    return true;
}

void DB::_GetSerializedBufSize(size_t *points_size, size_t *strings_size, size_t *hsdbs_size)
{
    *points_size  = 0;
    *strings_size = 0;
    *hsdbs_size   = 0;
    for (auto& db : *this)
    {
        *points_size += sizeof(DATABASE_HEADER) / sizeof(uint32_t);
        *points_size += db.GetNames().size();

        *strings_size += db.Name.size() + 1;
        for (auto& name : db.GetNames())
        {
            *strings_size += name.size() + 1;
        }

        *hsdbs_size += db.GetDbSize();
    }

    *points_size *= sizeof(uint32_t);
}

class StringBuf
{
public:
    StringBuf(char *buf) : _p(buf), _start(buf) {};
    virtual ~StringBuf() {};

    uint32_t Add(std::string s)
    {
        uint32_t result;
        auto     iter = _map.find(s);
        if (iter == _map.end())
        {
            result = _map[s] = uint32_t(_p - _start);
            strcpy(_p, s.c_str());
            _p += s.size() + 1;
        }
        else
        {
            result = iter->second;
        }
        return result;
    }

    size_t Size() { return _p - _start; };

private:
    char *_start;
    char *_p;
    std::unordered_map <std::string, uint32_t> _map;
};

void DB::_SerializeToMemory(char *points, char *strings, char *hsdbs, size_t *strings_size)
{
    uint32_t *pp = (uint32_t *)points;
    char *    hp = hsdbs;
    StringBuf sb(strings);

    for (auto& db : *this)
    {
        DATABASE_HEADER header = { sb.Add(db.Name), db.GetNames().size(), hp - hsdbs, db.GetDbSize()};

        memcpy(pp, &header, sizeof(DATABASE_HEADER));
        pp += sizeof(DATABASE_HEADER) / sizeof(uint32_t);

        memcpy(hp, db.GetDb(), db.GetDbSize());
        hp += db.GetDbSize();

        for (auto& name : db.GetNames())
        {
            *pp++ = sb.Add(name);
        }
    }

    *strings_size = sb.Size();
}

bool DB::Save(const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        return false;
    }

    size_t points_size;
    size_t strings_size;
    size_t hsdbs_size;

    _GetSerializedBufSize(&points_size, &strings_size, &hsdbs_size);

    char *points  = new char[points_size];
    char *strings = new char[strings_size];
    char *hsdbs   = new char[hsdbs_size];

    _SerializeToMemory(points, strings, hsdbs, &strings_size);

    ZDB_HEADER header = { size(), DefaultVOffset };

    fseek(fp, sizeof(ZDB_HEADER), SEEK_SET);

    header.points_offset = ftell(fp);
    _ZFileWrite(points, points_size, fp);

    header.strings_offset = ftell(fp);
    _ZFileWrite(strings, strings_size, fp);

    header.dbs_offset = ftell(fp);
    _ZFileWrite(hsdbs, hsdbs_size, fp);

    fseek(fp, 0, SEEK_SET);
    fwrite(&header, sizeof(ZDB_HEADER), 1, fp);

    delete[]points;
    delete[]strings;
    delete[]hsdbs;

    fclose(fp);
    return true;
}
