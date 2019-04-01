#pragma
#include "efloat.h"
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <map>
#include <unordered_map>
#include <unordered_set>
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
		Node(XMLNodePr&& , const EVector& minVal);
		Node();
		std::shared_ptr<NodeValue>  v; // the potentially repeated structure
						// stores all the information of the node
		EVector::Axis splitDir; // split along x or y
		std::vector<Efloat> splits;// the location of the splits
		std::vector<std::shared_ptr<Node>>  children;
	};

/**************************************************************************************************
 * GroupPair is value pair stored in the unordered maps or the hash table 
 *
 */
	typedef std::pair<unsigned, std::shared_ptr<Node>>  GroupPair;
	// stores Groups indexed by their YWidth.  For the same YWidth there may be 
	// multiple groups
	typedef std::unordered_multimap<const Efloat, unsigned> YWidth;
	// first Efloat has Groups ordered by XWidth.  Given a X width, it returns the one mulimap There is only one multimap 
	//   for each YWidth.  That returns a hash table
	// of Groups ordered by XWidth;
	typedef std::unordered_map<const Efloat, YWidth>  XYWidth;
	// XY values as an Efloat
	typedef std::pair<Efloat, Efloat>   XYE;
	// map of all the groups at a location
	typedef std::unordered_map< const XYE, XYWidth>  GroupLoc;
	// map of all the groups at a location

/* first unsigned holds the id of the unique group, the second holds the pointer to the terminal region
 * in the lower left corner to identify the group location

	typedef std::pair<unsigned, std::weak_ptr<Node>>  GroupPairWk;
/* BranchNode holds the data structure for the Branches that also has the groups that are cut by the branch node.
 */
	struct BranchNode :Node {
			std::unordered_set<unsigned> SplitGroups;
	};
/********************************************************************************************************
 * LeafNode holds the leaf node
 */
/*******************************************************************************************************
 * BottomUp   Holds the data structures for the Bottom up approach.
 *
 */
	struct BottomUp{
		// holds the spatial data structure for the location of the NT and terminal regions
		std::shared_ptr<Node> node;
		// holds all the grouped nodes that repeat more than once.  One copy per unique ID
		std::unordered_map<const unsigned, std::shared_ptr<Node>> Groups;
	};
	tinyxml2::XMLElement* getElement(tinyxml2::XMLDocument*, char* input);
	
	tinyxml2::XMLNode* getMainShape(tinyxml2::XMLDocument* doc);
}
