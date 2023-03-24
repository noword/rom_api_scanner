#include <stdio.h>
#include <stdint.h>
#include "minilzo.h"
#include "db.h"

#define  OUT_LEN(N)     ((N) + (N) / 16 + 64 + 3)

size_t DB::_ZFileRead(void* buf, size_t size, FILE* fp)
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
	unsigned char* zbuf = new unsigned char[_zsize];
	fread(zbuf, _zsize, 1, fp);
	lzo1x_decompress(zbuf, _zsize, (unsigned char*)buf, &size, NULL);
	delete[]zbuf;
	return _size;
}

void DB::_ZFileWrite(void* buf, size_t size, FILE* fp)
{
	unsigned char* zbuf = new unsigned char[OUT_LEN(size)];
	char* wrkmem = new char[LZO1X_1_MEM_COMPRESS];
	lzo_uint zsize = 0;

	lzo1x_1_compress((unsigned char*)buf, size, (unsigned char*)zbuf, &zsize, wrkmem);
	
	fwrite(&size, sizeof(uint32_t), 1, fp);
	fwrite(&zsize, sizeof(uint32_t), 1, fp);
	fwrite(zbuf, zsize, 1, fp);

	delete[]wrkmem;
	delete[]zbuf;
}


bool DB::Load(const char* path)
{
	FILE* fp = fopen(path, "rb");
	if (!fp)
	{
		return false;
	}
	size_t size = _ZFileRead(NULL, 0, fp);
	char* buf = new char[size];
	char* p = buf;
	_ZFileRead(buf, size, fp);
	uint32_t* num = (uint32_t*)p;
	p += sizeof(uint32_t);
	for (uint32_t i = 0; i < *num; i++)
	{
		uint32_t* n = (uint32_t*)p;
		p += sizeof(uint32_t);
		_names.push_back(p);
		p += *n;
	}
	delete[]buf;
	
	_db_size = _ZFileRead(NULL, 0, fp);
	_db_bytes = new char[_db_size];
	_ZFileRead(_db_bytes, _db_size, fp);

	fclose(fp);
	return true;
}

bool DB::Save(const char* path)
{
	FILE* fp = fopen(path, "wb");
	if (!fp)
	{
		return false;
	}

	size_t size = 4;
	for (auto& n : _names)
	{
		size += 5 + n.size();
	}

	char* buf = new char[size];
	memset(buf, 0, size);
	char* p = buf;
	*(uint32_t*)p = _names.size();
	p += sizeof(uint32_t);
	for (auto& n : _names)
	{
		*(uint32_t*)p = n.size() + 1;
		p += sizeof(uint32_t);
		memcpy(p, n.c_str(), n.size());
		p += n.size() + 1;
	}

	_ZFileWrite(buf, size, fp);
	_ZFileWrite(_db_bytes, _db_size, fp);

	delete[]buf;

	fclose(fp);
	return true;
}