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
};

class Scanner
{
public:
	Scanner(DB* db):_db(db) {};
	virtual ~Scanner() {};

	bool Scan(int db_index, const char* name, int voffset);
	bool Scan(int db_index, const char* buf, size_t size, int voffset);
	const std::vector<ResultStruct> GetResults() { return _results; };
	const std::string GetSdkName() { return _sdk_name; };
	void PrintResults();

private:
	void _PostProcessResults(size_t size, int voffset, const std::vector<std::string> &names);
	bool _Scan(const char *buf, size_t size, Database *database, int voffset);

	DB *_db;
	std::vector<ResultStruct> _results;
	std::string _sdk_name;
};