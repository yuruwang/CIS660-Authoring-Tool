#pragma once

#include <unordered_map>
#include <vector>
#include <queue>   
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

	Shape(const string& name, bool isTerminal, vec3 size, vec3 position);
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

	void addChild(Shape& child);
	vector<Shape> applyTo(const Shape& shape);
	vector<Shape> splitRule(const Shape& pred, AXIS axis);
	vector<Shape> repeatRule(const Shape& pred, AXIS axis);
	vector<float> calcSplitRatio(const Shape& pred, const vector<Shape>& children, AXIS axis);
	int calcRepeatTimes(const Shape& pred, const vector<Shape>& repeatChildren, AXIS axis);

	Rule(const string& predecessor, int axisId, int ruleType, int numOfChildren);
	~Rule() {}
};

class Grammar {
public:
	string name;
	unordered_map<string, Rule> ruleTable;
	void parseGrammarFromFile(const string& filePath);
	void addRule(string line);
	Rule getRuleByShape(const Shape& shape);
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
	queue<Shape> queue;

	void loadMaterialsFromFile(const string& filePath);
	void expand();

	Facade(const string& facadeName, const vec3& defualtSize);
	~Facade() {}
};

extern unordered_map<string, vector<Shape>> expandResultsTable;
extern unordered_map<string, Rule> finalRuleTable;








