#ifndef __JSONParser__
#define __JSONParser__

#include <string>
#include <cstdio>

#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include "Common/Types.h"


enum class ParserError
{
	ErrorNone =0,
	FileFailed,
	ElementInvalid,
	IntInvalid,
	BoolInvalid,
	StringInvalid
};

class JSONParser
{
public:
	JSONParser(const std::string& name);
	bool Open();

	bool GetValueBool(std::string sElement);
	const char* GetValueString(std::string sElement);
	int GetValueInt(std::string sElement);

private:
	std::string sJsonName;
	std::shared_ptr<rapidjson::Document> m_Document;
};

#endif