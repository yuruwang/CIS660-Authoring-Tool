//define NDEBUG
#include <assert.h>
#include "parseLayout.h"
#include <sstream>
#include <algorithm>

namespace Layout {
	/* ******************************************************************************************************************
	 * LineBasics is a base class that holds common functions and common 
	 * data like the data for xLocPr, yLocPr and the axis.  
	 ********************************************************************************************************************/
	class LineBasics {
	protected:
		// minMap x, y dimensions of spatial Node bounding box
		minMaxPr xLocPr, yLocPr;
		// axis of GroupNode
		EVector::Axis splitDir;
	public:
		// initialize all variables
		LineBasics(const GroupPair& StartLocation);
		// updates the StartLocation Group
		void updateSearchCorner(Layout::GroupPair& loc);
		// anyOverlaps determins if any splitlines within startLocation
		// could overlap with inputted nonterminal group
		bool anyOverlaps() const;
		// provide the size of the location and this will return true
		// if the group is contained within location.  If it not within
		// the location then one needs to look to the parent for a
		// startGroup.
		bool groupWithinLocation() const;
		// used to get whether a child node with a minMaxPr
		// along current splitDir overlaps with NTgroup
		bool operator()(const minMaxPr pr) const;
		// determines whether a splitline verlaps with current
		// NTgroup.  The splitLine is within StartLocation group
		bool operator()(const Efloat& splitLine) const;
		// returns current Axis
		EVector::Axis axis() const;
	};
	/* ******************************************************************************************************************
	 * LineIntersects tests if a splitLine within a spatialLocation GroupPair splits a
	 * combined group. The group has a non zero width and a height. It also tests whether a childNode is
	 * overlapping with the group, where the group is within the location
	 ********************************************************************************************************************/
	class LineIntersects: public LineBasics {
	private:
		// the size of the group  Node
		// group lowerLeft;
		// minMax pairs of the group Node
		const minMaxPr xGroup, yGroup;
	public:
		// initialize all variables
		LineIntersects(const GroupPair& StartLocation, const GroupPair& NTgroup);
		// anyOverlaps determins if any splitlines within startLocation
		// could overlap with inputted nonterminal group
		bool anyOverlaps() const;
		// provide the size of the location and this will return true
		// if the group is contained within location.  If it not within
		// the location then one needs to look to the parent for a
		// startGroup.
		bool groupWithinLocation() const;
		// used to get whether a child node with a minMaxPr
		// along current splitDir overlaps with NTgroup
		bool operator()(const minMaxPr pr) const;
		// determines whether a splitline verlaps with current
		// NTgroup.  The splitLine is within StartLocation group
		bool operator()(const Efloat& splitLine) const;
	};
	// determine if a LineSegment overlaps with any splitline in the
	// BranchNode.  Also given a LineSegment and a GroupNode will determine
	// if the line segment overlaps with the group
	class LineOverlapsLine: public LineBasics {
		private:
			// this is the x or y minMaxPr along the splitDirection;
			const Layout::LineSegment  line;  // the line that encodes
			// true if the the splits in the branch Node are along
			// the lineSegment
			bool  aligned;

		public:
			// initialize all variables
			LineOverlapsLine (const GroupPair& loc,  LineSegment ls);
			//  find 
			void updateSearchCorner(Layout::GroupPair& loc);
			// Given the size of the spatial Location  or Branche Node, will determine
			// in any lines within this overlap
			bool anyOverlaps() const;
			// groupWithinLocation determines if the lineSegment is
			// completely within the GroupPair XLocPr, yLocPr.
			bool groupWithinLocation() const;
			// provide the size of the location and this will return true
			// if the group is contained within location.  If it not within
			// the location then one needs to look to the parent
			// supply a child min max range for the bounding box in the
			// splitDir true means that should could contain an Overlap line
			bool operator()(const Layout::minMaxPr pr) const; 
			// true if the splitline overlaps the stored line
			bool operator()(const Efloat& splitLine) const;
	};
	/*******************************************************************************************************************
	 *      @func findContainingParent will find the branch Node that contains the
	 *      complete nonTerminal Group (groupPair), NTGroup.  It walks up the tree looking until it finds a
	 *       parest that contains the group.
	 *      @params[in]       currentLoc.  A grouppair with a terminal node that to
	 *      		  	to begin searching.  Needs to start with a terminal
	 *      		  	node, so that it can walk up the spatial tree.
	 *      		  line is made with LineIntersects( currentLoc, NTgroup)
	 *       @return          The groupPair of the node containing the NTgroup
	 * *****************************************************************************************************************/
	template<typename T> GroupPair    findContainingParent(Layout::GroupPair currentLoc, 
				 T& line)
	{
		// true when the new group within line is within this currentLoc.  
      		if (line.groupWithinLocation()) {
			return currentLoc;
		}
		// no so check parent. initialize with child location.
		EVector parentLoc { currentLoc.second};
		// parent is null
		if ( currentLoc.first->parent.expired() ||  currentLoc.first->parent.lock() == nullptr)
		{
			throw std::runtime_error("Parent expired or not large enough for this group");
		}
		// this will update the parent location
		parentLLCorner( currentLoc.first -> parent.lock(), currentLoc.first, parentLoc);
		GroupPair parentPair( currentLoc.first -> parent.lock(), parentLoc);
		line.updateSearchCorner(parentPair);
		return Layout::findContainingParent(parentPair, line);
	}
	/******************************************************************************************************************
	 *      @func branchesWithOverlappingSplits finds all branches and the splitlines
	 *         	within the branch that has overlapping splits with a given
	 *         	Nonterminal group.
	 *	@params[in]  current.  GroupPair of the parentNode in spatial
	 *			structure big enough to contain any possible split
	 *		     LineIntersect& line made with (parentGroup, NTGroup) and it
	 *		     	characterizes the overlap between the
	 *		     	NTGroup and the Current Group.
	 *	           vector<BranchSplitPairs> all the splitlines that overlap with
	 *	this group
	 *	*********************************************************************************************************/
	template<typename T> void branchesWithOverlappingSplit(GroupPair currentLoc, T& line,  
			             std::vector<BranchSplitPair>& splits )
	{
		if (!line.anyOverlaps()){
			return;
		}
		//do the splits in current Node
		SplitItPair splitIndices { findOverlappingSplits(currentLoc, line)};
		if (splitIndices.first != splitIndices.second) {
			splits.push_back(BranchSplitPair(currentLoc.first, splitIndices));
		}
		ChildItPair childrenPairs { findOverlappingChildren( currentLoc, line)};
		std::ptrdiff_t  diff { childrenPairs.first - currentLoc.first ->children.begin()};
		// first child of currentLoc.first is an overlapping child
		std::vector<Efloat>::size_type indx {0};
		EVector childMin {currentLoc.second};
		if (diff > 0 && diff < static_cast<std::ptrdiff_t>(currentLoc.first ->children.size())) {
			indx = diff - 1;
			if (line.axis() == EVector::Axis::X)
			{
				childMin.x = currentLoc.second.x + currentLoc.first ->splits[indx++];
			}
			else{
				childMin.y = currentLoc.second.y + currentLoc.first ->splits[indx++];
			}
		}
		if ( line.axis() == EVector::Axis::X)
		{
			for (; childrenPairs.first < childrenPairs.second; ++childrenPairs.first )
			{ 
				GroupPair childPair ( *childrenPairs.first, childMin);
				line.updateSearchCorner(childPair);
				branchesWithOverlappingSplit(childPair, line, splits); 
				if ( indx < currentLoc.first->splits.size()) {
					childMin.x = currentLoc.second.x + 
						currentLoc.first -> splits[indx++];
				}
			}
	
		}
		else 
		{ 
			for (; childrenPairs.first < childrenPairs.second; ++childrenPairs.first )
			{ 
				GroupPair childPair ( *childrenPairs.first, childMin);
				line.updateSearchCorner(childPair);
				branchesWithOverlappingSplit(childPair, line, splits); 
				if ( indx < currentLoc.first ->splits.size()) {
					childMin.y = currentLoc.second.y + 
						currentLoc.first -> splits[indx++];
				}
			}
		}
	}
}
/**************************************************************************************************************************
 * WeakCompare will compare weak_ptr<const Node>
 * ************************************************************************************************************************/
