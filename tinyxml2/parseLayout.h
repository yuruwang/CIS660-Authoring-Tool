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
#include <functional>
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
 * @Note     The splits now are not redundant and they hold all the position information determined recursively and 
 * 		through several additions.  This way the shared pointers to NodeValue get reused
 * ******************************************************************************/
	struct Node {
		Node(const EVector& sz, EVector::Axis sd, std::vector<Efloat>&& ss, 
				std::vector<std::shared_ptr<Node>>&& cn,
					std::shared_ptr<NodeValue> v); 
		std::shared_ptr<NodeValue>  v; // the potentially repeated structure
						// stores all the information of the node
		bool terminal() const;  // use v's terminal 
		EVector         size; // holds the size of the box
		EVector::Axis splitDir; // split along x or y
		std::vector<Efloat> splits;// the location of the splits
		std::vector<std::shared_ptr<Node>>  children;
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
	typedef std::pair<std::shared_ptr<Node>, EVector>  GroupPair;
	// List is list of Group Pairs;
	typedef std::list<GroupPair>   List;
	// ListIterator is the iterator to traverse the linked list;
	typedef  List::iterator  ListIterator;
	// GroupMap holds a hashtable of uIDTypes and a list of Group Pairs
	// Each groupPair has the same NodeValue but a different Node itself.
	typedef std::unordered_map<uIDType, List> GroupMap;
	// stores Groups indexed by their YWidth.  For the same YWidth there may be 
	// multiple groups
	typedef std::multimap<const Efloat, std::weak_ptr<Node> > YWidth;
	// first Efloat has Groups ordered by XWidth.  Given a X width, it
	// returns the one mulimap.  That  multimap in stored by ywidth.
	typedef std::map<const Efloat, YWidth>  XYWidth;
	// map of all the groups at a location
//	typedef std::unordered_map< , XYWidth>  GroupLoc;
	// map of all the groups at a location
	typedef std::unordered_map<std::string, uIDType> nameMap; // holds the names of all groups
	typedef std::pair<uIDType, std::list<EVector>>  SplitGroups; // all the groups that are split by a line
	enum GroupType {New, Existing};

/* first unsigned holds the id of the unique group, the second holds the pointer to the terminal region
 * in the lower left corner to identify the group location

	typedef std::pair<unsigned, std::weak_ptr<Node>>  GroupPairWk;
/* BranchNode holds a hash map of group ID and a list of locations of that group that are split by the 
 * splitline
 ******************************************************************************************************/
	struct BranchNode :Node {
		        BranchNode(const EVector& sz, EVector::Axis sd, std::vector<Efloat>&& ss,
					std::vector<std::shared_ptr<Node>>&& cn, 
				std::shared_ptr<NodeValue> v); 
			std::unordered_set<std::shared_ptr<Node>> splitGroups;
	};

/********************************************************************************************************
 * LeafNode holds the leaf node nodes these store all the groups that have an
 * origin in the lower left corner
 */

	struct LeafNode :Node {
		        LeafNode(const EVector& sz, EVector::Axis sd, std::vector<Efloat>&& ss,
					std::vector<std::shared_ptr<Node>>&& cn, 
				std::shared_ptr<NodeValue> v); 
			// stores all the groups organized by lower left corner
			XYWidth LL;
	};
/*******************************************************************************************************
 * BottomUp   Holds the data structures for the Bottom up approach.
 *
 */
	struct BottomUp{
		// parse the XML Document
		// by first opening the file
		BottomUp( const char *);
	private:
		// holds all the grouped nodes that repeat more than once.  One copy per unique ID
		// give this an index and it returns a GroupPair, a shared
		// pointer to the group and the list of locations, the lower
		// left coordinate
		GroupMap groups;
		// holds the spatial data structure for the location of the NT and terminal regions
		std::shared_ptr<Node> location;
		// cross reference maps names to uids
		nameMap    names;
		uIDType next;
		//take an XMLNodePr and generate a tree of all subnodes that
		//have this XMLNodePr as a root.
		std::shared_ptr<Node> XMLNode(XMLNodePr&& , const EVector& minVal, int level, nameMap& namesFound);
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
		std::list<GroupPair>::iterator addToGroupMap(std::shared_ptr<Node>, const EVector& minLocation, 
				GroupType expectedNew);
		// returns a list of Children Nodes ordered according to the splits
		// The node passed in is the Top level serializable node
		std::vector<std::shared_ptr<Node>> GetChildren(const std::vector<Efloat>& splits, 
				EVector::Axis ax, const tinyxml2::XMLNode * parent, const EVector& minVal, 
				int level, nameMap& namesFound);
/*************************************************************************************************************
 * @func  	removeSingles  removes groups that are repeated only once
 * @params 	[start, last),   First(inclusive), Last[exclusive] ID to search from
 * ************************************************************************************************************/
		void removeSingles(uIDType start, uIDType last);

	};
	tinyxml2::XMLElement* getElement(tinyxml2::XMLDocument*, char* input);
	
	tinyxml2::XMLNode* getMainShape(tinyxml2::XMLDocument* doc);
}
