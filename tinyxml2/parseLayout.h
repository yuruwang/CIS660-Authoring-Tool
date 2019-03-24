#pragma
#include "tinyxml2.h"
#include "efloat.h"
#include <string.h>
#include <vector>

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
/*        Holds the bounding box of dimensions that a Node has  minVal is the min, maxVal the max
and Size the difference.*/
class BoundBox
{
public:
	BoundBox(EVector minV, EVector maxV);
	BoundBox(const tinyxml2::XMLNode* node);
private:
	EVector minVal;
	EVector maxVal;
	EVector size;

};
// holds a square Facade element.
enum FacadeType {NT, T};
struct Node {
	Node(tinyxml2::XMLNode* node);
	int lvl;  // level in the binary tree
	bool iso; // not sure what this describes
	BoundBox bb;  // bound box
	int uid;  // unique id of the node
	std::string name;  //name of the node
	FacadeType ft; // term or non terminal
	EVector::Axis splitDir; // split along x or y
	std::vector<Efloat> splits;// the location of the splits
	std::vector<Node>  children;
};


tinyxml2::XMLElement* getElement(tinyxml2::XMLDocument*, char* input);

tinyxml2::XMLNode* getMainShape(tinyxml2::XMLDocument* doc);
