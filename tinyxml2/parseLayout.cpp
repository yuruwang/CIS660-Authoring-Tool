#include "parseLayout.h"

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

tinyxml2::XMLElement* getMainShape(tinyxml2::XMLDocument* doc)
{
	return doc -> FirstChildElement("SerializableFacade") ->
	FirstChildElement("MainShape");
}
