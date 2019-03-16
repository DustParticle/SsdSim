#include "JSONParser.h"

#include <fstream>
#include <rapidjson/istreamwrapper.h>

using namespace rapidjson;

void JSONParser::Parse(const std::string& filename)
{
	std::ifstream ifs(filename);

	if (!ifs.is_open())
	{
		throw JSONParser::Exception(JSONParser::Error::FileOpenFailed);
	}

	rapidjson::IStreamWrapper isw(ifs);

	_Document.ParseStream(isw);
}

bool JSONParser::GetValueBool(const std::string &memberValue)
{
	if (!_Document.HasMember(memberValue.c_str()))
	{
		throw Exception(Error::MemberValueInvalid);
	}

	Value::MemberIterator memberValueItr = _Document.FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsBool())
	{
		throw Exception(Error::MemberValueIsNotBool);
	}

	return memberValueItr->value.GetBool();
}

const char* JSONParser::GetValueString(const std::string &memberValue)
{
	if (!_Document.HasMember(memberValue.c_str()))
	{
		throw Exception(Error::MemberValueInvalid);
	}

	Value::MemberIterator memberValueItr = _Document.FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsString())
	{
		throw Exception(Error::MemberValueIsNotSring);
	}
	
	return memberValueItr->value.GetString();
}

int JSONParser::GetValueInt(const std::string &memberValue)
{
	if (!_Document.HasMember(memberValue.c_str()))
	{
		throw Exception(Error::MemberValueInvalid);
	}
	Value::MemberIterator memberValueItr = _Document.FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsInt())
	{
		throw Exception(Error::MemverValueIsNotInt);
	}

	return memberValueItr->value.GetInt();
}

int JSONParser::GetValueIntForAttribute(const std::string &attributes, const std::string &memberValue)
{
	if (!_Document.HasMember(attributes.c_str()))
	{
		throw Exception(Error::AttributeInvalid);
	}

	Value::MemberIterator memberAttributeItr = _Document.FindMember(attributes.c_str());
	
	if (!memberAttributeItr->value.IsObject())
	{
		throw Exception(Error::AttributeIsNotObject);
	}

	if (!memberAttributeItr->value.HasMember(memberValue.c_str()))
	{
		throw Exception(Error::MemberValueInvalid);
	}

	Value::ConstMemberIterator memberValueItr = memberAttributeItr->value.FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsInt())
	{
		throw Exception(Error::MemverValueIsNotInt);
	}

	return memberValueItr->value.GetInt();
}

const char* JSONParser::GetValueStringForAttribute(const std::string &attributes, const std::string &memberValue)
{
	if (!_Document.HasMember(attributes.c_str()))
	{
		throw Exception(Error::AttributeInvalid);
	}

	Value::MemberIterator memberAttributeItr = _Document.FindMember(attributes.c_str());

	if (!memberAttributeItr->value.IsObject())
	{
		throw Exception(Error::AttributeIsNotObject);
	}

	if (!memberAttributeItr->value.HasMember(memberValue.c_str()))
	{
		throw Exception(Error::MemberValueInvalid);
	}

	Value::ConstMemberIterator memberValueItr = memberAttributeItr->value.FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsString())
	{
		throw Exception(Error::MemberValueIsNotSring);
	}

	return memberValueItr->value.GetString();
}

const char * JSONParser::Exception::what() const throw ()
{
	const char *strErr = nullptr;
	switch (err)
	{
	case Error::FileOpenFailed:
	{
		strErr = "FileOpenFailed";
		break;
	}
	case Error::AttributeInvalid:
	{
		strErr = "AttributeInvalid";
		break;
	}
	case Error::AttributeIsNotObject:
	{
		strErr = "AttributeIsNotObject";
		break;
	}
	case Error::MemberValueInvalid:
	{
		strErr = "MemberValueInvalid";
		break;
	}
	case Error::MemberValueIsNotBool:
	{
		strErr = "MemberValueIsNotBool";
		break;
	}
	case Error::MemberValueIsNotSring:
	{
		strErr = "MemberValueIsNotSring";
		break;
	}
	case Error::MemverValueIsNotInt:
	{
		strErr = "MemverValueIsNotInt";
		break;
	}
	case Error::ReturnValueInvalid:
	{
		strErr = "ReturnValueInvalid";
		break;
	}
	default:
	{
		break;
	}
	}
	return strErr;
}
