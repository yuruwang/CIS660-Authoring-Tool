#include "ProceduralFacade.h"
#include <fstream>
#include <sstream>

Shape::Shape(const string& name, bool isTerminal, vec3 size) : name(name), isTerminal(isTerminal), size(size) {

}

Facade::Facade(const string& facadeName, const vec3& defualtSize): name(facadeName), 
																	axiom(Shape("building", false, defualtSize)) {
	
	// load grammar from file
	grammar = Grammar(facadeName);

	// load materials data
	string materialsFilePath = "./material/" + facadeName + ".txt"; // place holder for now
	loadMaterialsFromFile(materialsFilePath);

	// initialize axiom shape
}

void Facade::loadMaterialsFromFile(const string& filePath) {
	string line;
	ifstream file(filePath);
	if (file.is_open())
	{
		while (file.good())
		{
			getline(file, line);
			if (line.length() == 0) {
				break;
			}
			// split string into tokens
			string buf;                 // Have a buffer string
			stringstream ss(line);       // Insert the string into a stream
			vector<string> tokens; // Create vector to hold our words

			while (ss >> buf)
				tokens.push_back(buf);
			materialTable.insert({ tokens[0], tokens[1] });
		}
	}
	// for each line in p, add production
	file.close();
}


Grammar::Grammar(const string& facadeName): name(facadeName) {
	// parse grammar file
	string filePath = "./grammar/" + facadeName + ".txt";
	parseGrammarFromFile(filePath);
}

void Grammar::parseGrammarFromFile(const string& filePath) {
	string line;
	ifstream file(filePath);
	if (file.is_open())
	{
		while (file.good())
		{
			getline(file, line);
			addRule(line);
		}
	}
	// for each line in p, add production
	file.close();
}

void Grammar::addRule(string line) {
	if (line.length() == 0) {
		return;
	}
	// split string into tokens
	string buf;                 // Have a buffer string
	stringstream ss(line);       // Insert the string into a stream
	vector<string> tokens; // Create vector to hold our words

	while (ss >> buf)
		tokens.push_back(buf);
	
	// parse data in the line
	string predesessor = tokens[0];
	int axisId = stoi(tokens[1]);
	int ruleType = stoi(tokens[2]);
	int numChildren = stoi(tokens[3]);
	Rule rule(predesessor, axisId, ruleType, numChildren);

	vector<string> childrenTokens(tokens.cbegin() + 4, tokens.cbegin() + tokens.size());

	int span = 9;
	for (int i = 0; i < numChildren; ++i) {
		vec3 size;
		bool isTerminal;
		string shapeName;

		size = vec3(stof(childrenTokens[i * span]), stof(childrenTokens[i * span + 1]), stof(childrenTokens[i * span + 2]));
		isTerminal = stoi(childrenTokens[i * span + 7]);
		shapeName = childrenTokens[i * span + 8];

		Shape shape(shapeName, isTerminal, size);
		rule.addChild(shape);
	}

	ruleTable.insert({ predesessor, rule });
}

Rule::Rule(const string& pred, int axisId, int ruleType, int numChildren): predecessor(pred) {
	if (ruleType == 0) {
		type = split;
	}
	if (ruleType == 1) {
		type = repeat;
	}
	if (axisId == 0) {
		axis = x;
	}
	if (axisId == 1) {
		axis = y;
	}
	numOfChildren = numChildren;
}

void Rule::addChild(Shape& child) {
	children.push_back(child);
}


