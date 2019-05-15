# CIS660-Authoring-Tool
Authoring tool project of CIS660
5.0 The algorithm to Parse Layouts.

Creating the algorithm to generate the grammar as stated earlier consisted of two main parts, constructing a list of all possible groups organized by location, and using that information to generate candidate split rules and associate with them a cost.   With a good estimate of the cost of a split rule, one can limit the splits explored to those that minimize cost.  It is crucial to limit the number of possible splits because for each non-terminal rectangle with three split lines, there would be 8 possible splits. The number of possible split rules grows exponentially with each subdivided rectangle.  

Here the approach we used was to develop a series of structures to retrieve all repeated nonterminal groups at any location.  We are most interested in repeated groups because those could lead to a repeat rule which would allow stretching of facades where structures would repeat. Below lists of the structures developed.

5.0.1 Data Structures.


The examples begin as xml files and TINYXML2 written by Lee Thomason was used to parse the file.  Once parsed a KD tree like structure was made that has the same divisions and children that  the xml file had.  The structure is split along either X or Y in many branches.  Nodes are branch Nodes if they have terminal nodes underneath or leafNodes if they contain a terminal location.  

The next structure is a groups structure that holds an index number for each group. This is a hash table that allows multiple entries.  Some groups are terminal having one node and others are composite having many terminal nodes.  This group structure is used to find all non-terminal groups whether repeated or not.

The next structure sorts all groups by their lower left location.  The leaf nodes in the KD tree have a map that sorts all groups by their x width and y width.  This allows for a global search for 
possible split lines.  A node of the KD tree would either split in X or Y but by sorting by the lower left corner, routines provide a way to retrieve all group nodes that are there either by x width or y width.  It may be that the best splits are not along the KD tree and this structure allows one to break up each facade rectangle along either X or Y.


The next structure stores in each branch Node of the KD tree a list of all repeated groups that are split by the split line in the KD tree.  This is needed to evaluate the cost of a split line.  For a given split line code is written to retrieve all groups split by that line.  Since the nodes have containing terminals,  This list of broken nodes establishes the cost of using a split line.  This allows the top-down algorithm to choose the split Lines that have the lowest cost and so remove the fewest repeats.  Picking low cost lines makes it most likely that a repeat rule would be used.

Debugging was done throroughly.  Structrures were written to ensure that the groupnodes are registered in the correct lower left corner and the broken by the correct split lines. After inserting a node, the entire tree is tested to see ensure that it is found in all split lines that should split it and not in others.  

Some example runs demonstrate success.  One xml File F00_test10.xml has 837 terminal groups, and creates 4417 repeated groups with 211 names. Some   group Nodes have 325 terminals inside. :w
:  


it is  With this, one can sele to create at each location in the facade a list of all possible groups  that have 



There are three functions for removing Nodes that use removeGroupPair.  Each one
of the structures preserves the order of the data structures while removing.
Order here means that if a group is repeated then if it is split by a rule it
should incur a cost.  If it is a single group it should not incur a cost to
split. If a single group is split, there is is no repeat rule that will no
longer be possible, so there should be no cost to splitting.

Singles need to also be kept track of.  is necessary to keep single groups too in both the LL Corner structure


Discussion:

high_rise:

terms: 
15.  357 repeated terminals in the xml file,  229 unique groups, 1533 groups.
     Largest group composed of 357 terminals.  First division has 4 split lines,
     findXYLocMap finds 5 groups with an X width = 0.323 as it should, the
     smallest of which has 36 terminals, largest is 357 terminals.

 all tests run, no exceptions thrown