class  WeakCompare {
	private:
		std::shared_ptr<const Layout::Node> ntGroup;
	public:
		WeakCompare(std::shared_ptr<const Layout::Node> nt) :ntGroup{nt}
		{}
		// Compares Weak and shared pointer for the same address
		bool operator()(const Layout::WeakPair& T) {
			return !(T.second.owner_before(ntGroup) || 
					ntGroup.owner_before(T.second));
		}
};
/****************************************************************************************************************************
 * SharedCompare  will compare shared pointers with a set pointer
 */
class  SharedCompare {
	private:
		const std::shared_ptr<const Layout::Node> ntGroup;
	public:
		SharedCompare(const std::shared_ptr<const Layout::Node> nt) :ntGroup{nt}
		{}
		// Compares Weak and shared pointer for the same address
		bool operator()(const std::shared_ptr<const Layout::Node>& T) {
			return T == ntGroup;
		}
};
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
// Branch Nodes always have nonempty splits
Layout::BranchNode::BranchNode(const EVector& sz, const EVector::Axis sd, std::vector<Efloat>&& ss,
					std::weak_ptr<const Node> p, std::shared_ptr<NodeValue> v ):
					Node(sz, sd, std::move(ss),  p, v), splitGroups{std::vector<WeakMap>(splits.size())}
{}
// adds a weak reference to the groupNode split by the
			// line.  LL corner is not needed so not stored
			// throws exception if expired group or failure
void Layout::BranchNode::addGroup(std::shared_ptr<const Layout::Node> NTGroup, Layout::SplitItPair pr) const
{
	typedef std::vector<WeakMap>::size_type WS;
	WS curr { static_cast<WS>( pr.first - splits.begin())};
	WS last  { static_cast<WS>(pr.second - splits.begin())};
	for (; curr < last; ++curr)
	{
		WeakMap& thisMap = splitGroups[curr];
		std::weak_ptr<const Node> weakVal { NTGroup};
		WeakMap::const_iterator it {thisMap.insert(
				    std::make_pair(NTGroup -> v ->uid, weakVal))};
		if ( it == thisMap.end()) {
			throw std::runtime_error("failed to insert group in splitGroups");
		}
	}

}

			// removes a group from the branchNode
			// true if found /false if not found; throws exception
			// if expired
bool Layout::BranchNode::removeGroup(std::shared_ptr<const Layout::Node> NTGroup, std::vector<int>::size_type curr) const
{
	WeakMap& thisMap = splitGroups[curr];
	typedef WeakMap::const_iterator WIT;
	std::pair<WIT, WIT> pr { thisMap.equal_range(NTGroup -> v -> uid)};
	bool found {false};
	for ( ; pr.first != pr.second; ++pr.first)
	{
		if (pr.first ->second.expired()){
			throw std::runtime_error("expired pointer to Group Node in splits");
		}
		if (pr.first ->second.lock() == NTGroup)
		{
			thisMap.erase(pr.first);
			found = true;
			break;
		}
	}
	return found;
}
// strictly Overlap is true only if a and b share common points that are not on
// the boundary
bool  strictlyOverlap( const Layout::minMaxPr a, const Layout::minMaxPr b)
{
	bool overlaps {a.first < b.second &&  b.first < a.second};
	overlaps = overlaps || (b.first < a.second && a.first < b.second);
	return overlaps;
}

// strictly Overlap is true only if b falls within a and is not on
// the boundary
bool  strictlyOverlap( const Layout::minMaxPr a, const Efloat b)
{
	bool overlaps {a.first < b &&  b < a.second};
	return overlaps;
}
// containedWithin  is true if b is contained within a. They can share a
// boundary.
bool  containedWithin(const Layout::minMaxPr a, const Layout::minMaxPr b)
{
	bool overlaps {a.first <= b.first &&  b.second <= a.second};
	return overlaps;
}
/* LineBasics holds common information;
   group. The group has a non zero width and a height. It also tests whether a childNode is overlapping with the group,
   where the group is within the location  and weth*/
 
Layout::LineBasics::LineBasics(const Layout::GroupPair& loc):
	xLocPr { Layout::minMaxPr(loc.second.x, loc.second.x + loc.first -> size.x)}, 
	yLocPr { Layout::minMaxPr(loc.second.y, loc.second.y + loc.first -> size.y)},
	splitDir { loc.first -> splitDir}
{}
/* LineIntersects tests if a splitLine within a spatialLocation Node splits a
   group. The group has a non zero width and a height. It also tests whether a childNode is overlapping with the group,
   where the group is within the location  and wether there are any overlaps at all*/
 
