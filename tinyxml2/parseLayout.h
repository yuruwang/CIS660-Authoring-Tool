#pragma
#include "efloat.h"
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <map>
#include <unordered_map>
#include <unordered_set>
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
	enum FacadeType {NT, T};
/******************************************************************************
 * @struct   NodeValue 
 * @brief    holds the elements that are common to all regions similar to this
 * 	     They are located at bottom left location 0,0
 * @notes
 * ****************************************************************************/
	struct NodeValue {
		unsigned   	 uid;  // unique id of the node
		std::string 	name;  //name of the node
		EVector         size; // holds the size of the box
		unsigned           n;  // number of terminals in this node Term = 1
		bool terminal() const;  // terminal there is only one terminal Node here.
	};
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
		Node();
		std::shared_ptr<NodeValue>  v; // the potentially repeated structure
						// stores all the information of the node
		bool terminal() const;  // use v's terminal 
		EVector::Axis splitDir; // split along x or y
		std::vector<Efloat> splits;// the location of the splits
		std::vector<std::shared_ptr<Node>>  children;
	};

/**************************************************************************************************
 * GroupPair is value pair stored in the unordered maps or the hash table 
 *
 */
	typedef unsigned uIDType; // this is the index of the datastructures
	// GroupPair: first is the shared pointer to unique groups, second is a list of
	// 		lower left locations
	typedef std::pair<std::shared_ptr<Node>, std::list<EVector>>  GroupPair;
	// GroupMap holds a hashtable of uIDTypes and GroupPairs
	typedef std::unordered_map<uIDType, GroupPair> GroupMap;
	// stores Groups indexed by their YWidth.  For the same YWidth there may be 
	// multiple groups
	typedef std::unordered_multimap<const Efloat, uIDType> YWidth;
	// first Efloat has Groups ordered by XWidth.  Given a X width, it returns the one mulimap There is only one multimap 
	//   for each YWidth.  That returns a hash table
	// of Groups ordered by XWidth;
	typedef std::unordered_map<const Efloat, YWidth>  XYWidth;
	// map of all the groups at a location
	typedef std::unordered_map< const EVector, XYWidth>  GroupLoc;
	// map of all the groups at a location
	typedef std::unordered_map<std::string, uIDType> nameMap; // holds the names of all groups
	typedef std::pair<uIDType, std::list<EVector>>  SplitGroups; // all the groups that are split by a line

/* first unsigned holds the id of the unique group, the second holds the pointer to the terminal region
 * in the lower left corner to identify the group location

	typedef std::pair<unsigned, std::weak_ptr<Node>>  GroupPairWk;
/* BranchNode holds a hash map of group ID and a list of locations of that group that are split by the 
 * splitline
 ******************************************************************************************************/
	struct BranchNode :Node {
			SplitGroups splitGroups;
	};
/********************************************************************************************************
 * LeafNode holds the leaf node
 */
/*******************************************************************************************************
 * BottomUp   Holds the data structures for the Bottom up approach.
 *
 */
	struct BottomUp{
		// parse the XML Document
		// by first opening the file
		BottomUp( const char *);
		// holds all the grouped nodes that repeat more than once.  One copy per unique ID
		// give this an index and it returns a GroupPair, a shared
		// pointer to the group and the list of locations, the lower
		// left coordinate
		GroupMap groups;
		// holds the spatial data structure for the location of the NT and terminal regions
		std::shared_ptr<Node> location;
		GroupLoc LL;  // groups that have a LowerLeft corner at a Location
		GroupLoc UR;  // groups that have an upperRight corner at a Location
	private:
		//take an XMLNodePr and generate a tree of all subnodes that
		//have this XMLNodePr as a root.
		std::shared_ptr<Node> XMLNode(XMLNodePr&& , const EVector& minVal, int level, nameMap namesFound);
		uIDType next;
		// adds a repeated Terminal to all the the maps
		void addRepeatTerminalToMaps(uIDType termId, EVector location, std::shared_ptr<NodeValue currentValue);
		// check groups for a match with currentValue and update the
		// location if it is there and add a new record to nameMap and
		// groups if it is not
		GroupMap::iterator addTerminalToGroups(EVector location, std::shared_ptr<NodeValue> currentValue, 
				     nameMap& nm);

	};
	tinyxml2::XMLElement* getElement(tinyxml2::XMLDocument*, char* input);
	
	tinyxml2::XMLNode* getMainShape(tinyxml2::XMLDocument* doc);
}
