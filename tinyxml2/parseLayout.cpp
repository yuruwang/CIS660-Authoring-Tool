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

Layout::Node::Node(const EVector& sz, const EVector::Axis sd, 
		                std::vector<Efloat>&& ss, 
				std::weak_ptr<Node> p,
                                std::shared_ptr<NodeValue> vd) : v{vd},
					size{sz}, splitDir{sd}, 
					splits{std::move(ss)},
					parent{p}
{}
//Layout::Node::Node(){}
//
//Layout::Node::Node(std::shared_ptr<NodeValue> vv) : v (vv) {} 

Layout::BranchNode::BranchNode(const EVector& sz, const EVector::Axis sd, std::vector<Efloat>&& ss,
					std::weak_ptr<Node> p, std::shared_ptr<NodeValue> v ):
					Node(sz, sd, std::move(ss),  p, v)
{}

//Layout::Node::Node(std::shared_ptr<NodeValue> vv) : v (vv) {} 

Layout::LeafNode::LeafNode(const EVector& sz, const EVector::Axis sd, std::vector<Efloat>&& ss,
					std::weak_ptr<Node>  p,
				std::shared_ptr<NodeValue> v ):
				Node(sz, sd, std::move(ss), p, v)
{}
/************************************************************************************************************
 * @func      addGroupToXYLocMap.
 * @args      XYWidth& map, std::shared_ptr<Node> groupNode
 * @brief     will add an entry to the map or create an entry if need be.
 * ****************************************************************************************************/
 void addGroupToXYLocMap(Layout::XYWidth& map, const std::shared_ptr<Layout::Node> inNode)
{
	Layout::XYWidth::iterator  XYit {map.find(inNode->size.x)};
	// x found
	bool found = XYit != map.end();
	if (!found) {
		 std::pair<Layout::XYWidth::iterator, bool> success{
			 map.insert(std::pair<Efloat, Layout::YWidth>(inNode->size.x, Layout::YWidth()))};
		 if (!success.second) {
			throw std::runtime_error("failed to insert in XYWidth Map");
		 }
		 XYit = success.first;
	}
	// insert the node in the map
	XYit->second.insert(std::make_pair(inNode->size.y, inNode));
}


// returns a list of Children Nodes ordered according to the splits
// The node passed in is the Top level serializable node
std::vector<std::shared_ptr<Layout::Node>> Layout::BottomUp::GetChildren(const std::vector<Efloat>& splits, 
				EVector::Axis ax, const tinyxml2::XMLNode * parent, 
				std::weak_ptr<Node> p, const EVector& minVal, 
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
			children.push_back(XMLNode(std::move(pr), p, childMin, level, namesFound));
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
			children.push_back(XMLNode(std::move(pr), p, childMin, level, namesFound));
			if ( indx < splits.size()) {
				childMin.y = minVal.y + splits[indx++];
			}
		}
	}
	return children;
}
std::shared_ptr<Layout::Node> Layout::BottomUp::XMLNode(Layout::XMLNodePr&& nodePr, std::weak_ptr<Node> p,
		         const EVector&  minVal, int level,  Layout::nameMap& namesFound)
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
	EVector size {nodePr.second->size()};
	if (!(nodePr.second->min() == minVal)) {

		throw std::runtime_error("Box Location Error");
	}
	const tinyxml2::XMLNode* dir = nodePr.first->FirstChildElement("SplitsX");
	EVector::Axis splitDir = static_cast<EVector::Axis>(dir -> NoChildren());
	std::vector<Efloat> splits;
	if (splitDir == EVector::Axis::X)
	{
		dir = nodePr.first ->FirstChildElement("SplitsX");
		splits = parseList(dir, minVal.x);
	}
	else {
		dir = nodePr.first ->FirstChildElement("SplitsY");
		splits = parseList(dir, minVal.y);
	}
	std::shared_ptr<Node> thisNode;
	if ( splits.size() == 0) {
		v -> n = 1; // only one Node contained here
		// update namesFound and if there is a prior value return it
		GroupType  group{ addNodeValue(v, namesFound)};
		// leaf nodes hold the maps of all the groups that have an
		// origin at the lower left corner
		std::shared_ptr<LeafNode> lf = std::make_shared<LeafNode>(std::move(size), splitDir, std::move(splits),
				    p,  v);
		std::list<GroupPair>::iterator it {addToGroupMap(lf, minVal, group)};
		// this adds the node itself as the first group stored in the
		// Lower Left corner.
		addGroupToXYLocMap(lf->LL, lf);
		thisNode = lf;
	}
	else {
		v -> n = 0;
		// add only the terminals in all the children. Get a good count
		// for the number of terminals contained in the node
		//GroupType  group{ addNodeValue(v, namesFound)};
		thisNode = std::make_shared<BranchNode>(std::move(size), splitDir, std::move(splits),
				    p, v);
		//std::list<GroupPair>::iterator it {addToGroupMap(thisNode, minVal, group)}; 
	}
	thisNode ->children = GetChildren(thisNode ->splits, thisNode->splitDir, nodePr.first, thisNode, 
							minVal, level + 1, namesFound);
    for (std::shared_ptr<Node> child: thisNode ->children)
	{
			thisNode -> v-> n += child -> v->n;
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
	equal = equal && a.n == b.n;
	return equal;
}

