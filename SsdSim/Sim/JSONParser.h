#ifndef __JSONParser__
#define __JSONParser__

#include <string>
#include <cstdio>
#include <exception>

#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include "Common/Types.h"





class JSONParser
{
public:
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

	class Exception : public std::exception
	{
	public:
		Exception(JSONParser::Error err) : err(err) {}

		virtual const char * what() const throw();

	private:
		JSONParser::Error err;
	};

public:
	JSONParser(const std::string& filename);
	bool Open();

	bool GetValueBool(const std::string &memberValue);
	const char* GetValueString(const std::string &memberValue);
	int GetValueInt(const std::string &memberValue);

	int GetValueIntForAttribute(const std::string &attributes, const std::string &memberValue);

private:
	std::string sJsonName;
	std::shared_ptr<rapidjson::Document> m_Document;
};

#endif