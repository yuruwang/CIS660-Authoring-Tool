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

Layout::Node::Node(EVector::Axis sd, std::vector<Efloat>&& ss, std::vector<std::shared_ptr<Node>>&& cn, 
				std::shared_ptr<NodeValue> vd ): splitDir{sd}, splits{std::move(ss)},
	                        children{ std::move(cn)}, v{vd} 
{}
Layout::Node::Node(){}

Layout::Node::Node(std::shared_ptr<NodeValue> vv) : v (vv) {} 

Layout::BranchNode::BranchNode(EVector::Axis sd, std::vector<Efloat>&& ss,
					std::vector<std::shared_ptr<Node>>&& cn, 
				std::shared_ptr<NodeValue> v ):Node(sd, std::move(ss), std::move(cn), v)
{}
// returns a list of Children Nodes ordered according to the splits
// The node passed in is the Top level serializable node
std::vector<std::shared_ptr<Layout::Node>> Layout::BottomUp::GetChildren(const std::vector<Efloat>& splits, 
				EVector::Axis ax, const tinyxml2::XMLNode * parent, const EVector& minVal, 
				int level, Layout::nameMap& namesFound)
{
	std::vector<std::shared_ptr<Node>> children;
	// will be for Serializable Shape
	const tinyxml2::XMLNode * SerShape = parent->FirstChildElement("Children");
	if (SerShape == nullptr) return children;
	SerShape = SerShape ->FirstChild();
	// get all the xml nodes and Bounding Boxes
	std::vector<XMLNodePr> AllXMLNodes;
	while (SerShape != nullptr)
	{
		AllXMLNodes.push_back( XMLNodePr(SerShape, 
			std::unique_ptr<BoundBox>( 
				new BoundBox(SerShape->FirstChildElement("BBox")))));
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
				[] (const XMLNodePr& a, const XMLNodePr& b) -> bool {
				   return a.second->min().x < b.second->min().x;});
		EVector childMin = minVal;
		std::vector<Efloat>::size_type indx = 0;
		for (  XMLNodePr& pr : AllXMLNodes)
		{ 
			children.push_back(XMLNode(std::move(pr), childMin, level, namesFound));
			if ( indx < splits.size()) {
				childMin.x = minVal.x + splits[indx++];
			}
		}

	}
	else { 
		std::sort(AllXMLNodes.begin(), AllXMLNodes.end(), 
				[] (XMLNodePr& a, 
					XMLNodePr& b) -> bool {
				   return a.second->min().y < b.second->min().y;}); 
		EVector childMin = minVal;
		std::vector<Efloat>::size_type indx = 0;
		for ( XMLNodePr& pr : AllXMLNodes)
		{ 
			children.push_back(XMLNode(std::move(pr), childMin, level, namesFound));
			if ( indx < splits.size()) {
				childMin.y = minVal.y + splits[indx++];
			}
		}
	}
	return children;
}
std::shared_ptr<Layout::Node> Layout::BottomUp::XMLNode(Layout::XMLNodePr&& nodePr, const EVector&  minVal, int level, 
		           Layout::nameMap& namesFound)
{
	const tinyxml2::XMLElement* elem = nodePr.first->FirstChildElement("Level");
	////// params in file but not used:
	const tinyxml2::XMLNode * fchild = nodePr.first->FirstChild();
	int lvl;
	std::shared_ptr<NodeValue> v = std::make_shared<NodeValue>();
	tinyxml2::XMLError err = elem ->QueryIntText(&lvl);
	if (err != tinyxml2::XML_SUCCESS)
	{
		throw std::runtime_error("failed Level Conversion");
	}
	if (lvl != level) 
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
		GroupMap::iterator it = addTerminalToGroups(minVal, v, namesFound);
		thisNode = it->second.first;
	}
	else {
		v -> n = 0;
		// add only the terminals in all the children. Get a good count
		// for the number of terminals contained in the node
		for (std::shared_ptr<Node> child: children)
		{
			v-> n += child -> v->n;
		}
		thisNode = std::make_shared<BranchNode>(splitDir, std::move(splits), std::move(children), v);
	}
	return thisNode;
}
bool checkUnique_location( EVector newvalue, std::list<EVector>& list)
{
	for ( EVector& val : list)
	{
		if (val == newvalue)
		{
			return false;
		}
	}
	return true;
}
bool Layout::operator==(const Layout::NodeValue& a, const Layout::NodeValue& b)
{
	bool equal {a.uid == b.uid};
	equal = equal && a.name == b.name;
	equal = equal && a.size == b.size;
	equal = equal && a.n == b.n;
	return equal;
}

bool checkNodeValues(const std::shared_ptr<Layout::NodeValue> a, const std::shared_ptr<Layout::NodeValue> b)
{
	return a == b || *a == *b;
}
Layout::GroupMap::iterator 
	   Layout::BottomUp::addTerminalToGroups(const EVector& location, std::shared_ptr<Layout::NodeValue> v, 
				     Layout::nameMap& nm)
{
	nameMap::iterator itName = nm.find( v->name );
	GroupMap::iterator itGroup;
	// not found 
	if (itName == nm.end())
	{
		std::pair<Layout::nameMap::iterator, bool> insertName =
			  nm.insert(std::make_pair(v->name, next));
		if (!insertName.second){
				throw std::runtime_error("Failed to insert in Name Map");
			}
		std::list<EVector> thisList;
		thisList.push_back(location);
		v->uid = next;
		GroupPair pr = std::make_pair(std::make_shared<Node>(v), std::move(thisList));
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
		// find name in Groups
		itGroup  = groups.find(itName -> second);
		if (itGroup == groups.end())
		{
			std::runtime_error("Group corresponding to name not found.");
		}
		//check if location is unique and add it
		std::list<EVector>& locs = itGroup->second.second;
		if ( !checkUnique_location(location, locs))
		{
		    std::runtime_error("Location found prior");
		}
		locs.push_back(location);
		// check value if the same
		std::shared_ptr<Node> savedG = itGroup -> second.first;
		v->uid = savedG->v->uid;
		if (!checkNodeValues( v, savedG->v))
		{
			std::runtime_error("new and Saved terminals do not match");
		}
	}
	return itGroup;
}

Layout::BottomUp::BottomUp( const char * filename): next{0}
{
		tinyxml2::XMLDocument doc;
		doc.LoadFile( filename);
		tinyxml2::XMLNode *  node = Layout::getMainShape(&doc);
		Layout::XMLNodePr pr(node,
			std::unique_ptr<Layout::BoundBox>(
				new Layout::BoundBox(node->FirstChildElement("BBox"))));

		int errorID = doc.ErrorID();
		if ( errorID != tinyxml2::XML_SUCCESS ) {
			std::runtime_error("Doc did not read correctly");
		}

		EVector leftCorner(0, 0, 0);
		nameMap termNames;
		location = XMLNode(std::move(pr), leftCorner, 0, termNames);
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