bool checkNodeValues(const std::shared_ptr<Layout::NodeValue> a, const std::shared_ptr<Layout::NodeValue> b)
{
	return a == b || *a == *b;
}

/****************************************************************************************************
 *          addNodeValue will just add the NodeValue to the nameMap; if
            there is one there already, this returns the current one and
            discards the nodeValue entered.
*   ret:    returns true if the nodeValue is new and nameMap is changed.
*/

Layout::GroupType Layout::BottomUp::addNodeValue(std::shared_ptr<NodeValue>& nodeValue, nameMap& nm)
{
	nameMap::iterator itName = nm.find( nodeValue->name );
	GroupType group {GroupType::New};
	// not found 
	if (itName == nm.end())
	{
		// the one and only time next is used
		nodeValue->uid = next++;
		std::pair<Layout::nameMap::iterator, bool> insertName =
			  nm.insert(std::make_pair(nodeValue->name, nodeValue->uid));
		if (!insertName.second){
				throw std::runtime_error("Failed to insert in Name Map");
			}

	}
	else // Name is found already so check if the Node already exists and is the same
	{
		group = GroupType::Existing;
		// find name in Groups
		GroupMap::iterator itGroup  {groups.find(itName -> second)};
		if (itGroup == groups.end())
		{
			throw std::runtime_error("Group corresponding to name not found.");
		}
		std::list<GroupPair>::iterator it = itGroup->second.begin();
		if (it == itGroup->second.end()) {
			throw std::runtime_error("No groups found with Name");
		}
		// check node values
		nodeValue->uid = it->first->v->uid;
		for (; it!=itGroup->second.end(); ++it)
		{
			if (!checkNodeValues(it->first->v, nodeValue)) {
				throw std::runtime_error("Node Values not all identical.");
			}
		}
		// update the nodeValue
		it = itGroup->second.begin();
	        nodeValue = it->first->v;
	}
	return group;
}


/***************************************************************************************************************
 *              addNodeTo GroupMap; This adds a new Node to the group Map.  It
 *              returns an iterator to the list element that holds the node.
 **************************************************************************************************************/
std::list<Layout::GroupPair>::iterator Layout::BottomUp::addToGroupMap(
	std::shared_ptr<Layout::Node> node, const EVector& minLocation, Layout::GroupType grouptype)
{
	uIDType uid =  node -> v-> uid;
	GroupMap::iterator itGroup  {groups.find(uid)};
	bool found = itGroup != groups.end();
	// Node should not be in the group;
	GroupPair pr {std::make_pair(node, minLocation)};
	if (found)
	{
		itGroup ->second.push_back(std::move(pr));
		if (grouptype == GroupType::New) {
			throw std::runtime_error("Goup not in Name but in Group");
		}
	}
	// not found new group;
	else {
		std::pair<GroupMap::iterator, bool> success = groups.insert(
				      std::make_pair(uid,std::list<GroupPair>{std::move(pr)}));
		if (!success.second) {
			throw std::runtime_error("failed To add to Group map");
		}
		itGroup= success.first;
		if (grouptype == GroupType::Existing){
			throw std::runtime_error("Group in Name map but not in group map");
		}
	}
	return itGroup->second.begin();
}
/*************************************************************************************************************
 * constructor
 * **********************************************************************************************************/

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
			throw std::runtime_error("Doc did not read correctly");
		}

		EVector leftCorner(0, 0, 0);
		location = XMLNode(std::move(pr), std::weak_ptr<Node>(), leftCorner, 0, names);
}

/*************************************************************************************************************
 * @func  	removeSingles  removes groups that are repeated only once
 * @params 	[start, last),   First(inclusive), Last[exclusive] ID to search from
 * ************************************************************************************************************/
void Layout::BottomUp::removeSingles(uIDType current, uIDType last)
{
	for (; current < last; ++current)
	{
		GroupMap::iterator itGroup  {groups.find(current)};
		bool found = itGroup != groups.end();
		// Node should not be in the group;
		if (found && itGroup -> second.size() > 0) {
		    ListIterator itList = itGroup -> second.begin();
		    nameMap::iterator  nameit = names.find(itList -> first-> v -> name);
		    if (nameit == names.end())
		    {
			    throw std::runtime_error("Name corresponding to UID not found");
		    }
		    names.erase(nameit);
		    groups.erase(itGroup);
		}
	}
}


