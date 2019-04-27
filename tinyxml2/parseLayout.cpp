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
				std::weak_ptr<const Node> p,
                                std::shared_ptr<NodeValue> vd) : v{vd},
					size{sz}, splitDir{sd}, 
					splits{std::move(ss)},
					parent{p}
{}

Layout::BranchNode::BranchNode(const EVector& sz, const EVector::Axis sd, std::vector<Efloat>&& ss,
					std::weak_ptr<const Node> p, std::shared_ptr<NodeValue> v ):
					Node(sz, sd, std::move(ss),  p, v)
{}
typedef   std::pair<Efloat, Efloat> minMaxPr;

// strictly Overlap is true only if a and b share common points that are not on
// the boundary
bool  strictlyOverlap( const minMaxPr a, const minMaxPr b)
{
	bool overlaps {a.first < b.second &&  b.first < a.second};
	overlaps = overlaps || (b.first < a.second && a.first < b.second);
	return overlaps;
}

// strictly Overlap is true only if b falls within a and is not on
// the boundary
bool  strictlyOverlap( const minMaxPr a, const Efloat b)
{
	bool overlaps {a.first < b &&  b < a.second};
	return overlaps;
}
// containedWithin  is true if b is contained within a. They can share a
// boundary.
bool  containedWithin(const minMaxPr a, const minMaxPr b)
{
	bool overlaps {a.first <= b.first &&  b.second <= a.second};
	return overlaps;
}
/* LineIntersects tests if a splitLine within a spatialLocation Node splits a
   group. The group has a non zero width and a height. It also tests whether a childNode is overlapping with the group,
   where the group is within the location  and wether there are any overlaps at all*/
 
class LineIntersects {
	private:
		// minMap x, y dimensions of spatial Node bounding box
		const minMaxPr xLocPr, yLocPr;
		// axis of GroupNode
		const EVector::Axis splitDir;
		// the size of the group  Node
		// group lowerLeft;
		const bool expired;
		// minMax pairs of the group Node
		const minMaxPr xGroup, yGroup, currentGroup;
	public:
		// initialize all variables
		LineIntersects(Layout::GroupPair loc, Layout::WeakPair group):
			 xLocPr { minMaxPr(loc.second.x, loc.second.x + loc.first -> size.x)}, 
			 yLocPr { minMaxPr(loc.second.y, loc.second.y + loc.first -> size.y)},
			 splitDir { loc.first -> splitDir},
			 expired { group.first.expired()},
			 xGroup { (expired)? minMaxPr(Efloat(), Efloat()) : 
		           minMaxPr(group.second.x, group.second.x + group.first.lock() -> size.x)},
			 yGroup { (expired)? minMaxPr(Efloat(), Efloat()) : 
		           minMaxPr(group.second.y, group.second.y + group.first.lock() -> size.y)},
		         currentGroup { (splitDir == EVector::Axis::X) ?  xGroup:yGroup}
			{}
		// Given the size of the spatial Location Node, will determine
		// in any lines within this node could possible be splitlines
		bool anyOverlaps() const
		{
		    bool Overlaps {strictlyOverlap(xLocPr, xGroup)};
		    Overlaps = Overlaps && strictlyOverlap(yLocPr, yGroup);
		    return Overlaps;
		}
		// provide the size of the location and this will return true
		// if the group is contained within location.  If it not within
		// the location then one needs to look to the parent
		bool groupWithinLocation() const 
		{
		    bool Overlaps {containedWithin(xLocPr, xGroup)};
		    Overlaps = Overlaps && containedWithin(yLocPr, yGroup);
		    return Overlaps;
		}
		// used to get whether a child node
		bool operator()(const minMaxPr pr) const 
		{
			return strictlyOverlap(pr, currentGroup);
		}
		bool operator()(const Efloat& splitLine) const
		{
			if (splitDir == EVector::Axis::X)
			{
				return strictlyOverlap(xGroup, xLocPr.first + splitLine);
			}
			else{
				return strictlyOverlap(yGroup, yLocPr.first + splitLine);
			}
		}
};
// LineSegment encodes a line segment that would be part of a split line.
// An important observation is that the splitlines that divide groups are never
// on the bounding box, they are always 
struct LineSegment {
	minMaxPr  pr; // min and max values of the line say ymin, ymax.
	Efloat transverseVal; // the one transverse value say x.
	EVector::Axis  ax;  // the direction of min, max, say Y
};
/* LineOverlapsLine tests if a splitLVine is within a spatialLocation 
   group. The group has a non zero width and a height. It also tests whether a childNode is overlapping with the group,
   where the group is within the location  and wether there are any overlaps at all*/
 
