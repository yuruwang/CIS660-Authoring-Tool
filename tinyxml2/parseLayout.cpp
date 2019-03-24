#include "parseLayout.h"
#include <algorithm>

Position::Position(float x, float y, float z)
{
	xval = x;
	yval = y;
	zval = z;
}
Efloat  Position::x() const
{
	return xval;
}
tinyxml2::XMLElement* getElement(tinyxml2::XMLDocument* doc, char* input)
{
	return doc->FirstChildElement(input);
}

tinyxml2::XMLNode* getMainShape(tinyxml2::XMLDocument* doc)
{
	return doc -> FirstChildElement("SerializableFacade") ->
	FirstChildElement("MainShape");
}
/*  Parses a list of siblings as floats*/
std::vector<Efloat> parseList(const tinyxml2::XMLNode * node)
{
	const tinyxml2::XMLNode * val = node ->FirstChild();
	std::vector<Efloat> vals;
	while ( val != nullptr)
	{
		float x;
		const tinyxml2::XMLElement * elem = val ->ToElement();
		if (elem == nullptr) break;
		tinyxml2::XMLError err = elem ->QueryFloatText(&x);
		if (err != tinyxml2::XML_SUCCESS) break;
		vals.push_back(x);
		val = val -> NextSibling();
	}
	std::sort(vals.begin(), vals.end());
	return vals;
}

EVector getPosition(const tinyxml2::XMLNode * node)
{
	float xval;
	float yval;
	float zval;
	const tinyxml2::XMLElement* elem = node->FirstChildElement("X");
	tinyxml2::XMLError err = elem ->QueryFloatText(&xval);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed X Conversion");
	}
	elem = node->FirstChildElement("Y");
	err = elem->QueryFloatText(&yval);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Y Conversion");
	}
	elem = node->FirstChildElement("Z");
	err = elem->QueryFloatText(&zval);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Z Conversion");
	}
	return EVector(xval, yval, zval);
}
BoundBox::BoundBox(EVector minV, EVector maxV)
{
	minVal = minV;
	maxVal = maxV;
	size = maxV - minV;

}
BoundBox::BoundBox(const tinyxml2::XMLNode* node)
{
	const tinyxml2::XMLNode * pos = node->FirstChildElement("Min");
	minVal = getPosition(pos);
	pos = node->FirstChildElement("Max");
	maxVal = getPosition(pos);
	pos = node->FirstChildElement("Size");
	size = getPosition(pos);
	if (!(size + minVal == maxVal))
	{
		throw std::runtime_error("size not consistent");
	}
}


Node::Node(tinyxml2::XMLNode* node):bb(node ->FirstChildElement("BBox"))
{

	const tinyxml2::XMLElement* elem = node->FirstChildElement("Level");
	tinyxml2::XMLError err = elem ->QueryIntText(&lvl);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Level Conversion");
	}
	elem = node->FirstChildElement("UId");
	err = elem ->QueryIntText(&uid);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Uid Conversion");
	}
	elem = node->FirstChildElement("Isolated");
	int val;
	err = elem ->QueryIntText(&val);
	iso = static_cast<bool>(val);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Isolation Conversion");
	}
	elem = node->FirstChildElement("Label")->
	             FirstChildElement("LabelName");
	name = elem ->GetText();
	const tinyxml2::XMLNode* dir = node->FirstChildElement("SplitsX");
	splitDir = static_cast<EVector::Axis>(dir ->NoChildren());
	const tinyxml2::XMLNode* nodeChild =
	        node->FirstChildElement("Children");
	ft = static_cast<FacadeType>(nodeChild -> NoChildren());
	if (splitDir == EVector::Axis::X)
	{
		dir = node ->FirstChildElement("SplitsX");
	}
	else {
		dir = node ->FirstChildElement("SplitsY");
	}
	splits = parseList(dir);

}

