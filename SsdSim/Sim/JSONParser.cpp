#include "JSONParser.h"

#include <fstream>

using namespace rapidjson;

JSONParser::JSONParser(const std::string& jsonFilename) : sJsonName(jsonFilename)
{
}

bool JSONParser::Open()
{
	std::ifstream ifs(sJsonName);

	if (!ifs.is_open())
	{
		return false;
	}

	rapidjson::IStreamWrapper isw(ifs);

	m_Document = std::make_shared<rapidjson::Document>();
	m_Document->ParseStream(isw);

	return true;
}

bool JSONParser::GetValueBool(std::string memberValue)
{
	if (!m_Document->HasMember(memberValue.c_str()))
	{
		throw ParserError::MemberValueInvalid;
	}

	Value::MemberIterator memberValueItr = m_Document->FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsBool())
	{
		throw ParserError::MemberValueIsNotBool;
	}

	return memberValueItr->value.GetBool();
}

const char* JSONParser::GetValueString(std::string memberValue)
{
	if (!m_Document->HasMember(memberValue.c_str()))
	{
		throw ParserError::MemberValueInvalid;
	}

	Value::MemberIterator memberValueItr = m_Document->FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsString())
	{
		throw ParserError::MemberValueIsNotSring;
	}

	return memberValueItr->value.GetString();
}

int JSONParser::GetValueInt(const std::string memberValue)
{
	if (!m_Document->HasMember(memberValue.c_str()))
	{
		throw ParserError::MemberValueInvalid;
	}
	Value::MemberIterator memberValueItr = m_Document->FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsInt())
	{
		throw ParserError::MemverValueIsNotInt;
	}

	return memberValueItr->value.GetInt();
}

int JSONParser::GetValueIntForAttribute(const std::string &attributes, const std::string &memberValue)
{
	if (!m_Document->HasMember(attributes.c_str()))
	{
		throw ParserError::AttributeInvalid;
	}

	Value::MemberIterator memberAttributeItr = m_Document->FindMember(attributes.c_str());
	
	if (!memberAttributeItr->value.IsObject())
	{
		throw ParserError::AttributeIsNotObject;
	}

	if (!memberAttributeItr->value.HasMember(memberValue.c_str()))
	{
		throw ParserError::MemberValueInvalid;
	}

	Value::ConstMemberIterator memberValueItr = memberAttributeItr->value.FindMember(memberValue.c_str());

	if (!memberValueItr->value.IsInt())
	{
		throw ParserError::MemverValueIsNotInt;
	}

	return memberValueItr->value.GetInt();
}