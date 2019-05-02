#pragma
#include "efloat.h"
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <list>
#include "tinyxml2.h"
namespace Layout {
/*****************************************************************************************************
 * 	BoundBox holds all the data retreived from the XML files 
 *        Holds the bounding box of dimensions that a Node has  minVal is the min, maxVal the max
	and Size the difference.*/

	class BoundBox
	{
	public:
		BoundBox(EVector minV, EVector maxV);
		BoundBox(const tinyxml2::XMLNode* node);
		BoundBox();
		const EVector&  min() const;
		const EVector&  max() const;
		const EVector& size() const;
	private:
		EVector minVal;
		EVector maxVal;
		EVector sizeV;
	
	};
	// holds a square Facade element.
	typedef unsigned uIDType; // this is the index of the datastructures
	enum FacadeType {NT, T};
/******************************************************************************
 * @struct   NodeValue 
 * @brief    holds the elements that are common to all regions similar to this
 * 	     They are located at bottom left location 0,0
 * @notes
 * ****************************************************************************/
	struct NodeValue {
		uIDType   	 uid;  // unique id of the node
		std::string 	name;  //name of the node
		unsigned           n;  // number of terminals in this node Term = 1
		bool terminal() const;  // terminal there is only one terminal Node here.
	};
	bool operator==(const NodeValue& a, const NodeValue& b);
	// has the node and its bounding Box
        typedef std::pair<const tinyxml2::XMLNode * , std::unique_ptr<const BoundBox> > XMLNodePr;
/*********************************************************************************
 * @struct   Node 
 * @brief    Used to traverse the Nodes as used in the map that holds Nodes to unique
 * 	     trees that are found in the facade.  The nodes are roots to trees that represent
 * 	     a unique Structure. This basic structure does not have the extras needed for the
 * 	     spatial structure but does have everything needed for the map.
 * @params   const EVector& sz   			the 3D size of the Node
 * 	     const EVector::Axis  sd   			The split axis, generally x or y
 * 	     std::vector<Efloat>&& ss  			The splits as Efloats
 * 	     std::vector<std::shared_ptr<Node>>&& cn   	The children of this node not provided to constructor
 * 	     std::weak_ptr<Node>  p    			The parent of this node
 * 	     						This parent is created
 * 	     						during the location
 * 	     						structure and is not
 * 	     						changed later even when
 * 	     						this same node is used
 * 	     						in other groups
 * 	     std::shared_ptr<NodeValue>  v		The value of this node,
 * 	     						a name, uid, and n
 * 	     						terminals within.
 * @Note     The splits now are not redundant and they hold all the position information determined recursively and 
 * 		through several additions.  This way the shared pointers to NodeValue get reused
 * ******************************************************************************/
	struct Node {
		Node(const EVector& sz, const EVector::Axis sd, std::vector<Efloat>&& ss, 
				 std::weak_ptr<const Node> p,
					std::shared_ptr<NodeValue> v); 
		std::shared_ptr<NodeValue>  v; // the potentially repeated structure
						// stores all the information of the node
		bool terminal() const;  // use v's terminal 
		const EVector         size; // holds the size of the box
		const EVector::Axis splitDir; // split along x or y
		std::vector<Efloat> splits;// the location of the splits
		std::vector<std::shared_ptr<const Node>>  children;
		std::weak_ptr<const Node>  parent;
		virtual ~Node() {}
	};



/**************************************************************************************************
 * GroupPair is value pair stored in the unordered maps or the hash table 
 *
 */
	typedef unsigned uIDType; // this is the index of the datastructures
	// GroupPair: first is the shared pointer to unique groups, or groups second is a list of
	// 		lower left locations. The Node really points to a node
	// 		in the spatial Structure.  Even if the Nodes are the
	// 		same type, their sizes could be different.  Thus there
	// 		is one pointer to each unique node.  The second
	// 		parameter is the Lower Left start location.
	typedef std::pair<std::shared_ptr<const Node>, const EVector>  GroupPair;
	// non const Node to pass partially formed nodes
	typedef std::pair<std::shared_ptr<Node>, const EVector>  nGroupPair;
	// List is list of Group Pairs;
	//typedef std::pair<std::weak_ptr<const Node>, EVector>  WeakPair;
	//childIT  is an iterator over the children of a Node
	typedef std::vector<std::shared_ptr<const Node>>::const_iterator ChildIt;
	typedef  std::pair<ChildIt, ChildIt>  ChildItPair; 
	//splitIT  is an iterator over the splits in a Node
	typedef std::vector<Efloat>::const_iterator SplitIt;
	typedef  std::pair<SplitIt, SplitIt>  SplitItPair; 
	// List is list of Group Pairs;
	//typedef std::list<GroupPair>   List;
	// ListIterator is the iterator to traverse the linked list;
	//typedef  List::iterator  ListIterator;
	// GroupMap holds a hashtable of uIDTypes and a Group Pairs
	// Each groupPair has the same NodeValue but a different Node itself.
	typedef std::unordered_multimap<uIDType, GroupPair> GroupMap;
	// WeakMap holds a hashtable of uIDTypes and a Group Pairs
	// Each groupPair has the same NodeValue but a different Node itself.
	typedef std::pair<GroupMap::const_iterator, GroupMap::const_iterator> GroupMapIt; 
	typedef std::unordered_multimap<uIDType, std::weak_ptr<const Node>> WeakMap;
	// stores Groups indexed by their YWidth.  The YWidth is already grouped
	// by Xwidth. At one location there should only be one group that has
	// the same X and Y width.  Hence this is a map, not a multimap
	typedef std::map<const Efloat, std::weak_ptr<const Node> > YWidth;
	// first Efloat has Groups ordered by XWidth.  Given a X width, it
	// returns the one mulimap.  That  multimap in stored by ywidth.
	typedef std::map<const Efloat, YWidth>  XYWidth;
	// map of all the groups at a location
	//	typedef std::unordered_map< , XYWidth>  GroupLoc;
	// map of all the groups at a location
	typedef std::unordered_map<std::string, uIDType> nameMap; // holds the names of all groups
	enum GroupType {New, Existing};
	// for add toXYLocMAP returns
	// Old :  There was an old group there already no Not inserted
	// New :  New Group inserted
	// NewExpired: Old group was expired new added over it.
	// Fail: Failed to insert Group
	enum InsertType {OldNode, NewNode, NewExpired, Fail};
	// BranchSplitPair is the branchNode and the index of the split line
	// that a nonTerminal group is split by
	typedef  std::pair<std::shared_ptr<const Node>, SplitItPair> BranchSplitPair;

/* BranchNode is a type of node used for nonTerminal regions with children.  It
 * has one new container to hold all the groups that would be broken by this
 * split.  
 * @Members   std::unordered_set<std::list<GroupPair>::iterator> 
 * 	       holds an iterator to a GroupPair ( a node and Lower Left
 * 	       location)
 ******************************************************************************************************/
	struct BranchNode :Node {
		        BranchNode(const EVector& sz, const EVector::Axis sd, std::vector<Efloat>&& ss, 
					std::weak_ptr<const Node> p,
					std::shared_ptr<NodeValue> v); 
			mutable std::vector<WeakMap> splitGroups;
			// adds a weak reference to the groupNode split by the
			// line.  LL corner is not needed so not stored
			// throws exception if failed to insert
			void addGroup(std::shared_ptr<const Node> NTGroup, SplitItPair);
			// removes a group from the branchNode
			// true if found /false if not found; throws exception
			// if expired
			bool removeGroup(std::shared_ptr<const Node> NTGroup, std::vector<int>::size_type );
	};
	// Pair of Efloats making up a min max along a dimension X or Y
	typedef   std::pair<Efloat, Efloat> minMaxPr;
	// LineSegment encodes a line segment that would be part of a split line.
	// An important observation is that the splitlines that divide groups are never
	// on the bounding box, they are always 
	struct LineSegment {
		minMaxPr  pr; // min and max values of the line say ymin, ymax.
		Efloat transverseVal; // the one transverse value say x.
		EVector::Axis  ax;  // the direction of min, max, say Y
	};
	
/* ******************************************************************************************************************
 * LineIntersects tests if a splitLine within a spatialLocation GroupPair splits a
 * combined group. The group has a non zero width and a height. It also tests whether a childNode is 
 * overlapping with the group, where the group is within the location 
 ********************************************************************************************************************/
	class LineIntersects {
		private:
			// minMap x, y dimensions of spatial Node bounding box
			minMaxPr xLocPr, yLocPr;
			// axis of GroupNode
			EVector::Axis splitDir;
			// the size of the group  Node
			// group lowerLeft;
			// minMax pairs of the group Node
			const minMaxPr xGroup, yGroup; 
			minMaxPr currentGroup;
		public:
			// initialize all variables
			LineIntersects(Layout::GroupPair& StartLocation, Layout::GroupPair& NTgroup);
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
/********************************************************************************************************
 * LeafNode holds the leaf node nodes these store all the groups that have an
 * origin in the lower left corner
 * @Members      XYWidth LL  all the groups with this terminal share the same LL
 * 			corner.
 * *****************************************************************************************************/
	struct LeafNode :Node {
		        LeafNode(const EVector& sz, const EVector::Axis sd, std::vector<Efloat>&& ss,
					std::weak_ptr<const Node> p,
					std::shared_ptr<NodeValue> v); 
			// stores all the groups organized by lower left corner
			mutable XYWidth LL;
/************************************************************************************************************
 * @func      addGroupToXYLocMap.
 * @args[in]  std::shared_ptr<const Node> groupNode
 * @return[out]  InsertType tells if there was a matching Node there or not.
 *                Whether to insert the new Node
 * @brief     will add an entry to the map or create an entry if need be.
 * 		returns true if the add was successful.  returns false if there
 * 		is already a group of the same X, Y width that has not expired
 * 		(meaning do not add).
 * 		There should only be one group with the same XYWidth.  
 * 		If the new group matches the old one, this will keep the old.
 * 		The prior does not have to be eliminated with a call because if
 * 		it is deleted in the GroupMap it will be expired here.
 * 		
 * ****************************************************************************************************/
		        InsertType addGroupToXYLocMap(std::shared_ptr<const Node> inNode) const;
/******************************************************************************************************
 * bool removeFromXYLocMap will remove a Node from the XY map.  It should be
 * found.  returns true if found and removed successfully */
		        bool removeFromXYLocMap(std::shared_ptr<const Node> inNode) const;
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
		std::list<std::shared_ptr<const Node>>  findXYLocMap(EVector::Axis ax, Efloat width, unsigned n) const;
		std::list<std::shared_ptr<const Node>>  findXYLocMap(EVector::Axis ax, Efloat width) const;
	};

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
	std::shared_ptr<const LeafNode> findLLNode(std::shared_ptr<const Node> init, EVector& ll, 
				const EVector& term);

// provide a child and an absolute LL coordinate, and this finds the lower left
// coordinate of the parent.
	void   parentLLCorner(std::shared_ptr<const  Node> parent, std::shared_ptr<const Node> child, 
				 EVector& minValueChild);
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
 
