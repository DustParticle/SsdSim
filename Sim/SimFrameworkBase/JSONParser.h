#ifndef __JSONParser__
#define __JSONParser__

#include <string>
#include <cstdio>
#include <exception>

#include <rapidjson/document.h>

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
	void Parse(const std::string& filename);

	bool GetValueBool(const std::string &memberValue);
	const char* GetValueString(const std::string &memberValue);
	int GetValueInt(const std::string &memberValue);

	int GetValueIntForAttribute(const std::string &attributes, const std::string &memberValue);
	const char* GetValueStringForAttribute(const std::string &attributes, const std::string &memberValue);

private:
	rapidjson::Document _Document;
};

#endif