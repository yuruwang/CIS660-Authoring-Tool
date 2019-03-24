#pragma
#include "tinyxml2.h"
#include "efloat.h"

class  Position
{
public:
	Position(float x, float y, float z);
	Efloat x() const;
private:
	Efloat xval;
	float yval;
	float zval;
};

class BoundBox
{

};

tinyxml2::XMLElement* getElement(tinyxml2::XMLDocument*, char* input);

tinyxml2::XMLElement* getMainShape(tinyxml2::XMLDocument* doc);