class LineOverlapsLine {
	private:
		// minMap x, y dimensions of spatial Node bounding box
		const minMaxPr xLocPr;
		const minMaxPr yLocPr;
		// axis of GroupNode
		const EVector::Axis splitDir;
		// the size of the group  Node
		// group lowerLeft;
		// minMax pairs of the group Node
		const LineSegment  line;  // the line that encodes
	public:
		// initialize all variables
		LineOverlapsLine (Layout::GroupPair loc,  const LineSegment ls):
			 xLocPr { minMaxPr(loc.second.x, loc.second.x + loc.first -> size.x)}, 
			 yLocPr { minMaxPr(loc.second.y, loc.second.y + loc.first -> size.y)},
			 splitDir { loc.first -> splitDir},
			 line  { ls}{}
		// Given the size of the spatial Location Node, will determine
		// in any lines within this node could possible be splitlines
		bool anyOverlaps() const
		{
		    bool Overlaps;
		    if (line.ax == EVector::Axis::X)
		    { 
			    Overlaps = strictlyOverlap( yLocPr, line.transverseVal);
			    Overlaps = Overlaps && strictlyOverlap(xLocPr, xLocPr);
		    }
		    else
		    { 
			    Overlaps = strictlyOverlap( xLocPr, line.pr);
			    Overlaps = Overlaps && strictlyOverlap(yLocPr, line.pr);
		    }
		    return Overlaps;
		}
		// provide the size of the location and this will return true
		// if the group is contained within location.  If it not within
		// the location then one needs to look to the parent
		bool groupWithinLocation() const 
		{
		    bool Overlaps;
		    if (line.ax == EVector::Axis::X)
		    { 
			    Overlaps = strictlyOverlap( yLocPr, line.transverseVal);
			    Overlaps = Overlaps && containedWithin(xLocPr, line.pr);
		    }
		    else
		    { 
			    Overlaps = strictlyOverlap( xLocPr, line.transverseVal);
			    Overlaps = Overlaps && containedWithin(yLocPr, line.pr);
		    }
		    return Overlaps;
		}
		// supply a child min max range for the bounding box in the
		// splitDir true means that should could contain an Overlap line
		bool operator()(const minMaxPr pr) const 
		{
			return strictlyOverlap(pr, line.transverseVal);
		}
		// true if the splitline overlaps the stored line
		bool operator()(const Efloat& splitLine) const
		{
			if (splitDir == EVector::Axis::X)
			{
				return  xLocPr.first + splitLine == line.transverseVal;
			}
			else{
				return yLocPr.first + splitLine ==  line.transverseVal;
			}
		}
};
/******************************************************************************************************
 *  @func    findOverlappingSplits will find within a Node of the spatial structure
 *  			tree, which Splits may overlap with a new Group;
 *  @params[in]     GroupPair locGroup -Node and location in the spatial
 *  			 in the spatial structure of where to look.
 *  		    LineIntersects.  All params of the childGroup related to
 *  		    	line intersection
 *  @params[out]    SplitItPair  a pair of iterators of the splitLines that
 *  		    cut the group.
 *  @brief          In order for a group to overlap with a child There has to be an overlapping area. 
 *  		    If a group falls within a child, it may not intersect with a
 *  		    split line but would intersect with the children's split
 *  		    lines.  If it is a border and no area overlaps, should be
 *  		    false.  This is not a recursive function,
 *  		    but just covers the children in the locGroup.
 ******************************************************************************************************/
