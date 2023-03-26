#include <stdio.h>
#include <stdint.h>
#include "minilzo.h"
#include "db.h"

#define  OUT_LEN(N)    ((N) + (N) / 16 + 64 + 3)


size_t Database::_ZFileRead(void *buf, size_t size, FILE *fp)
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
    unsigned char *zbuf = new unsigned char[_zsize];
    fread(zbuf, _zsize, 1, fp);
    lzo1x_decompress(zbuf, _zsize, (unsigned char *)buf, &size, NULL);
    delete[]zbuf;
    return _size;
}

void Database::_ZFileWrite(void *buf, size_t size, FILE *fp)
{
    unsigned char *zbuf   = new unsigned char[OUT_LEN(size)];
    char *         wrkmem = new char[LZO1X_1_MEM_COMPRESS];
    lzo_uint       zsize  = 0;

    lzo1x_1_compress((unsigned char *)buf, size, (unsigned char *)zbuf, &zsize, wrkmem);

    fwrite(&size, sizeof(uint32_t), 1, fp);
    fwrite(&zsize, sizeof(uint32_t), 1, fp);
    fwrite(zbuf, zsize, 1, fp);

    delete[]wrkmem;
    delete[]zbuf;
}

bool Database::Load(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        return false;
    }

    bool result = Load(fp);
    fclose(fp);

    return result;
}

bool Database::Load(FILE *fp)
{
    uint32_t length;
    fread(&length, sizeof(uint32_t), 1, fp);
    char* name = new char[length + 1];
    fread(name, length, 1, fp);
    Name = name;
    delete[]name;

    size_t size = _ZFileRead(NULL, 0, fp);
    char * buf  = new char[size];
    char * p    = buf;
    _ZFileRead(buf, size, fp);
    uint32_t *num = (uint32_t *)p;
    p += sizeof(uint32_t);
    for (uint32_t i = 0; i < *num; i++)
    {
        uint32_t *n = (uint32_t *)p;
        p += sizeof(uint32_t);
        _names.push_back(p);
        p += *n;
    }
    delete[]buf;

    size = _ZFileRead(NULL, 0, fp);
    buf = new char[size];
    _ZFileRead(buf, size, fp);

    _db_bytes.assign(buf, size);
    delete[]buf;

    return true;
}

bool Database::Save(const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        return false;
    }

    bool result = Save(fp);
    fclose(fp);

    return result;
}

bool Database::Save(FILE *fp)
{
    uint32_t length = Name.size() + 1;
    fwrite(&length, sizeof(uint32_t), 1, fp);
    fwrite(Name.c_str(), length, 1, fp);

    size_t size = 4;
    for (auto& n : _names)
    {
        size += 5 + n.size();
    }

    char *buf = new char[size];
    memset(buf, 0, size);
    char *p = buf;
    *(uint32_t *)p = _names.size();
    p += sizeof(uint32_t);
    for (auto& n : _names)
    {
        *(uint32_t *)p = n.size() + 1;
        p += sizeof(uint32_t);
        memcpy(p, n.c_str(), n.size());
        p += n.size() + 1;
    }

    _ZFileWrite(buf, size, fp);
    delete[]buf;

    _ZFileWrite((void*)_db_bytes.c_str(), _db_bytes.size(), fp);

    return true;
}

bool DB::Load(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        return false;
    }

    uint32_t num;
    fread(&num, sizeof(uint32_t), 1, fp);
    fread(&DefaultVOffset, sizeof(uint32_t), 1, fp);

    for (uint32_t i = 0; i < num; i++)
    {
        push_back(Database(fp));
    }

    fclose(fp);

    return true;
}

bool DB::Save(const char *path)
{
    FILE* fp = fopen(path, "wb");
    if (!fp)
    {
        return false;
    }

    uint32_t num = size();
    fwrite(&num, sizeof(uint32_t), 1, fp);
    fwrite(&DefaultVOffset, sizeof(uint32_t), 1, fp);
    for (auto &db : *this)
    {
        db.Save(fp);
    }

    fclose(fp);
    return true;
}