	GroupPair    findContainingParent(GroupPair currentLoc, 
				 LineIntersects& line);
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
    void branchesWithOverlappingSplit(GroupPair currentLoc, LineIntersects& line,  std::vector<BranchSplitPair>& splits );

/*******************************************************************************************************
 *   bool sameGroup(std::shared_ptr<const Node> a, std::shared_ptr<const Node>
 *   		b) determines if the groups should be considered the same.  That means
 *   		they either have one terminal and that terminal has the same UID or that
 *   		the group is composed of two groups each  have all subgroups
 *   @params[in]  std::shared_ptr<const Node> a, b  the two groups to compare
 *   @return     bool true if they are the same
 *   @brief      if both terminal compares uid for similarity, If both not
 *   		  terminal does not compare uid of the a, b but rather compares
 *   		  uid of all the children.  When groups are being formed from
 *   		  other groups, the identity of the children is already set.
 *   		  This will then determine if a new group is really like a
 *   		  previous group found or not.
 ****************************************************************************************************/
	bool sameGroup(std::shared_ptr<const Node> a, std::shared_ptr<const Node> b);
/****************************************************************************************************
 * @func   makeParentGroup makes a parent GroupPair out of a vector of children
 * 	   GroupPairs.  The Children altogether should form a rectangle
 * 	   contiguous in one dimension and all the same in the other. 
 * @param[in]  std::vector<GroupPair>  children.  the contiguous children
 *             EVector::Axis           splitdir.  The direction of the splits
 *             string                  name       optional name supplied
 * @return    a  GroupPar with all the children together
 * @brief     This creates a ValueNode for the shared group but does not add it
 * 	      to any structures.  It sets the uid = -1.  Id does count the
 * 	      terminals in the children and provide a correct count of
 * 	      terminals.  It does not alter the children in any way and does not
 * 	      reset the children's parents
 * **************************************************************************************************/
	   nGroupPair makeParentGroup(const std::vector<GroupPair> children, EVector::Axis splitDir, 
			    std::string name = "unlabeled");   
/*******************************************************************************************************
 * BottomUp   Holds the data structures for the Bottom up approach.
 *
 */
	struct BottomUp{
		// parse the XML Document
		// by first opening the file
		BottomUp( const char *);
		// this allows one to copy a BottomUp structure.  The copy does
		// not refer to any nodes in the original so original can be
		// changed or deleted and copy remains intact.  This allows one
		// to delete split groups in copy without affecting original.
		BottomUp( const BottomUp& );
		// holds the spatial data structure for the location of the NT and terminal regions
		// cross reference maps names to uids
		uIDType next;
		nameMap    names;
		// holds all the grouped nodes that repeat more than once.  One copy per unique ID
		// give this an index and it returns a GroupPair, a shared
		// pointer to the group and the list of locations, the lower
		// left coordinate
		GroupMap groups;
		// this holds the locations of the root node together with its
		// lower left location.
		GroupPair location;
	private:
		//take an XMLNodePr and generate a tree of all subnodes that
		//have this XMLNodePr as a root.
		std::shared_ptr<const Node> XMLNode(XMLNodePr&& , std::weak_ptr<const Node> p,
				const EVector& minVal, int level, nameMap& namesFound);
		// initializeLocationTree  parses the XML file, finishes
		// Location structure and established terminals in the groups
		// and the names map
		GroupPair initializeLocationTree(const char * filename);
		// produces a copy of the location with all independent
		// structures for the new BottomUp Node
		// recursively add a new Node based on the otherNode but not
		// using any structures in the otherNode.  It will link to the
		// parent
		std::shared_ptr<const Node> copyTree( std::shared_ptr<const Node>   otherNode, const EVector& minVal, 
				std::weak_ptr< const Node> p); 
///****************************************************************************************************
// *          addNodeValue will just add the NodeValue to the nameMap; if
//	    there is one there already, this returns the current one and
//	    discards the nodeValue entered.
//*   ret:    returns true if the nodeValue is new and nameMap is changed.
//*****************************************************************************************************/