Layout::LineIntersects::LineIntersects(const Layout::GroupPair& loc, const Layout::GroupPair& group):
	LineBasics(loc), 
	xGroup { Layout::minMaxPr(group.second.x, group.second.x + group.first -> size.x)},
	yGroup { Layout::minMaxPr(group.second.y, 
	       	 group.second.y + group.first -> size.y)}
{}
		// updates the corner to begin searching from;
void Layout::LineBasics::updateSearchCorner(Layout::GroupPair& loc)
		{
			 xLocPr = Layout::minMaxPr(loc.second.x, loc.second.x + loc.first -> size.x); 
			 yLocPr = Layout::minMaxPr(loc.second.y, loc.second.y + loc.first -> size.y);
			 splitDir = loc.first -> splitDir;
		}

		// Given the size of the spatial Location Node, will determine
		// in any lines within this node could possible be splitlines
bool Layout::LineBasics::anyOverlaps() const
		{
		    return false;
		}
bool Layout::LineIntersects::anyOverlaps() const
		{
		    bool Overlaps {strictlyOverlap(xLocPr, xGroup)};
		    Overlaps = Overlaps && strictlyOverlap(yLocPr, yGroup);
		    return Overlaps;
		}
bool Layout::LineBasics::groupWithinLocation() const 
		{
		    return false;
		}
		// provide the size of the location and this will return true
		// if the group is contained within location.  If it not within
		// the location then one needs to look to the parent
bool Layout::LineIntersects::groupWithinLocation() const 
		{
		    bool Overlaps {containedWithin(xLocPr, xGroup)};
		    Overlaps = Overlaps && containedWithin(yLocPr, yGroup);
		    return Overlaps;
		}
		// used to get whether a child node
bool Layout::LineBasics::operator()(const Layout::minMaxPr pr) const 
		{
			return  false;
		}
bool Layout::LineIntersects::operator()(const Layout::minMaxPr pr) const 
		{
			return  (splitDir == EVector::Axis::X) ? 
				strictlyOverlap(pr, xGroup) : strictlyOverlap(pr, yGroup);
		}
bool Layout::LineBasics::operator()(const Efloat& splitLine) const
		{
		      return false;
		}
bool Layout::LineIntersects::operator()(const Efloat& splitLine) const
		{
		      return (splitDir == EVector::Axis::X)? 
			      strictlyOverlap(xGroup, xLocPr.first + splitLine):
			      strictlyOverlap(yGroup, yLocPr.first + splitLine);
		}
EVector::Axis  Layout::LineBasics::axis() const
{
	return splitDir;
}
/* LineOverlapsLine tests if a splitLVine is within a spatialLocation 
   group. The group has a non zero width and a height. It also tests whether a childNode is overlapping with the group,
   where the group is within the location  and wether there are any overlaps at all*/
 
Layout::LineOverlapsLine::LineOverlapsLine (const Layout::GroupPair& loc,  Layout::LineSegment ls):
			LineBasics(loc),
			 // splitDir == Y means the split lines go along X and
			 // have one value at Y; for LineSegment this means
			 // line.ax == X ( because line segment is along X)
			 line  { ls}, aligned{ line.ax != splitDir}{}
void Layout::LineOverlapsLine::updateSearchCorner(Layout::GroupPair& loc)
		{
			LineBasics::updateSearchCorner(loc); 
			aligned = splitDir != line.ax;
		}
bool Layout::LineOverlapsLine::anyOverlaps() const
{
	bool Overlaps;
	if (line.ax == EVector::Axis::X)
	{ 
		Overlaps = strictlyOverlap( yLocPr, line.transverseVal);
		Overlaps = Overlaps && strictlyOverlap(xLocPr, line.pr);
	}
	else
	{ 
		Overlaps = strictlyOverlap( xLocPr, line.transverseVal);
		Overlaps = Overlaps && strictlyOverlap(yLocPr, line.pr);
	}
	return Overlaps;
}
bool Layout::LineOverlapsLine::groupWithinLocation() const 
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
bool Layout::LineOverlapsLine::operator()(const Layout::minMaxPr pr) const 
{
	return (aligned) ? strictlyOverlap(pr, line.transverseVal):
			strictlyOverlap(pr, line.pr);
}
// true if the splitline overlaps the stored line
bool Layout::LineOverlapsLine::operator()(const Efloat& splitLine) const
{
	// if not aligned, the stored line and the splits are perpendicular.
	// No split lines can equal.
	bool overlap {false};
	if (aligned) {
		Efloat val {(splitDir == EVector::Axis::X)? xLocPr.first : yLocPr.first};
		val +=splitLine;
		overlap = (val == line.transverseVal);
	}
	return overlap;
}
/******************************************************************************************************
 *  @func    findOverlappingSplits will find within a Node of the spatial structure
 *  			tree, which Splits may overlap with a new Group;
 *  @params[in]     GroupPair locGroup -Node and location in the spatial
 *  			 in the spatial structure of where to look.
 *  		    LineIntersects/LineOverlapsLine.  All params of the childGroup related to
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
template<typename T> Layout::SplitItPair  findOverlappingSplits(Layout::GroupPair loc, const T& 
		           lineFunction)
{
	Layout::SplitItPair  pr { loc.first -> splits.begin(), loc.first ->splits.end()};
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
 *  @params[out]    ChildItPair  a pair of iterators of the children nodes that
 *  		    contain overlaps.
 *  @brief          In order for a group to overlap with a child There has to be an overlapping area. 
 *  		    If a group falls within a child, it may not intersect with a
 *  		    split line but would intersect with the children's split
 *  		    lines.  If it is a border and no area overlaps, should be
 *  		    false.  This is not a recursive function,
 *  		    but just covers the children in the locGroup.
 ******************************************************************************************************/
// finds overlapping children 
template<typename T> Layout::ChildItPair  findOverlappingChildren(Layout::GroupPair loc, T& child)
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
		Layout::minMaxPr prOverlap {prior,  minVal + second};
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
		Layout::minMaxPr prOverlap {prior,  minVal + second};
		bool overlaps { child(prOverlap)};
		prior = prOverlap.second;
		if ( !overlaps) {
			break;
		}
	}
	return pr;
}


