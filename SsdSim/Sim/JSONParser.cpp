#include "JSONParser.h"

#include <fstream>

using namespace rapidjson;

JSONParser::JSONParser(const std::string& name) : sJsonName(name)
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

bool JSONParser::GetValueBool(std::string sElement)
{
	Value::MemberIterator memberValue = m_Document->FindMember(sElement.c_str());

	if (memberValue == m_Document->MemberEnd())
	{
		throw ParserError::ElementInvalid;
	}
	if (!memberValue->value.IsBool())
	{
		throw ParserError::BoolInvalid;
	}

	return memberValue->value.GetBool();
}

const char* JSONParser::GetValueString(std::string sElement)
{
	Value::MemberIterator memberValue = m_Document->FindMember(sElement.c_str());

	if (memberValue == m_Document->MemberEnd())
	{
		throw ParserError::ElementInvalid;
	}
	if (!memberValue->value.IsString())
	{
		throw ParserError::StringInvalid;
	}

	return memberValue->value.GetString();
}

int JSONParser::GetValueInt(std::string sElement)
{
	Value::MemberIterator memberValue = m_Document->FindMember(sElement.c_str());

	if (memberValue == m_Document->MemberEnd())
	{
		throw ParserError::ElementInvalid;
	}
	if (!memberValue->value.IsInt())
	{
		throw ParserError::IntInvalid;
	}

	return memberValue->value.GetInt();
}