		GroupType addNodeValue(std::shared_ptr<NodeValue>& nodeValue, nameMap& nm);
/***************************************************************************************************************
 *              addNodeTo GroupMap; This adds a new Node to the group Map.  It
 *              returns an iterator to the list element that holds the node. 
 **************************************************************************************************************/
		GroupMap::const_iterator addToGroupMap(std::shared_ptr<const Node>, const EVector& minLocation, 
				GroupType expectedNew);
		// returns a list of Children Nodes ordered according to the splits
		// The node passed in is the Top level serializable node
		// args are the splits, the axis (x or y) ,the xmlnode parent
		// (for the data), the nodeParent, the minVal postion, the level
		// int and the nameFound. alot of the arguments are to call
		// xmlnode on the children
		std::vector<std::shared_ptr<const Node>> GetChildren(const std::vector<Efloat>& splits, 
				const EVector::Axis ax, const tinyxml2::XMLNode * parent, 
				std::weak_ptr<const Node> nodeParent, const EVector& minVal, 
				int level, nameMap& namesFound);
/*************************************************************************************************************
 * @func  	removeSingles  removes groups that are repeated only once
 * @params 	[start, last),   First(inclusive), Last[exclusive] ID to search from
 * ************************************************************************************************************/
		void removeSingles(uIDType start, uIDType last);
/*************************************************************************************************************
 * @func      addNTGroups adds Non-terminal Groups to the GroupMap.  It goes
 * 		through every group in the groupMap and builds nodes that have nTerm terminal regions.  Each pass adds
 * 		all groups that have a total of nTerms.  
 * @params[in]  nTerms.  The number of terminals in the built up group. If a
 * 		group has 3 terminals and nTerms is 5, this would add only
 * 			groups that have 2 terminals.
 * 		GroupMapIt pr  These are the GroupPairs to look for neighboring
 * 			groups.  This is all the groups of type 7.
 * 		EVector::Axis:: ax  Y means look for groups above and X means
 * 			look for matching groups to the left.
 * @precondition:  all lower groups have already been built up.  In
 * 		example above all repeated groups of 3 and 2 are already in the
 * 		map.
 * @brief       Groups are a concatenation of two groups with the same ywidth or
 * 		xwidth.  In order to build up all the possible 3 element groups,
 * 		the 2 element groups have to be built; without that you may miss
 * 		some groups.  For higher N elements, there may be more than one 
 * 		way to build up the same group, for example it could be a 4 and
 * 		1 or a 3 and 2. We don't want multiple copies of the same group,
 * 		so whichever way is built first will get an entry in the LL
 * 		table of the terminal nodes. There is at most one group with the
 * 		same LL coordinate and same XWidth and YWidth and the same
 * 		number of terminals.  If a duplicate is created that will not be
 * 		added to the groupMap.
 *****************************************************************************************************************/
		void addNTGroups(unsigned nTerms);
		void addNTGroups(GroupMapIt pr, EVector::Axis ax, unsigned nTerms);  
	};
	tinyxml2::XMLElement* getElement(tinyxml2::XMLDocument*, char* input);
	
	tinyxml2::XMLNode* getMainShape(tinyxml2::XMLDocument* doc);
}