// ll is the lower left of the terminal node starting location, ntGroup is the
// non terminal group to insert into the splitlines
void Layout::addNTGroupToSplitLines(Layout::GroupPair ll, Layout::GroupPair ntGroup)
{
	// get the line intersects class to determine overlaps and split lines 
	Layout::LineIntersects  line( ll, ntGroup);
	Layout::GroupPair  parent {Layout::findContainingParent(ll, line)};
	std::vector<Layout::BranchSplitPair> splits;
	// find all the split lines
	Layout::branchesWithOverlappingSplit(parent, line, splits);
	for ( Layout::BranchSplitPair pr : splits)
	{
		std::shared_ptr<const Layout::BranchNode> br { 
			std::dynamic_pointer_cast<const Layout::BranchNode>(pr.first)};
		if (br == nullptr) 
		{ 
			throw std::runtime_error("dynamic cast failed to get branch Node");
		}
		br ->addGroup( ntGroup.first, pr.second);
	}
}
// basic Function: takes a const Node and inserts it into the NodeMap nm.
void insertIntoNodeMap( Layout::NodeMap& nm,  std::shared_ptr<const Layout::Node> thisNode)
{
	auto lam{
	   [=](const std::pair<Layout::uIDType, std::shared_ptr<const Layout::Node>>& T) -> bool
	   {
		   return T.second == thisNode;
		} };
	Layout::NodeMapItPr nmIt {nm.equal_range(thisNode ->v -> uid)};
	Layout::NodeMapIt foundIt { std::find_if(nmIt.first, nmIt.second, lam)};
	if (foundIt != nmIt.second) {
		nm.insert(std::make_pair(thisNode->v ->uid, thisNode));
		foundIt = std::find_if( ++nmIt.first, nmIt.second, lam);
		if (foundIt !=nmIt.second) {
			throw std::runtime_error("found twice in nodeMap");
		}

	}
}
/******************************************************************************
 * addSplitsToNodeMap will all all of the nodes in a BranchSplitPair to an
 * existing NodeMap (nm).  It will only add unique Nodes, not duplicats, where a
 * duplicate is the name uid in the exact same location.
 * *****************************************************************************/
void addSplitsToNodeMap( Layout::NodeMap& nm, Layout::BranchSplitPair pr) 
{
		std::shared_ptr<const Layout::BranchNode> br { 
			std::dynamic_pointer_cast<const Layout::BranchNode>(pr.first)};
		if (br == nullptr) 
		{ 
			throw std::runtime_error("dynamic cast failed to get branch Node");
		}
		Layout::SplitIt start = br->splits.begin();
		typedef std::vector<Layout::NodeMap>::size_type ST;
		// loop over every split line
		for (; pr.second.first != pr.second.second; ++pr.second.first)
		{
			ST indx { static_cast<ST>(pr.second.first - start)};
			Layout::WeakMap& map { br -> splitGroups[indx]};
			// loop over every element
			for (Layout::WeakMapIt start { map.begin()}; start != map.end(); ++start)
			{
				if (start ->second.expired()){
					throw std::runtime_error("expired Node is split");
				}
				insertIntoNodeMap(nm, start -> second.lock());
			}
		}
}

/******************************************************************************************************************************
 * @func        allSplitGroups will take lineSegment and return all the groups
 * 			that are split by the line segment. The groups returned
 * 			are unique but there may be more than one ntGroup with
 * 			the same index. 
 * @params[in]   GroupPair LL, the starting search location.  Will go up and
 * 			down the tree to find all overlapping lineSegments.
 *               lineSegment & line , the line segment that defines the line you
 *               are looking for. 
 *  @return      a map of const Nodes that are cut by the line
 *  @brief       There may be a single line if the line happens to be exactly
 *  		along the kd tree splitlines, or it could be along multiple kd
 *  		tree splitlines.  This gathers the splits from all of them. It
 *  		does not remove any groups from branch Nodes.
 *  ******************************************************************************************************************/
Layout::NodeMap Layout::allSplitGroups(Layout::GroupPair ll, Layout::LineSegment& line)
{
	Layout::LineOverlapsLine lol{ ll, line};
	// get the line intersects class to determine overlaps and split lines 
	Layout::GroupPair  parent {Layout::findContainingParent(ll, lol)};
	std::vector<Layout::BranchSplitPair> splits;
	// find all the split lines
	Layout::branchesWithOverlappingSplit(parent, lol, splits);
	NodeMap nodes;
	for ( Layout::BranchSplitPair pr : splits)
	{ 
		addSplitsToNodeMap(nodes, pr);
	}
	return  Layout::NodeMap();
}

/******************************************************************************************************************
 *    @func     addNodeMaps(NodeMap sum, NodeMap other) will add all the unique
 *    		shared pointers from new into sum.
 *    @params[in]      sum:  all the unique const Nodes sorted by uid
 *                     other:  other Node map to insert. Does not modify
 *                     NodeMap& b.
 * *************************************************************************************************************/
void Layout::addToNodeMaps(Layout::NodeMap& sum, const Layout::NodeMap& b)
{
	for ( const std::pair<Layout::uIDType, std::shared_ptr<const Layout::Node>>& nodePair: b) 
	{
		insertIntoNodeMap(sum, nodePair.second);
	}
}



/******************************************************************************************************************
 *    @func     addNodeMaps(NodeMap sum, NodeMap other) will add all the unique
 *    		shared pointers from new into sum.
 *    @params[in]      sum:  all the unique const Nodes sorted by uid
 *                     other:  other Node map to insert. Does not modify
 *                     NodeMap& b.
 * *************************************************************************************************************/