Layout::SplitItPair  findOverlappingSplits(Layout::GroupPair loc, const LineIntersects& lineFunction)
{
	Layout::SplitItPair  pr { loc.first -> splits.end(), loc.first ->splits.end()};
	// find the first
	pr.first = find_if(pr.first, pr.second, lineFunction);
	pr.second = find_if_not(pr.first, pr.second, lineFunction);
	return pr;
}
/******************************************************************************************************
 *  @func    findOverlappingChildren will find within a Node of the spatial structure
 *  			tree, which Children may overlap with a new Group;
 *  @params[in]     GroupPair locGroup -Node and location in the spatial
 *  			 in the spatial structure of where to look.
 *  		    ChildIntersects.  All params of the childGroup related to
 *  		    	overlap
 *  @params[out]    ChildItPair  a pair of iterators of the splitLines that
 *  		    cut the group.
 *  @brief          In order for a group to overlap with a child There has to be an overlapping area. 
 *  		    If a group falls within a child, it may not intersect with a
 *  		    split line but would intersect with the children's split
 *  		    lines.  If it is a border and no area overlaps, should be
 *  		    false.  This is not a recursive function,
 *  		    but just covers the children in the locGroup.
 ******************************************************************************************************/
// finds overlapping children 
Layout::ChildItPair  findOverlappingChildren(Layout::GroupPair loc, LineIntersects& child)
{
	Layout::ChildIt last { loc.first -> children.end()};
	Layout::ChildItPair  pr { last, last};
	// find the first
	if ( loc.first ->children.size() == 0){
		return pr;
	}
	Efloat prior { (loc.first -> splitDir == EVector::Axis::X) ?  
	        loc.second.x : loc.second.y};
	Efloat minVal { prior};
	Efloat maxVal { (loc.first -> splitDir == EVector::Axis::X) ?  
	        loc.first -> size.x : loc.first -> size.y};
	Layout::ChildIt start { loc.first ->children.begin()};
	pr.first = start;
	// look for the first true
	for (; pr.first < last; ++pr.first)
	{
		std::vector<int>::size_type t = static_cast<std::vector<int>::size_type> (pr.first - start);
		Efloat second { (t  < loc.first -> children.size() - 1) ? 
			         loc.first -> splits[t]: maxVal}; 
		minMaxPr prOverlap {prior,  minVal + second};
		bool overlaps { child(prOverlap)};
		prior = prOverlap.second;
		if ( overlaps) {
			pr.second = pr.first+ 1;
			break;
		}
	}
	// now loop for the last match
	for (; pr.second < last; ++pr.second)
	{
		std::vector<int>::size_type t = static_cast<std::vector<int>::size_type> 
			      (pr.second - start);
		Efloat second { (t  < loc.first -> children.size() - 1) ? 
			         loc.first -> splits[t]: maxVal}; 
		minMaxPr prOverlap {prior,  minVal + second};
		bool overlaps { child(prOverlap)};
		prior = prOverlap.second;
		if ( !overlaps) {
			break;
		}
	}
	return pr;
}

Layout::LeafNode::LeafNode(const EVector& sz, const EVector::Axis sd, std::vector<Efloat>&& ss,
					std::weak_ptr<const Node>  p,
				std::shared_ptr<NodeValue> v ):
				Node(sz, sd, std::move(ss), p, v)
{}
/************************************************************************************************************
 * @func      addGroupToXYLocMap.
 * @args[in]  std::shared_ptr<const Node> groupNode
 * @return[out]  InsertType  Tells whether a new group was inserted or an old
 * 			group there.
 * @brief     will add an entry to the map or create an entry if need be.
 * 		returns true if the add was successful.  returns false if there
 * 		is already a group of the same X, Y width that has not expired
 * 		(meaning do not add).
 * 		There should only be one group with the same XYWidth.  
 * 		If the new group matches the old one, this will keep the old.
 *      if there are two ways to make the same group the second time this will
 *      return true, false.  That means the inserted group should not be added to groupMap.
 * 		The prior does not have to be eliminated with a call because if
 * 		it is deleted in the GroupMap it will be expired here.
 * 		
 * ****************************************************************************************************/
