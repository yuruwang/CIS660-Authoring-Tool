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
std::vector<Efloat> parseList(const tinyxml2::XMLNode * node, Efloat min)
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
		vals.push_back(Efloat(x, 6e-7f, Efloat::Normal) - min);
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
	Efloat xE(xval, 6e-7f, Efloat::Normal);
	Efloat yE(yval, 6e-7f, Efloat::Normal);
	Efloat zE(zval, 6e-7f, Efloat::Normal);
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
bool Layout::NodeValue::terminal() const
{
	return n == 1;
}
bool Layout::Node::terminal() const
{	
	return v -> terminal();
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
std::vector<std::shared_ptr<Layout::Node>> GetChildren(const std::vector<Efloat>& splits, 
				EVector::Axis ax, const tinyxml2::XMLNode * parent, const EVector& minVal, 
				int level, nameMap namesFound)
{
	std::vector<std::shared_ptr<Layout::Node>> children;
	// will be for Serializable Shape
	const tinyxml2::XMLNode * SerShape = parent->FirstChildElement("Children");
	if (SerShape == nullptr) return children;
	SerShape = SerShape ->FirstChild();
	// get all the xml nodes and Bounding Boxes
	std::vector<Layout::XMLNodePr> AllXMLNodes;
	while (SerShape != nullptr)
	{
		AllXMLNodes.push_back( Layout::XMLNodePr(SerShape, 
			std::unique_ptr<Layout::BoundBox>( 
				new Layout::BoundBox(SerShape->FirstChildElement("BBox")))));
		SerShape =SerShape ->NextSibling();
	}
	bool validLength = (splits.size() == 0 && AllXMLNodes.size() == 0) ||
		            (splits.size() != 0  && AllXMLNodes.size() == splits.size() + 1);
	if ( !validLength) 
	{
		throw std::runtime_error("Number of Nodes Not valid");
	}
	// sort according to split axis
	if ( ax == EVector::Axis::X)
	{
		std::sort(AllXMLNodes.begin(), AllXMLNodes.end(), 
				[] (const Layout::XMLNodePr& a, const Layout::XMLNodePr& b) -> bool {
				   return a.second->min().x < b.second->min().x;});
		EVector childMin = minVal;
		std::vector<Efloat>::size_type indx = 0;
		for (  Layout::XMLNodePr& pr : AllXMLNodes)
		{ 
			children.push_back(XMLNode(std::move(pr), childMin, level, namesMap));
			if ( indx < splits.size()) {
				childMin.x = minVal.x + splits[indx++];
			}
		}

	}
	else { 
		std::sort(AllXMLNodes.begin(), AllXMLNodes.end(), 
				[] (Layout::XMLNodePr& a, 
					Layout::XMLNodePr& b) -> bool {
					//const Efloat ay = a.second.min().y;
					//const Efloat by = b.second.min().y;
				   return a.second->min().y < b.second->min().y;}); 
		EVector childMin = minVal;
		std::vector<Efloat>::size_type indx = 0;
		for ( Layout::XMLNodePr& pr : AllXMLNodes)
		{ 
			children.push_back(XMLNode(std::move(pr), childMin, level, namesFound));
			if ( indx < splits.size()) {
				childMin.y = minVal.y + splits[indx++];
			}
		}
	}
	return children;
}
std::shared_ptr<Node> Layout::BottomUp::XMLNode(Layout::XMLNodePr&& nodePr, const EVector&  minVal, int Level, 
		           nameMap namesFound)
{
	const tinyxml2::XMLElement* elem = nodePr.first->FirstChildElement("Level");
	////// params in file but not used:
	int lvl;
	std::shared_ptr<NodeValue> v = std::make_shared<NodeValue>();
	tinyxml2::XMLError err = elem ->QueryIntText(&lvl);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Level Conversion");
	}
	if (lvl != Level) 
	{
		throw std::runtime_error("level is not expected");
	}
	elem = nodePr.first->FirstChildElement("UId");
	int u;
	err = elem ->QueryIntText(&u);
	v->uid = static_cast<unsigned>(u);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Uid Conversion");
	}
	elem = nodePr.first->FirstChildElement("Isolated");
	int val;
	err = elem ->QueryIntText(&val);
	bool iso = static_cast<bool>(val);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Isolation Conversion");
	}
	elem = nodePr.first->FirstChildElement("Label")->
	             FirstChildElement("LabelName");
	v->name = elem ->GetText();
	// There are two estimates of location.  One is gotten through all the
	// splits recursively down the tree.  The other is from the boundBox.
	// This comparison tests if the two are the same.
	v ->size = nodePr.second->size();
	if (!(nodePr.second->min() == minVal)) {

		throw std::runtime_error("Box Location Error");
	}
	const tinyxml2::XMLNode* dir = nodePr.first->FirstChildElement("SplitsX");
	EVector::Axis splitDir = static_cast<EVector::Axis>(dir -> NoChildren());
	std::vector<Efloat> splits;
	std::vector<std::shared_ptr<Node>> children;
	if (splitDir == EVector::Axis::X)
	{
		dir = nodePr.first ->FirstChildElement("SplitsX");
		splits = parseList(dir, minVal.x);
	}
	else {
		dir = nodePr.first ->FirstChildElement("SplitsY");
		splits = parseList(dir, minVal.y);
	}
	children = GetChildren(splits, splitDir, nodePr.first, minVal, level + 1, namesFound);
	std::shared_ptr<Node> thisNode;
	if ( children.size() == 0) {
		v -> n = 1; // only one Node contained here
		if (!NodePr.first->NoChildren()){
			std::runtime_error("There should be no children, but children found.");
		}
		thisNode = std::make_shared<Node>();
	}
	else {
		v -> n = 0;
		// add only the terminals in all the children. Get a good count
		// for the number of terminals contained in the node
		for (std::shared_ptr<Node> child: children)
		{
			v-> n += child -> v->n;
		}
		if (NodePr.first->NoChildren() || v -> n == 0)
		{
			std::runtime_error("Children report terminals, but none found");
		}
		thisNode = std::make_shared<BranchNode>();
	}
	thisNode ->splits = std::move(splits);
	thisNode ->splitDir = splitDir;
	thisNode ->children = std::move(children);
	NameMap::iterator it = namesFound(v->name);
	if (it == namesFound.end())
	{
	/// add all the nodevalue to all the maps 
	}
	else {
	       
	}
}
bool checkNodeValues(const std::make_shared<NodeValue> a, const std::make_shared<NodeValue> b)
{
	return a == b || *a == *b;
}
Layout::GroupMap::iterator 
   BottomUp::addTerminalToGroups(EVector location, std::shared_ptr<NodeValue> v, 
				     nameMap& nm)
{
	NameMap::iterator itName = nm.find( v->name );
	GroupMap::iterator itGroup;
	// not found 
	if (itName == nm.end())
	{
		std::pair<nameMap::iterator, bool> insertName =
			  nm.insert(std::make_pair(v->name, next));
		{
			if (!insertName.second){
				throw std::runtime_error("Failed to insert in Name Map");
			}
		}
		std::list<EVector> thisList;
		thisList.push_back(location);
		std::shared_ptr<Node> node = std::make_shared<Node>();
		node->v = v;
		GroupPair pr = std::make_pair(std::move(node), std::move(thisList));
		std::pair<GroupMap::iterator, bool> insertRes;
		insertRes = groups.insert(std::make_pair(next++, std::move(pr)));
		if (insertRes.second) {
			itGroup = insertRes.first;
		}
		else{
			std::runtime_error("Failed to insert in the Group map");
		}

	}
	else // Name is found already so check if the Node already exists and is the same
	{
		itGroup  = groups.find(it.name -> second);
		if (itGroup == groups.end())
		{
			std::runtime_error("Group corresponding to name not found.");
		// check if it is the same;
		std::shared_ptr<NodeValue> v { itGroup -> second ->first -> v};
	}
}

Layout::BottomUp( const char * filename): next{0}
{
		XMLDocument doc;
		doc->LoadFile( filename);
		tinyxml2::XMLNode *  node = Layout::getMainShape(&doc);
		Layout::XMLNodePr pr(node,
			std::unique_ptr<Layout::BoundBox>(
				new Layout::BoundBox(node->FirstChildElement("BBox"))));
		EVector leftCorner(0, 0, 0);
		nameMap termNames;
		location = XMLNode(std::move(pr2), leftCorner, 0, termNames);
}
/***********************************************************
	while (SerShape != nullptr)
	{
		std::shared_ptr<Layout::Node> node Layout::Node(SerShape, minVal));
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
*******************************/