//	void addToNodeMaps(NodeMap& sum, const NodeMap& b);
void Layout::removeNTGroupFromSplitLines( Layout::GroupPair ll, Layout::GroupPair ntGroup)
{
	// get the line intersects class to determine overlaps and split lines 
	Layout::LineIntersects  line( ll, ntGroup);
	Layout::GroupPair  parent {Layout::findContainingParent(ll, line)};
	std::vector<Layout::BranchSplitPair> splits;
	// find all the split lines
	Layout::branchesWithOverlappingSplit(parent, line, splits);
	for ( Layout::BranchSplitPair pr : splits)
	{
		std::shared_ptr<const Layout::BranchNode> br { 
			std::dynamic_pointer_cast<const Layout::BranchNode>(pr.first)};
		if (br == nullptr) 
		{ 
			throw std::runtime_error("dynamic cast failed to get branch Node");
		}

		typedef std::vector<Efloat>::size_type ES;
		ES curr { static_cast<ES>( pr.second.first - br ->splits.begin())};
		ES last  { static_cast<ES>(pr.second.second - br->splits.begin())};
		for (; curr < last; ++curr)
		{
			bool success {br -> removeGroup(ntGroup.first, curr)};
			if (!success)
			{
				throw std::runtime_error("failed to remove NT Group");
			}
		}
	}
}
// creates a LineSegment from an ntGroup; a line segmenti
Layout::minMaxPr initializeLSPair ( const EVector& ll, EVector::Axis s, const EVector& size)
{
	return  (s == EVector::X) ? Layout::minMaxPr( ll.x, ll.x + size.x):
		                 Layout::minMaxPr(ll.y, ll.y + size.y);
}
Efloat  initializeLSCorner ( const EVector& ll, EVector::Axis s)
{
	return  (s == EVector::X) ?  ll.y : ll.x;
}
Efloat  initializeLSTransverseVal ( const EVector& ll, EVector::Axis s, const EVector& size)
{
	return  (s == EVector::X) ?  ll.y +  size.y: ll.x  + size.x;
}
EVector::Axis  oppositeAxis(EVector::Axis ax)
{	
	return (ax == EVector::Axis::X) ? EVector::Axis::Y : EVector::Axis::X;
}
Layout::LineSegment::LineSegment(const EVector& ll, EVector::Axis s, std::shared_ptr<const Node> ntGroup):
ax{s}, pr{initializeLSPair(ll, ax, ntGroup->size)}, 
	transverseVal {initializeLSTransverseVal( ll, ax, ntGroup -> size)} 
{}

