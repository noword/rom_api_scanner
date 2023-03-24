#pragma once
#include <vector>
#include "hs.h"
#include "db.h"

struct ResultStruct
{
	uint32_t index;
	uint32_t start;
	uint32_t end;
	const char* name;

	bool operator<(const ResultStruct& a) const
	{
		return index < a.index;
	}
};

class Scanner
{
public:
	Scanner();
	Scanner(const char* name) :Scanner() { LoadDatabase(name); };
	virtual ~Scanner();

	bool LoadDatabase(const char* name);
	
	bool ScanFile(const char* name, int voffset);
	bool ScanHex(char* buf, size_t size, int voffset);
	const std::vector<ResultStruct> GetResults() { return _results; };
	void PrintResults();

private:
	void _ToHexString(char *inbuf, char *hexbuf, size_t size);
	void _PostProcessResults(size_t size, int voffset);
	void _Deinitialize();

	DB _db;
	hs_database_t* _hs_db;
	hs_scratch_t* _hs_scratch;
	std::vector<ResultStruct> _results;
};