Layout::InsertType Layout::LeafNode::addGroupToXYLocMap(std::shared_ptr<const Node> inNode) const
{
	XYWidth::iterator  XYit {LL.find(inNode->size.x)};
	InsertType type {Fail};
	// x found
	bool NotFound = (XYit == LL.end());
	// if x is not found make sure there is a x entry with a map. Ymap
	// inserted. Node is not inserted yet!
	if (NotFound) {
		 std::pair<XYWidth::iterator, bool> success{
			 LL.insert(std::pair<Efloat, YWidth>(inNode->size.x, YWidth()))};
		 if (!success.second) {
			throw std::runtime_error("failed to insert in XYWidth Map");
		 }
		 XYit = success.first;
	}
	YWidth::iterator Yit { XYit -> second.find(inNode->size.y)};
	// pr.first true means there is already a group with the same location and the
	// same X and Y size.  This should be a duplicate group. check its
	// number of elements. 
	// pr.second means a match was found and it expired. Must have been
	// deleted by the groupMap owner.
	bool Found =  Yit != XYit -> second.end();
        bool expired = false;
	if (Found) {
		expired = Yit->second.expired();
	}
	//found and prior did not expire no need to replace but check nTerms
	// this case means that there are two ways to make the same group
	if (Found && !expired)
	{
		// if expired, it means that there was a single group that was
		// not repeated. check if n terminals the same
		std::shared_ptr<const Node> foundVal { Yit -> second.lock()};
		if (inNode -> v -> n != foundVal -> v -> n)
		{
			throw std::runtime_error("Two groups with the same size have different"
					"numbers of primitives");
		}
		return InsertType::OldNode;
	}
	// either not found or found and expired. replace group there.
	std::pair<YWidth::iterator, bool> success {XYit->second.insert(
			          std::make_pair(inNode->size.y, inNode))};
	if (success.second)
	{
		type =(expired)? InsertType::NewExpired: InsertType::NewNode;
	}
	if (!success.second)
	{
		throw std::runtime_error("Allocation error. new group not added");
	}
	return type;
}

/****************************************************************************************************
 * @function  list<std::shared_ptr<const Node>>  findXYLocMap(EVector::Axis ax, Efloat
 * 		size)
 * @params[in]    ax is either X or Y, the axis to look for;
 *                size is the size to match to, 
 *               if n is there then 
 *                n    is the number of terminals to match to
 * @params[out]   list<std::weak_ptr<const Node>>  the list of weak_ptrs to
 * Nodes that match.
 * ****************************************************************************************************/
std::list<std::shared_ptr<const Layout::Node>>  Layout::LeafNode::findXYLocMap(EVector::Axis ax, 
					Efloat width, unsigned n) const
{
	std::list<std::shared_ptr<const Node>> list;
	if (ax == EVector::Axis::X) { 
		XYWidth::iterator  XYit { LL.find(width)};
		if (XYit == LL.end())
		{
			return list;
		}
		YWidth::iterator curr{ XYit->second.begin() };
		YWidth::iterator last{ XYit->second.end() };
		for (; curr != last; ++curr)
		{
			if (curr->second.expired()){
				throw std::runtime_error("Expired Group in Location Map");
			}
			std::shared_ptr<const Node> cptr {curr ->second.lock()};
			// number match
			if ( cptr -> v -> n == n) {
				list.push_back(cptr);
			}
		}
	}
	else // Y Axis
	{ 
		for (XYWidth::iterator curr { LL.begin()} ; curr != LL.end(); ++curr)
		{
			YWidth::iterator  Yit { curr -> second.find(width)};
			if (Yit != curr -> second.end())
			{
				if (Yit ->second.expired()){
					throw std::runtime_error("Expired Group in Location Map");
				}
				std::shared_ptr<const Node> cptr {Yit ->second.lock()};
				if ( cptr -> v -> n == n) {
					list.push_back(cptr);
				}
			}
		}
	}
	return list;
}
std::list<std::shared_ptr<const Layout::Node>>  Layout::LeafNode::findXYLocMap(EVector::Axis ax, Efloat width) const
{
	std::list<std::shared_ptr<const Node>> list;
	if (ax == EVector::Axis::X) { 
		XYWidth::iterator  XYit { LL.find(width)};
		if (XYit == LL.end())
		{
			return list;
		}
		for (YWidth::iterator curr {XYit -> second.begin()}; 
				   curr != XYit -> second.end(); ++curr)
		{
			if (curr->second.expired()){
				throw std::runtime_error("Expired Group in Location Map");
			}
			std::shared_ptr<const Node> cptr {curr ->second.lock()};
			list.push_back(cptr);
		}
	}
	else // Y Axis
	{ 
		for (XYWidth::iterator curr { LL.begin()} ; curr != LL.end(); ++curr)
		{
			YWidth::iterator  Yit { curr -> second.find(width)};
			if (Yit != curr -> second.end())
			if (Yit ->second.expired()){
				throw std::runtime_error("Expired Group in Location Map");
			}
			std::shared_ptr<const Node> cptr {Yit ->second};
			list.push_back(cptr);
		}
	}
	return list;
}

