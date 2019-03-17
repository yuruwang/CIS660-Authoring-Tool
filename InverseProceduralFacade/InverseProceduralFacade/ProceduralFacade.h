#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include "vec.h"

using namespace std;

class Shape {

public:
	string name;
	bool isTerminal;
	vec3 size;
	vec3 position;
	vec3 scale;
	string material;

	Shape(const string& name, bool isTerminal, vec3 size);
	~Shape() {}
};

class Rule {
public:
	typedef enum { split, repeat } RULE_TYPE;
	typedef enum { x, y } AXIS;

public:
	string predecessor;
	RULE_TYPE type;
	AXIS axis;
	int numOfChildren;
	vector<Shape> children;

	Rule(const string& predecessor, int axisId, int ruleType, int numOfChildren);
	void addChild(Shape& child);
	~Rule() {}
};

class Grammar {
public:
	string name;
	unordered_map<string, Rule> ruleTable;
	void parseGrammarFromFile(const string& filePath);
	void addRule(string line);
	Grammar(const string& name = "Layout");
	~Grammar() {}
};

class Facade {
public:
	string name;
	Grammar grammar;
	Shape axiom;
	unordered_map<string, string> materialTable;
	unordered_map<string, vector<Shape>> shapeTable;  // key: materialsName, value: list of Shapes
	vector<Shape> queue;

	void loadMaterialsFromFile(const string& filePath);
	void expand();

	Facade(const string& facadeName, const vec3& defualtSize);
	~Facade() {}
};