// true if   ll <= d (desired) < ll + size, false otherwise
bool withinBox (const EVector& ll, const EVector& size, const EVector& d)
{
	bool  within { ll.x <= d.x && d.x < ll.x + size.x};
	within = within && (ll.y <= d.y && d.y < ll.y + size.y);
	within = within && (ll.z <= d.z && d.z < ll.z + size.z);
	return within;
}
// provide a child and an absolute LL coordinate, and this finds the lower left
// coordinate of the parent.
EVector   parentLLCorner(std::shared_ptr<const  Layout::Node> parent, std::shared_ptr<const Layout::Node> child, 
				EVector& minValueChild)
{
	std::vector<std::shared_ptr<Layout::Node>>::const_iterator it = find(parent->children.begin(), 
						parent->children.end(), child);
	if ( it == parent->children.end() )
	{
		throw std::runtime_error("child not found");
	}
	std::ptrdiff_t diff = it - parent->children.begin();
	if (diff == 0)
	{
		return minValueChild;
	}
	if (parent -> splitDir == EVector::Axis::X)
	{
		minValueChild.x -= parent->splits[diff -1];
	}
	else{
		minValueChild.y -= parent->splits[diff -1];
	}
	return minValueChild;
}
	

/**************************************************************************************************
 * @func    std::shared_ptr<Node> findLLNode(std::shared_ptr<Node> init, Evector& lowerLeft)
 * @params[in]    std::shared_ptr< Node> curr: start Node should be a node
 * 			in the spatial tree with a valid parent. Should not be
 * 			a combined node in the groups.
 *                EVector& ll init location of that node in the
 *                		spatial structure this will change for each node
 * 		  EVector&   term the lower left corner that one wants to get to
 * @params[out]   std::shared_ptr< const Node>  the primitive node with this coordinate or null if no
 *                        such pointer exists
 * @brief          will look through the tree starting at the GroupPair, searching up the tree
 * 		   or down the tree and return the node that has the lowerLeft corner at term
 ************************************************************************************************/
std::shared_ptr<const Layout::Node> Layout::findLLNode(std::shared_ptr<const Layout::Node> curr, EVector& ll, const EVector& term)
{
	if (curr == nullptr) {
		throw std::runtime_error("initial Node is null\n");
	}
	bool within { withinBox(ll, curr->size, term)};
	// outside of bounding box check parent
	if (!within)
	{
		if (!curr-> parent.expired())
		{
			// get the lower left corner of parent
			std::shared_ptr<const Layout::Node> pp = curr->parent.lock();
			ll = parentLLCorner(pp, curr, ll);
			return findLLNode(pp, ll, term);
		}
		else {
			return nullptr;
		}
	}
	//terminal case where either this location is the lower left or is not	
	if (  curr->v->terminal()) {
		return (term == ll)? curr: nullptr;
	}
	// within a child find the child
	
	EVector::Axis ax = curr->splitDir;
	Efloat minVal { ( ax == EVector::Axis::X)? term.x - ll.x: term.y - ll.y};
	// find the last value that minVal could be inserted and is greater than relativeMin. 
	// This is the first slit that is greater than the value  and is the correct
	// function.
	std::vector<Efloat>::const_iterator it {std::upper_bound( curr->splits.begin(), 
			 	curr->splits.end(), minVal)};
	int indx { static_cast<int>(it - curr->splits.begin())};
	// update the lower bound of the new box
	if (indx != 0)
	{
		if ( ax == EVector::Axis::X) {
			ll.x += curr->splits[indx - 1];
		}
		else {
			ll.y += curr->splits[indx -1];
		}
	}
	// because children have one more element than splits, the indx
	// is correct for the children vector
	std::shared_ptr<const Layout::Node> child {curr->children[indx]};
	return findLLNode( child, ll, term);
}
	

// add nonterminal groups of size n to groupMap
void Layout::BottomUp::addNTGroups(size_t n)
{
	UIDType first {next};
	for (uIDType u = 0; u < first; u++)
	{
		GroupMap::iterator it {groups.find(u)};
		if (it == groups.end())
		{
				continue;
		}
		for ( GroupPair& pr : it->second)
		{
			EVector neighborLoc { pr.second}

			std::shared_ptr<Node> thisCorner { findLLNode(pr.first, pr.second, pr.second)};
			neighborLoc.x +=pr.first.size.x;
			std::shared_ptr<Node> neighbor  {findLLNode(pr.first, pr.second, neighborLoc)};
			if (neighbor != nullptr)
				std::cout <<"neighborFound" << std::endl;
			else
				std::cout << "No neighbor" << std::endl;

		}
	}
}










