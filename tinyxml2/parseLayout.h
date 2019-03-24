#include "tinyxml2.h"
#include "efloat.h"

class  Position
{
public:
	Position(float x, float y, float z);
	float x() const;
private:
	Efloat xval;
	float yval;
	float zval;
};

class BoundBox
{

};