/******************************************************************************************************
 * bool removeFromXYLocMap will remove a Node from the XY map.  It should be
 * found.  returns true if found and removed successfully */
bool Layout::LeafNode::removeFromXYLocMap(std::shared_ptr<const Layout::Node> inNode) const
{
	if (inNode == nullptr){
		return false;
	}
	XYWidth::iterator XYit { LL.find(inNode->size.x)};
	if (XYit == LL.end()){
	 	throw std::runtime_error("No node with that X width found");
	}
	YWidth::iterator Yit { XYit -> second.find(size.y)};
	if (Yit == XYit ->second.end()){
		throw std::runtime_error("No Node with Y Width");
	}
	XYit ->second.erase(Yit);
	if (XYit ->second.size() == 0){
		LL.erase(XYit);
	}
	return true;
}

Layout::nGroupPair Layout::makeParentGroup(const std::vector<Layout::GroupPair> PrChildren, EVector::Axis splitDir, 
			    std::string name)
{
	
	if (PrChildren.size() == 0) {
		return Layout::nGroupPair( nullptr, EVector());
	}
	nGroupPair parentGrp { nullptr , PrChildren[0].second};
	std::vector<std::shared_ptr<const Node>> children;
	std::vector<Efloat>  splits;
	std::shared_ptr<NodeValue>  v { std::make_shared<NodeValue>(NodeValue{
				std::numeric_limits<uIDType>::max(), name, 0})};
	typedef std::vector<Layout::GroupPair>::const_iterator GrpPairIt;
	GrpPairIt last {PrChildren.cend()};
	EVector size;
	for (GrpPairIt it {PrChildren.cbegin()};it < last; ++it)
	{
		children.push_back(it -> first);
		v -> n += it -> first -> v -> n;
		// prior to last iteration
		if ( it + 1 != last) 
		{
			if (splitDir == EVector::Axis::X)
			{
				splits.push_back(it->second.x + it -> first-> size.x -
						parentGrp.second.x);
			}
			else
			{
				splits.push_back(it->second.y + it -> first -> size.y -
						parentGrp.second.y);
			}
		}
		else
		{
			size = it -> second + it -> first -> size - parentGrp.second;
		}
	}
	parentGrp.first = std::make_shared<Node>(size, splitDir, std::move(splits), 
			std::weak_ptr<Node>(), v);
	parentGrp.first -> children = std::move(children);
	return parentGrp;
}


	

	
// returns a list of Children Nodes ordered according to the splits
// The node passed in is the Top level serializable node
std::vector<std::shared_ptr<const Layout::Node>> Layout::BottomUp::GetChildren(const std::vector<Efloat>& splits, 
				const EVector::Axis ax, const tinyxml2::XMLNode * parent, 
				std::weak_ptr<const Node> p, const EVector& minVal, 
				int level, Layout::nameMap& namesFound)
{
	std::vector<std::shared_ptr<const Node>> children;
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
	else 
	{ 
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
std::shared_ptr<const Layout::Node> Layout::BottomUp::XMLNode(Layout::XMLNodePr&& nodePr, std::weak_ptr<const Node> p,
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
		splits = std::move(parseList(dir, minVal.x));
	}
	else {
		dir = nodePr.first ->FirstChildElement("SplitsY");
		splits = std::move(parseList(dir, minVal.y));
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
		GroupMap::const_iterator it {addToGroupMap(lf, minVal, group)};
		// this adds the node itself as the first group stored in the
		// Lower Left corner.
		std::shared_ptr<const LeafNode> clf = std::const_pointer_cast<const LeafNode>(lf);
		Layout::InsertType val{ lf ->addGroupToXYLocMap(clf)};
		if (val!=InsertType::NewNode) {
			throw std::runtime_error("This terminal added before or failed to add");
		}
		thisNode = lf;
	}
	else {
		v -> n = 0;
		// add only the terminals in all the children. Get a good count
		// for the number of terminals contained in the node
		//GroupType  group{ addNodeValue(v, namesFound)};
		thisNode = std::make_shared<BranchNode>(std::move(size), splitDir, std::move(splits),
				    p, v);
		//GroupMap::iterator it {addToGroupMap(thisNode, minVal, group)}; 
	}
	thisNode ->children = GetChildren(thisNode ->splits, thisNode->splitDir, nodePr.first, thisNode, 
							minVal, level + 1, namesFound);
	for (std::shared_ptr<const Node> child: thisNode ->children)
	{
			thisNode -> v-> n += child -> v->n;
	}
	return thisNode;
}
bool Layout::operator==(const Layout::NodeValue& a, const Layout::NodeValue& b)
{
	bool equal {a.uid == b.uid};
	equal = equal && a.name == b.name;
	equal = equal && a.n == b.n;
	return equal;
}

bool checkNodeValues(std::shared_ptr<const Layout::NodeValue> a, std::shared_ptr<const Layout::NodeValue> b)
{
	return a == b || *a == *b;
}

/****************************************************************************************************
 *          addNodeValue will just add the NodeValue to the nameMap; if
            there is one there already, this returns the current one and
            discards the nodeValue entered. It checks if all matching Nodes have the equivalent NodeValue.
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
		GroupMapIt itpair  {groups.equal_range(itName -> second)};
		if (itpair.first == groups.end())
		{
			throw std::runtime_error("Group corresponding to name not found.");
		}
		// check node values
		// This does not add but rather checks if all the values in
		// matching nodes match this on.
		GroupPair pr = itpair.first->second;
		std::shared_ptr<const Node> nd{ pr.first };
		nodeValue->uid = nd ->v->uid;
		for ( GroupMap::const_iterator it { itpair.first} ; it != itpair.second; ++it)
		{
			if (!checkNodeValues(it->second.first->v, nodeValue)) {
				throw std::runtime_error("Node Values not all identical.");
			}
		}
		// update the nodeValue
		nodeValue = itpair.first->second.first->v;
	}
	return group;
}


/***************************************************************************************************************
 *              addNodeTo GroupMap; This adds a new Node to the group Map.  It
 *              returns an iterator to the list element that holds the node.
 **************************************************************************************************************/
Layout::GroupMap::const_iterator Layout::BottomUp::addToGroupMap(
	std::shared_ptr<const Layout::Node> node, const EVector& minLocation, Layout::GroupType grouptype)
{
	uIDType uid =  node -> v-> uid;
	GroupMapIt itpair  {groups.equal_range(uid)};
	// there is no iterator in groups but there was an entry in the nameMap:
	// Error
	if (itpair.first == groups.end() && grouptype == GroupType::Existing) 
	{
		throw std::runtime_error("Group in Name map but not in group map");
	}
	else if (itpair.first != groups.end() && grouptype == GroupType::New)
	{
		throw std::runtime_error("Goup not in NameMap but in Group");
	}
	// check if this pair matches any existing pairs
	bool found{ false };
	for (GroupMap::const_iterator it { itpair.first} ; it != itpair.second; ++it)
	{
		if ( itpair.first -> second.second == minLocation)
		{
			throw std::runtime_error("Two groups with same groupid and location");
		}
	}
	// Node should not be in the group;
	GroupPair pr {std::make_pair(node, minLocation)};
	GroupMap::const_iterator it {groups.insert(
		std::make_pair(uid,std::move(pr)))};
	if (it == groups.end()) {
		throw std::runtime_error("failed To add to Group map");
	}
	return it;
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
		removeSingles(0, next);
		addNTGroups(2);
}

/*************************************************************************************************************
 * @func  	removeSingles  removes groups that are repeated only once
 * @params 	[start, last),   First(inclusive), Last[exclusive] ID to search from
 * ************************************************************************************************************/
void Layout::BottomUp::removeSingles(uIDType current, uIDType last)
{
	for (; current < last; ++current)
	{
		//Exactly one element was found
		// need the range so only look up once.
		GroupMapIt pr{ groups.equal_range(current) };
		//GroupMap::size_type t = static_cast<GroupMap::
		//	size_type>(std::distance(pr.first, pr.second));
		if (pr.first == groups.end()) {
			// no elements found
			continue;
		}
		GroupMap::const_iterator second = pr.first;
		++second;
		// one element so delete
		if (second == pr.second) {
			if (pr.first == groups.end()) {
				throw std::runtime_error("No groups found when there should have been 1 found");
			}
		    	nameMap::iterator  nameit = names.find( pr.first -> second.first -> v -> name);
		    	if (nameit == names.end())
		    	{
			    throw std::runtime_error("Name corresponding to UID not found");
		    	}
		    	names.erase(nameit);
				EVector StartSearch = pr.first->second.second;
		    	std::shared_ptr<const LeafNode>  llcorner { findLLNode(pr.first -> second.first , 
					                                    StartSearch,
				                            pr.first -> second.second)};
		    	bool success {llcorner ->removeFromXYLocMap(pr.first ->second.first)};
		    	if (!success){
			    throw std::runtime_error("failed to remove node in XY Location map");
		    	}
		    	groups.erase(pr.first);
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
void   Layout::parentLLCorner(std::shared_ptr<const  Layout::Node> parent, std::shared_ptr<const Layout::Node> child, 
				EVector& minValueChild)
{
	std::vector<std::shared_ptr<const Layout::Node>>::const_iterator it = find(parent->children.cbegin(), 
						parent->children.cend(), child);
	if ( it == parent->children.end() )
	{
		throw std::runtime_error("child not found");
	}
	std::ptrdiff_t diff = it - parent->children.begin();
	if (diff == 0)
	{
		return;
	}
	if (parent -> splitDir == EVector::Axis::X)
	{
		minValueChild.x -= parent->splits[diff -1];
	}
	else{
		minValueChild.y -= parent->splits[diff -1];
	}
	return;
}
	

/**************************************************************************************************
 * @func    std::shared_ptr<Node> findLLNode(std::shared_ptr<Node> init, Evector& lowerLeft, 
 * 				const EVector& term)
 * @params[in]    std::shared_ptr<const Node> init: start Node. If it is a terminal node then you
 * 			can navigate the location KD tree and go up and down.
 * 			If it is a groupNode in the KD tree then you also can go
 * 			up and down.  It can also be used to find the Lower Left
 * 			corner of a group node. In this case the lowerleft and
 * 			term should be the same.
 *                EVector& ll init location of that node in the
 *                		spatial structure. This will get updated for each call up and down, so
 *                		will get modified
 *                		 as the algorithm traverses the tree.
 * 		  const EVector&   term the lower left corner that one wants to get to
 * @params[out]   std::shared_ptr< const Node>  the terminal node with this coordinate or null if no
 *                        such pointer exists
 * @precondition  initial node exists.
 * @brief          will look through the tree starting at the GroupPair, searching up the tree
 * 		   or down the tree and return the node that has the lowerLeft corner at term
 ************************************************************************************************/
std::shared_ptr<const Layout::LeafNode> Layout::findLLNode(std::shared_ptr< const Layout::Node> curr,  EVector& ll, const EVector& term)
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
			// updates ll to be that of the parent
			parentLLCorner(pp, curr, ll);
			return findLLNode(pp, ll, term);
		}
		else {
			return nullptr;
		}
	}
	//terminal case where either this location is the lower left or is not	
	if (  curr->v->terminal()) {
		// std::shared_ptr<LeafNode> lf = std::dynamic_pointer_cast<LeafNode>(curr);
		return (term == ll)? std::dynamic_pointer_cast<const LeafNode>(curr): std::shared_ptr<const LeafNode>(nullptr);
	}
	// within a child find the child
	
	EVector::Axis ax = curr->splitDir;
	Efloat minVal { ( ax == EVector::Axis::X)? term.x - ll.x: term.y - ll.y};
	// find the last value that minVal could be inserted and is greater than relativeMin. 
	// This is the first slit that is greater than the value  and is the correct
	// function.
	std::vector<Efloat>::const_iterator it {std::upper_bound( curr->splits.cbegin(), 
			 	curr->splits.cend(), minVal)};
	int indx { static_cast<int>(it - curr->splits.cbegin())};
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
	

 /*   @brief      if both terminal compares uid for similarity, If both not
 *   		  terminal does not compare uid of the a, b but rather compares
 *   		  uid of all the children.  Thus will identify same groups based
 *   		  on topology and the identity of children, so when groups are
 *   		  being formed, their similarity can be tested for
 ****************************************************************************************************/
bool Layout::sameGroup(std::shared_ptr<const Layout::Node> a, std::shared_ptr<const Layout::Node> b)
{
	bool same {false};
	// if a is term and b is not or vice versa same is false (default)
	if ( a->v ->terminal() && b ->v -> terminal() ) 
	{
		same = (a->v -> uid == b->v -> uid);
	}
	// both not terminal
	else if ( ! (a ->v -> terminal() ||  b -> v -> terminal()))
	{
		if ( a -> splitDir == b -> splitDir && 
				a -> children.size() == b -> children.size())
		{
		        same = true;
			typedef std::vector<std::shared_ptr<const Layout::Node>>::const_iterator VecIT;
			VecIT ait { a->children.cbegin()};
			VecIT bit { b -> children.cbegin()};
			for (; ait < a -> children.cend(), bit < b ->children.cend(); 
					++ait, ++ bit)
			{
				if ((*ait) -> v -> uid != (*bit) -> v -> uid)
				{
					same  = false;
					break;
				}
			}
		}
	}
	return same;
}
// add nonterminal groups of size n to groupMap
void Layout::BottomUp::addNTGroups(unsigned in)
{
	uIDType first {next};
	for (uIDType u = 0; u < first; u++)
	{
		GroupMapIt pr {groups.equal_range(u)};
		if (pr.first == groups.end())
		{
				continue;
		}
	}
}
// creates new groups. the pr should be at least all the iterators of a unique id.
// It will check if the created groups match from the start iterator and if it
// does will use that uid.
void Layout::BottomUp::addNTGroups(Layout::GroupMapIt pr, EVector::Axis ax, unsigned nTerms)
{

	uIDType first {next};
	// no groups in range
	if (pr.first == pr.second)
	{
			return;
	}
	for ( GroupMap::const_iterator it = pr.first; it != pr.second; ++it)
	{
		unsigned termsInGroup { it -> second.first->v -> n};
		// no groups to add
		if (termsInGroup >= nTerms){
			continue;
		}
		unsigned termsSeek {nTerms - termsInGroup};
		// startLoc  where to begin searching
		EVector startLoc { it ->second.second};
		// this corner is terminal in ll corner. 
		std::shared_ptr<const LeafNode> thisCorner { findLLNode(it ->second.first, startLoc, it ->second.second)};
		// target is the LL corner of neighbor sought.
		const EVector target{ (ax == EVector::Axis::X) ?
			   it->second.second + it->second.first->size.x :
			   it->second.second + it->second.first->size.y };
		std::shared_ptr<const LeafNode> neighbor  {findLLNode(thisCorner, startLoc, target)};
		if (neighbor == nullptr) {
			continue;
		}
		// matchingNeighbors will be a list of all groups that match
		// width and number of terminals.  for neighbor to the left X
		// should match Y width.
		std::list<std::shared_ptr<const Node>> matchingNeighbors { (ax == EVector::Axis::X)?
			 neighbor -> findXYLocMap(EVector::Axis::Y, it -> second.first -> size.y, termsSeek) :
			 neighbor -> findXYLocMap(EVector::Axis::X, it -> second.first -> size.x, termsSeek)};
		for (std::shared_ptr<const Node> mneighbor : matchingNeighbors)
		{
			const std::vector<GroupPair> children { it -> second, GroupPair( mneighbor, target)};
			std::string name { "group of " + nTerms};
			name += " with subgroups: " +
				it->second.first->v->uid;
			name += " and " + neighbor -> v -> uid;
			GroupPair NewGroupPr { makeParentGroup( children, ax, name)};
			// find first matching Group
			// current has the id of this group;
			uIDType  curr {first};
			GroupType grouptype { Layout::GroupType::New};
			for ( ; curr < next; ++curr) {
				Layout::GroupMapIt matching {groups.equal_range(curr)};
				if (matching.first != matching.second)
				{ 
					if (Layout::sameGroup(matching.first -> second.first, NewGroupPr.first)){
							grouptype = GroupType::Existing;
							break;
					}
				}
			}
			NewGroupPr.first->v -> uid = curr;
			// new group number
			if (curr == next) ++next;
			addToGroupMap( NewGroupPr.first, NewGroupPr.second, grouptype);
			thisCorner -> addGroupToXYLocMap(NewGroupPr.first);
		}

	}
}



