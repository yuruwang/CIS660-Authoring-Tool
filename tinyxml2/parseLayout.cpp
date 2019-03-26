#include "parseLayout.h"
#include <algorithm>


tinyxml2::XMLElement* Layout::getElement(tinyxml2::XMLDocument* doc, char* input)
{
	return doc->FirstChildElement(input);
}

tinyxml2::XMLNode* Layout::getMainShape(tinyxml2::XMLDocument* doc)
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
	Efloat xE(xval, 6e-7, Efloat::Normal);
	Efloat yE(yval, 6e-7, Efloat::Normal);
	Efloat zE(yval, 6e-7, Efloat::Normal);
	return EVector(xE, yE, zE);
}
Layout::BoundBox::BoundBox(){}
Layout::BoundBox::BoundBox(EVector minV, EVector maxV)
{
	minVal = minV;
	maxVal = maxV;
	sizeV = maxV - minV;

}
Layout::BoundBox::BoundBox(const tinyxml2::XMLNode* node)
{
	const tinyxml2::XMLNode * pos = node->FirstChildElement("Min");
	minVal = getPosition(pos);
	pos = node->FirstChildElement("Max");
	maxVal = getPosition(pos);
	pos = node->FirstChildElement("Size");
	sizeV = getPosition(pos);
	if (!(sizeV + minVal == maxVal))
	{
		throw std::runtime_error("size not consistent");
	}
}

const EVector&  Layout::BoundBox::min() const
{
		return minVal;
}
const EVector&  Layout::BoundBox::max() const
{
		return maxVal;
}
const EVector&  Layout::BoundBox::size() const
{
	       return sizeV;
}
Layout::Node::Node(){}
// returns a list of Children Nodes ordered according to the splits
// The node passed in is the Top level serializable node
std::vector<std::unique_ptr<Layout::Node>> GetChildren(const std::vector<Efloat>& splits, 
				EVector::Axis ax, const tinyxml2::XMLNode * parent)
{
	
	typedef std::vector<Efloat>::size_type IndxType;
	//if no splits then no children
	IndxType childrenSize = (splits.size())? splits.size() +1: splits.size();
	std::vector<std::unique_ptr<Layout::Node>> children(childrenSize);
	// will be for Serializable Shape
	const tinyxml2::XMLNode * SerShape = parent->FirstChildElement("Children");
	if (SerShape == nullptr) return children;
	SerShape = SerShape ->FirstChild();
	while (SerShape != nullptr)
	{
		std::unique_ptr<Layout::Node> node (new Layout::Node(SerShape));
		// find the minimum value of a box
		Efloat minVal = (ax== EVector::Axis::X) ? node -> bb.min().x: node -> bb.min().y;
		std::vector<Efloat>::const_iterator loc = 
			        std::upper_bound(splits.begin(), splits.end(), minVal);
		IndxType indx =static_cast<IndxType>( loc - splits.begin()); 
		children[indx] = std::move(node);
		SerShape = SerShape ->NextSibling();
	}
	// check if all the children have a node:
	for (const std::unique_ptr<Layout::Node>& oneNode : children)
	{
		if (oneNode == nullptr)
		{
			throw std::runtime_error("At Least one Child Node not Found");
		}
	}
	return children;
}
Layout::Node::Node(const tinyxml2::XMLNode* node):bb(node ->FirstChildElement("BBox"))
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
	children = GetChildren(splits, splitDir, node);
}