Layout::LineSegment::LineSegment(GroupPair gpr,  SplitIt  split) : ax { oppositeAxis(gpr.first->splitDir)},
             pr{ initializeLSPair(gpr.second, ax, gpr.first -> size)}, 
	     transverseVal{ initializeLSCorner(gpr.second, ax) + *split } 
{}
       
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
			{ 
				if (Yit->second.expired()) {
					throw std::runtime_error("Expired Group in Location Map");
				}
				std::shared_ptr<const Node> cptr {Yit ->second.lock()};
				list.push_back(cptr);
			}
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
	YWidth::iterator Yit { XYit -> second.find(inNode ->size.y)};
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

	EVector childMin {minVal};
	std::vector<Efloat>::size_type indx {0};
	if ( ax == EVector::Axis::X)
	{
		std::sort(AllXMLNodes.begin(), AllXMLNodes.end(), 
				[] (const XMLNodePr& a, const XMLNodePr& b) -> bool {
				   return a.second->min().x < b.second->min().x;});
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


// copyTree should make a copy of otherNode, meaning copy values not sharing th
// same memory.
std::shared_ptr<const Layout::Node> Layout::BottomUp::copyTree( std::shared_ptr<const Layout::Node>   otherNode, const EVector& minVal, 
				std::weak_ptr< const Layout::Node> p)
{
	// copy values over
	std::shared_ptr<NodeValue> v = std::make_shared<NodeValue>(*(otherNode -> v));
	std::shared_ptr<Node> thisNode;
	std::vector<Efloat> splits { otherNode->splits};
	if ( v->terminal()) {
		// update namesFound and if there is a prior value return it
		GroupType  group{ addNodeValue(v, names)};
		// leaf nodes hold the maps of all the groups that have an
		// origin at the lower left corner
		if (splits.size() != 0) {
			throw std::runtime_error("splits should be zero");
		}
		std::shared_ptr<LeafNode> lf = std::make_shared<LeafNode>(otherNode -> size, otherNode ->splitDir, 
				std::move(splits), p,  v);
		GroupMap::const_iterator it {addToGroupMap(lf, minVal, group)};
		// this adds the node itself as the first group stored in the
		// Lower Left corner.
		Layout::InsertType val{ lf ->addGroupToXYLocMap(lf)};
		if (val!=InsertType::NewNode) {
			throw std::runtime_error("This terminal added before or failed to add");
		}
		thisNode = lf;
	}
	else{ 
		thisNode = std::make_shared<BranchNode>(otherNode ->size, otherNode ->splitDir, std::move(splits),
				    p, v);
		thisNode->v->n = 0; // reset to start counting terminals as a check.
	}
	EVector childMin {minVal};
	std::vector<Efloat>::size_type indx {0};
	if ( thisNode -> splitDir == EVector::Axis::X) {
		for (std::shared_ptr<const Node> child: otherNode ->children)
		{
			std::shared_ptr<const Node> x{ copyTree(child, childMin, thisNode) };
			thisNode->v->n += x->v->n;
			thisNode ->children.push_back( x);
			if (indx < thisNode -> splits.size()){
				childMin.x = minVal.x + thisNode->splits[indx++];
			}
		}
	}
	else {
		for (std::shared_ptr<const Node> child: otherNode ->children)
		{
			std::shared_ptr<const Node> x{ copyTree(child, childMin, thisNode) };
			thisNode->v->n += x->v->n;
			thisNode ->children.push_back( x);
			if (indx < thisNode -> splits.size()){
				childMin.y = minVal.y + thisNode->splits[indx++];
			}
		}
	}
	if (thisNode -> v ->n != otherNode -> v ->n) {
		throw std::runtime_error("Number of terminals does not match");
	}
	return thisNode;
}


bool Layout::BottomUp::checkGroupPairStorage( std::shared_ptr<const Layout::Node>   thisNode, const EVector& minVal, 
				const Layout::GroupPair& ntGroup, bool last, bool& termFound) 
{
	// copy values over
	bool valid {false};
	if (thisNode->terminal()) {
		// check leaf.
		std::shared_ptr<const LeafNode> leaf{ std::dynamic_pointer_cast<const LeafNode>(thisNode) };
		if (leaf == nullptr) {
			return false;
		}
		typedef std::list<std::shared_ptr<const Node>> LIST;
		LIST nodesFoundx{ leaf->findXYLocMap(EVector::Axis::X, ntGroup.first->size.x) };
		LIST nodesFoundy {leaf->findXYLocMap(EVector::Axis::Y, ntGroup.first->size.y)};
		SharedCompare cmp(ntGroup.first);
		bool shouldFind = !termFound && ntGroup.second == minVal; 
		LIST::iterator matchFoundx{ std::find_if(nodesFoundx.begin(), nodesFoundx.end(), cmp) };
		LIST::iterator matchFoundy{ std::find_if(nodesFoundy.begin(), nodesFoundy.end(), cmp) };
		bool valid {false};
		if (shouldFind) {
			valid = matchFoundx != nodesFoundx.end() && matchFoundy != nodesFoundy.end();
			if (!valid) {
				return valid;
			}
			matchFoundx = std::find_if(++matchFoundx, nodesFoundx.end(), cmp);
			matchFoundy = std::find_if(++matchFoundy, nodesFoundy.end(), cmp);
			valid = matchFoundx == nodesFoundx.end() && matchFoundy == nodesFoundy.end();
			if (!valid) {
				return valid;
			}
			termFound = true;
		}
		else {
			valid = matchFoundx == nodesFoundx.end() && matchFoundy == nodesFoundy.end();
			if (!valid) {
				return valid;
			}
		}
	}
	else{ 
	    valid  = NodeSplit( SplitItPair( thisNode ->splits.begin(), thisNode->splits.end()), GroupPair(thisNode, minVal), 
				    ntGroup, last);
	    if (!valid) {
		    return false;
	    }
	}
	EVector childMin {minVal};
	std::vector<Efloat>::size_type indx {0};
	if ( thisNode -> splitDir == EVector::Axis::X) {
		for (std::shared_ptr<const Node> child: thisNode ->children)
		{
			bool valid{checkGroupPairStorage(child, childMin, ntGroup, last, termFound) };
			if (!valid){
				return false;
			}
			if (indx < thisNode -> splits.size()){
				childMin.x = minVal.x + thisNode->splits[indx++];
			}
		}
	}
	else {
		for (std::shared_ptr<const Node> child: thisNode ->children)
		{
			bool valid{checkGroupPairStorage(child, childMin, ntGroup, last, termFound) };
			if (!valid){
				return false;
			}
			if (indx < thisNode -> splits.size()){
				childMin.y = minVal.y + thisNode->splits[indx++];
			}
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

Layout::GroupPair Layout::BottomUp::initializeLocationTree(const char * filename)
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

		return GroupPair(XMLNode(std::move(pr), 
					std::weak_ptr<const Node>(), 
					pr.second->min(), 0, names), pr.second ->min());
}


Layout::BottomUp::BottomUp( const char * filename): next{0}, names{}, groups{}, 
	       location{initializeLocationTree(filename)}
{
		for (unsigned n{ 1 }; n <= location.first ->v->n; ++n)
		{
			addNTGroups(n);
		};
}
Layout::BottomUp::BottomUp( const Layout::BottomUp& other): next{0}, names{}, groups{}, 
	       location{GroupPair(copyTree(other.location.first, other.location.second, std::weak_ptr<const Node>()), 
			   other.location.second) }
{
		for (unsigned n{ 1 }; n <= location.first ->v->n; ++n)
		{
			addNTGroups(n);
		};
}

Layout::GroupMap::const_iterator Layout::BottomUp::findNode(std::shared_ptr<const Node> node, bool * last) const
{
	GroupMapIt   pr { groups.equal_range(node -> v -> uid)};
	GroupMap::const_iterator next = pr.first;
	// t is the count of elements
	GroupMap::size_type t {0};
	for (; pr.first != pr.second; ++pr.first, ++t)
	{
		if (pr.first -> second.first == node) {
			++t;
			break;
		}
	}
	if (pr.first == pr.second)
	{
		pr.first == groups.end();
		*last = (t == 1);
	}
	else {
		GroupMap::const_iterator next = pr.first;
		*last = (t == 1 && ++next == pr.second);
	}
	return pr.first;
}
/***************************************************************************************************
 * @func    keyFound   checks a WeakMap to see whether a const Node is in the
 * 		map
 * @params[in] map    : a weakMap that may hold the key
 *	       ntGroup: a nonterminal group (shared ptr ) that may be in the
 *	       		map.
 * @return     NodeFound : Found, NotFound, Repeated, or Error
 * ***************************************************************************************************/
Layout::NodeFound keyFound(const Layout::WeakMap& map,  std::shared_ptr<const Layout::Node> ntGroup)
{
	Layout::WeakMapItPr mapPr {  map.equal_range(ntGroup -> v -> uid)};
	WeakCompare   cmp(ntGroup);
	Layout::WeakMapIt  foundloc { std::find_if(mapPr.first, mapPr.second, cmp)};
	Layout::NodeFound found { Layout::NodeFound::Error};
	if (foundloc == mapPr.second) 
	{
		found = Layout::NodeFound::NotFound;
	}
	else {
		foundloc = std::find_if(++foundloc, mapPr.second, cmp);
		if (foundloc == mapPr.second)
		{
			found = Layout::NodeFound::Found;
		}
		else
		{
			found = Layout::NodeFound::Repeat;
		}
	}
	return found;
}
/*************************************************************************************************************
 * @func        NodeSplit will check if the nt group really is split by the
 * 			non terminal node (ntNode). If the group is found in the
 * 			hashTable it
 * 			should be split and if it is not found  it should not be
 * 			split.
 * @params[in]  split   This is the  SplitItPair of iterators
 * 			to the Splits Vector within it. 
 * 		gpr     This is the groupPair of the BranchNode; its Node and
 * 			Node and Location.
 * 		ntGroup Here is the nonTerminal Group Node that should be found.
 * 		bool    last  true if this group is the last of its type so the
 * 		        splits should not be found regardless of whether there
 * 		        is any overlap.
 * @return      bool    true means that the splits are valid.  Valid means that
 * 			if the node was found, it is in the splits and if the
 * 			node was not found it is not in the splits.
 * @brief               This will insure that the splitGroups are found where
 * 			they should be and not found where they should not be
 * 			found.
 * ************************************************************************************************************/
bool Layout::BottomUp::NodeSplit(Layout::SplitItPair split, const Layout::GroupPair& gpr, const Layout::GroupPair& ntPr, 
		   bool last ) const
{
	bool valid {false};
	// gpr must be a branch Node
	std::shared_ptr<const BranchNode> br {std::dynamic_pointer_cast<const BranchNode>(gpr.first)};
	if (br == nullptr)
	{
		return false;
	}
	// start location of splits and maps
        const std::vector<Efloat>::const_iterator splitSt { br -> splits.begin()};
	const std::vector<WeakMap>::const_iterator  mapsSt { br -> splitGroups.begin()};
	for (SplitIt curr{split.first}; curr != split.second; ++curr)
	{
		std::vector<int>::difference_type diff { curr - splitSt};
		const WeakMap& map { *(diff + mapsSt)};
		// test if ntGroups is in the map if there look for overlaps
		// if not look for no overlaps.
		NodeFound foundState { keyFound(map, ntPr.first)};
		if (foundState == NodeFound::Error || foundState == NodeFound::Repeat){
			return false;
		}
		// if this is the last occurance then the splits should have
		// been removed already.
		if ( last ) {
			return foundState == NodeFound::NotFound;
		}
		LineSegment line (gpr, curr);
		// bool test if LineOverlapsLine
		LineOverlapsLine  ll(ntPr, line);
		// true/false whether ll finds overlaps or not
		bool llFindsOverlaps{ ll.anyOverlaps()};
		// now use LineIntersects
		LineIntersects li(gpr, ntPr);
		// true false if li finds overlaps; the branch and ntGroup
		// should overlap and the splitline should cut the ntGroup
		bool liFindsOverlaps{ li.anyOverlaps() && li(*curr)};
		if (liFindsOverlaps != llFindsOverlaps) {
			valid = false;
			break;
		}
		// valid if the node was not found and did not overlap or the
		// node was found and it did overlap.
		valid = (foundState == NodeFound::Found && llFindsOverlaps) ||
			(foundState == NodeFound::NotFound && !llFindsOverlaps);
		if ( !valid) {
			break;
		}
	}
	return valid;
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
			removeGroupPair( pr.first, RemoveType::LastSplitOnly);
		}
	}
}

Layout::GroupMap::const_iterator Layout::BottomUp::removeGroupPair( Layout::GroupMap::const_iterator it, RemoveType type)
{
	//  they have not been removed
	bool splitsRemovedPrior { type == RemoveType::LastSplitRemoved};
	if ( type == RemoveType::LastAll || type == RemoveType::LastSplitRemoved) 
	{
		nameMap::iterator  nameit = names.find( it -> second.first -> v -> name);
		if (nameit == names.end())
		{
	    		throw std::runtime_error("Name corresponding to UID not found");
		}
		names.erase(nameit);
	}
	bool termsFound{ false };
	assert( checkGroupPairStorage(location.first,
			location.second, it ->second, splitsRemovedPrior, termsFound) );
	assert(termsFound);
	assert(testAddingNodes(*this, it ->second, splitsRemovedPrior));
	// remove from all the splitlines
	EVector StartSearch = it -> second.second;
	std::shared_ptr<const LeafNode>  llcorner { findLLNode(it -> second.first , 
			                                    StartSearch,
		                            it -> second.second)};
	if (!splitsRemovedPrior) {
		removeNTGroupFromSplitLines( GroupPair( llcorner, it ->second.second), 
							it ->second);
	}
	if ( type == RemoveType::LastSplitOnly) {
		return it;
	}
	bool success = llcorner ->removeFromXYLocMap(it ->second.first);
	if (!success)
	{
	    throw std::runtime_error("failed to remove node in XY Location map");
	}
	
	return groups.erase(it);
}

Layout::GroupMap::const_iterator Layout::BottomUp::removeNode(std::shared_ptr<const Node> n, bool * singleLeft)
{
		GroupMapIt pr{ groups.equal_range(n -> v -> uid) };
		// keep track of the number of matching groups;
		if ( pr.first == pr.second) {
			throw std::runtime_error("No elements in Group map");
		}
	       	GroupMap::size_type t {0};
	        GroupMap::const_iterator lastValid{ pr.second};
		GroupMap::const_iterator next {pr.first};
		bool removedSuccess{ false};
		for (; pr.first != pr.second; ++pr.first)
		{
			// the next iterator in the series
			++next;
			if (pr.first ->second.first == n)
			{
				// This case is that the one Node left matches-
				if (t == 0 && next == pr.second) {
					pr.first = removeGroupPair(pr.first, RemoveType::LastSplitRemoved);
				}
				else {
					pr.first = removeGroupPair(pr.first, RemoveType::NotLastAll);
				}
				removedSuccess = true;
				break;
			}
			else {
				lastValid = pr.first++;
				++t;
			}
		}
		if ( !removedSuccess) {
			throw std::runtime_error("No match Found");
		}
		// check for non match
		for (; pr.first != pr.second; ++pr.first)
		{
			if (pr.first ->second.first == n)
			{
				throw std::runtime_error("Group matches two nodes");
				break;
			}
			else {
				lastValid = pr.first++;
				++t;
			}
		}
		*singleLeft = (t == 1);
		// keep the rule that the last element has no splits
		if (singleLeft) {
			removeGroupPair(lastValid, RemoveType::LastSplitOnly);
		}
		return lastValid;
}
void Layout::BottomUp::removeNodes(Layout::NodeMap& nMap)
{
	       NodeMapIt start {nMap.begin()};
	       if (start == nMap.end())
	       {
		       return;
	       }
	       uIDType index { start -> second -> v -> uid};
	       NodeMapItPr nMapPr { nMap.equal_range(index)};
	       GroupMapIt    pr { groups.equal_range(index)};
	       // t is the number of Group Pairs of this type that have not
	       // matched.
	       GroupMap::size_type t {0};
	       // n number matched
	       GroupMap::size_type n {0};
	       GroupMap::const_iterator lastValid{ pr.second};
	       GroupMap::const_iterator next {pr.first};
	       for ( ; pr.first != pr.second; )
	       {
		      next++;
		      auto lam{
				   [=](const std::pair<uIDType, std::shared_ptr<const Node>>& T) -> bool
				   {
					   return T.second == pr.first->second.first;
					} };
		       NodeMapIt foundVal { std::find_if(nMapPr.first, nMapPr.second, lam)};
		       // matchfound
		       if (foundVal != nMapPr.second){
			       if (foundVal == nMapPr.first) {
				       nMapPr.first = nMap.erase(foundVal);
			       }
			       else {
				       nMap.erase(foundVal);
			       }
			       // check for another matche that should not be
			       // there
		       	       foundVal = std::find_if(nMapPr.first, nMapPr.second, lam);
			       if (foundVal != nMapPr.second) {
				       throw std::runtime_error("Node is doubled in Node Map");
			       }
				// This case is that the one Node left matches-
				if (t == 0 && n == 0 && next == pr.second) {
					pr.first = removeGroupPair(pr.first, RemoveType::LastSplitRemoved);
				}
				else if (next != pr.second)
				{
					pr.first = removeGroupPair(pr.first, RemoveType::NotLastAll);
				}
				else {
					pr.first = removeGroupPair(pr.first, RemoveType::LastAll);
				}
				n++;
		       }
		       else{
				lastValid = pr.first++;
				++t;
				if (t > 1 && nMapPr.first == nMapPr.second){
					break;
				}
		      }
               }

	       if (nMapPr.first != nMapPr.second) {
		       throw std::runtime_error("Some Nodes not deleted");
	       }
	       if (t == 1) {
		       removeGroupPair(lastValid, RemoveType::LastSplitOnly);
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
	uIDType init {next};
	for (uIDType u = 0; u < init; u++)
	{
		GroupMapIt pr {groups.equal_range(u)};
		uIDType start{ next };
		addNTGroups(pr, EVector::Axis::X, in);
		removeSingles(start, next);
		start = next;
		addNTGroups(pr, EVector::Axis::Y, in);
		removeSingles(start, next);
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
		//bool matchx =it->second.second.x == Efloat(0, Efloat::Normal);
		//matchx = matchx && it->second.second.y == Efloat(0.0295818001, Efloat::Normal);
		// target is the LL corner of neighbor sought.
		EVector target{ it->second.second };
		if (ax == EVector::Axis::X)
		{
			target.x += it->second.first->size.x;
		}
		else {
			target.y += it->second.first->size.y;
		}
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
			std::ostringstream ss;
			ss << "Terms : " << nTerms;
			ss << "; groups : " << it->second.first->v->uid;
			ss << " & " << mneighbor->v->uid;
			ss << ((ax == EVector::Axis::X) ? " X" : " Y");
			nGroupPair NewGroupPr { makeParentGroup( children, ax, ss.str())};
			//bool matchsizex = NewGroupPr.first->size.x == Efloat(1.0, Efloat::Normal);
			//matchsizex = matchsizex && NewGroupPr.first->size.y == Efloat(0.269383729, Efloat::Normal);

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
			// current < next means this group is repeated 
			InsertType type {thisCorner -> addGroupToXYLocMap(NewGroupPr.first)};
			switch (type)
			{
				// a prior node exists at this location with the
				// same size and number of terminals
				case OldNode: // don't add and don't increment
					break;
				case NewNode:
				{
					// increment and update the names
					if (curr == next) {
						grouptype = addNodeValue(NewGroupPr.first->v, names);
					}
					addToGroupMap(NewGroupPr.first,
						NewGroupPr.second, grouptype);
					// add group to all split lines
					addNTGroupToSplitLines(GroupPair(thisCorner, it->second.second),
						NewGroupPr);
					bool termsFound{ false };
					assert( checkGroupPairStorage(location.first,
							location.second, NewGroupPr, false, termsFound) );
					assert(termsFound);
					assert(testAddingNodes(*this, NewGroupPr, false));
					break;
				}
				case NewExpired:
					if (curr == next) {
						grouptype = addNodeValue(NewGroupPr.first -> v, names);
					}
					addToGroupMap( NewGroupPr.first, 
							NewGroupPr.second, grouptype);
					throw std::runtime_error("Expired matching Group");
					break;
				default:
					throw std::runtime_error("failed to insert");
					break;
			}
		}

	}
}


/*******************************************************************************************************
 * findLLTermInBranch will find the lowerLeft terminal group in a branch;
 *
 *************************************************************************************************************/
 std::shared_ptr<const Layout::Node> findLLCornerBranch(std::shared_ptr<const Layout::Node> branch) 
{
		 if (branch ->terminal()) {
			 return branch;
		 }
		 return findLLCornerBranch( branch -> children[0]);
}

/*****************************************************************************************************************
 *  @func    testAddingNodes will check if a given GroupPair that was inserted
 *  	     is present correctly in all its locations.  For the corners.  It
 *  	     will start in the location, ll corner and search for it using its X
 *  	     and Y widths.  It checks whether the findXYLocation is working. For
 *  	     the splits and branches it finds a list of branches where it should
 *  	     be then checks each one whether it is there and wether it actually
 *  	     overlaps the split line.  This exercises LineIntersects and
 *  	     LineOverlapsLine.  
 * ***************************************************************************************************************/
bool Layout::testAddingNodes(const BottomUp&  bu, GroupPair pr, bool priorlast)
{
	bool  last {false}; 
	GroupMap::const_iterator it { bu.findNode(pr.first, &last)};
	 if (it == bu.groups.end())
	 {
		 return false;
	 }
	// test corner location;
        EVector startLocation { bu.location.second};
		std::shared_ptr<const LeafNode> llCorner{ findLLNode(bu.location.first, startLocation, pr.second) };
	// look for it using the X Width
	typedef std::list<std::shared_ptr<const Node>> LIST;
	LIST nodesFound = llCorner->findXYLocMap(EVector::Axis::X, pr.first -> size.x);
	SharedCompare cmp(pr.first);
	LIST::iterator matchFound{ std::find_if(nodesFound.begin(), nodesFound.end(), cmp) };
	if (matchFound == nodesFound.end()) {
		return false;
	}
	matchFound = std::find_if(++matchFound, nodesFound.end(), cmp);
	//should not repeat
	if (matchFound != nodesFound.end())
	{
		return false;
	}
	nodesFound = llCorner ->findXYLocMap(EVector::Axis::Y, pr.first -> size.y);
	matchFound = std::find_if(nodesFound.begin(), nodesFound.end(), cmp);
	// should be found
	if (matchFound == nodesFound.end()) {
		return false;
	}
	matchFound = std::find_if(++matchFound, nodesFound.end(), cmp);
	//should not repeat
	if (matchFound != nodesFound.end())
	{
		return false;
	}
	// get the line intersects class to determine overlaps and split lines 
	Layout::LineIntersects  line( bu.location, pr);
	Layout::GroupPair  parent {Layout::findContainingParent(bu.location, line)};
	std::vector<Layout::BranchSplitPair> splits;
	// find all the split lines
	Layout::branchesWithOverlappingSplit(parent, line, splits);
	for (BranchSplitPair oneSplit:splits)
	{
	        std::shared_ptr<const Node> llcorner {findLLCornerBranch(oneSplit.first)};
		GroupMap::const_iterator it { bu.findNode(llcorner, &last)};
		last = last && priorlast;
		 if (it == bu.groups.end()) {
			 return false;
		 }
		 // second argument is the branch in the location tree and its ll corner
	     bool found = bu.NodeSplit(oneSplit.second, 
			      Layout::GroupPair(oneSplit.first, it -> second.second), pr, last);
	     if (!found){
		     return false;
	     }
	}
	return true;
}

