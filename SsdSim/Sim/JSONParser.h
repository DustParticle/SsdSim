#ifndef __JSONParser__
#define __JSONParser__

#include <string>
#include <cstdio>

#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include "Common/Types.h"


enum class Error
{
	ErrorNone = 2,
	FileOpenFailed,
	ReturnValueInvalid,

	AttributeInvalid,
	AttributeIsNotObject,

	MemberValueInvalid,
	MemverValueIsNotInt,
	MemberValueIsNotSring,
	MemberValueIsNotBool
};

class JSONParser
{
public:
	JSONParser(const std::string& filename);
	bool Open();

	static std::string ErrorToString(Error err);
	bool GetValueBool(std::string &memberValue);
	const char* GetValueString(std::string &memberValue);
	int GetValueInt(const std::string &memberValue);

	int GetValueIntForAttribute(const std::string &attributes, const std::string &memberValue);

private:
	std::string sJsonName;
	std::shared_ptr<rapidjson::Document> m_Document;
};

#endif