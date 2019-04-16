#include "ProceduralFacade.h"
#include <fstream>
#include <sstream>

unordered_map<string, vector<Shape>> expandResultsTable;
unordered_map<string, Rule> finalRuleTable;

Shape::Shape(const string& name, bool isTerminal, vec3 size, vec3 posistion) : name(name), isTerminal(isTerminal), size(size), position(posistion) {
	if (isTerminal) {
		//position = size / 2;  // place primitive model at the center of bounding box region
		scale = size;  // assume the primitive model is unit size
	}
}

Facade::Facade(const string& facadeName, const vec3& defualtSize): name(facadeName), 
																	axiom(Shape("building", false, defualtSize, vec3(0, 0, 0))) {
	
	// load grammar from file
	grammar = Grammar(facadeName);

	// load materials data
	string materialsFilePath = "E:\\CGGT\\CIS660\\Authoring_tool\\MayaPlugin\\CIS660-Authoring-Tool\\InverseProceduralFacade\\InverseProceduralFacade\\material\\" + facadeName + ".txt"; // place holder for now
	loadMaterialsFromFile(materialsFilePath);

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

void Facade::expand() {
	queue.push(axiom);
	
	while (queue.size() > 0) {
		Shape currShape = queue.front();
		queue.pop();

		auto ruleSearch = grammar.ruleTable.find(currShape.name);
		if (ruleSearch != grammar.ruleTable.end()) {
			Rule rule = ruleSearch->second;
			vector<Shape> succsesors = rule.applyTo(currShape);
			for (int i = 0; i < succsesors.size(); ++i) {
				Shape succsesor = succsesors[i];
				if (succsesor.isTerminal) {  // add terminal shapes to final result table
					if (shapeTable.find(succsesor.name) == shapeTable.end()) {
						vector<Shape> shapeList;
						shapeList.push_back(succsesor);
						shapeTable.insert({ succsesor.name, shapeList });
					}
					else {
						shapeTable.find(succsesor.name)->second.push_back(succsesor);

					}
				}
				else {                      // add non-terminal shape to the queue
					queue.push(succsesor);
				}
			}
		}
	}

	expandResultsTable = unordered_map<string, vector<Shape>>(shapeTable);
}

Grammar::Grammar(const string& facadeName): name(facadeName) {
	// parse grammar file
	//string filePath = "./grammar/" + facadeName + ".txt";
	string filePath = "E:\\CGGT\\CIS660\\Authoring_tool\\MayaPlugin\\CIS660-Authoring-Tool\\InverseProceduralFacade\\InverseProceduralFacade\\grammar\\" + facadeName + ".txt";
	parseGrammarFromFile(filePath);
	finalRuleTable = unordered_map<string, Rule>(ruleTable);
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

		Shape shape(shapeName, isTerminal, size, vec3(0, 0, 0));  // use placeholder for the shape pos
		rule.addChild(shape);
	}

	ruleTable.insert({ predesessor, rule });
}

Rule Grammar::getRuleByShape(const Shape& shape) {
	auto ruleSearch = ruleTable.find(shape.name);
	if (ruleSearch != ruleTable.end()) {
		return ruleSearch->second;
	}
}

Rule::Rule(const string& pred, int axisId, int ruleType, int numChildren): predecessor(pred), numOfChildren(numOfChildren){
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
}

void Rule::addChild(Shape& child) {
	children.push_back(child);
}

vector<Shape> Rule::applyTo(const Shape& shape) {
	switch (type) {
	case split:
		return splitRule(shape, axis);
	case repeat:
		return repeatRule(shape, axis);
	default:
		break;
	}
}

vector<Shape> Rule::splitRule(const Shape& pred, AXIS axis) {
	vector<Shape> successors;

	vector<float> ratios = calcSplitRatio(pred, children, axis);

	vec3 accumulate_pos = vec3(pred.position);
	accumulate_pos[axis] -= pred.size[axis] / 2;
	for (int i = 0; i < children.size(); ++i) {
		vec3 newSize = vec3(pred.size);
		newSize[axis] = newSize[axis] * ratios[i];

		vec3 newPos = vec3(accumulate_pos);
		newPos[axis] += newSize[axis] / 2;
		accumulate_pos[axis] += newSize[axis];

		Shape successor(children[i].name, children[i].isTerminal, newSize, newPos);
		successors.push_back(successor);
	}

	return successors;
}

vector<Shape> Rule::repeatRule(const Shape& pred, AXIS axis) {
	vector<Shape> successors;

	vector<float> splitRatios = calcSplitRatio(pred, children, axis);
	int repeatTimes = calcRepeatTimes(pred, children, axis);

	vec3 singleRepeatSize = vec3(pred.size);
	singleRepeatSize[axis] = pred.size[axis] / repeatTimes;

	vec3 accumulate_pos = vec3(pred.position);
	accumulate_pos[axis] -= pred.size[axis] / 2;
	for (int i = 0; i < repeatTimes; ++i) {
		for (int j = 0; j < children.size(); ++j) {
			vec3 newSize = vec3(singleRepeatSize);
			newSize[axis] = newSize[axis] * splitRatios[j];

			vec3 newPos = vec3(accumulate_pos);
			newPos[axis] += newSize[axis] / 2;
			accumulate_pos[axis] += newSize[axis];

			Shape successor = Shape(children[j].name, children[j].isTerminal, newSize, newPos);
			successors.push_back(successor);
		}
	}
	

	return successors;
}

vector<float> Rule::calcSplitRatio(const Shape& pred, const vector<Shape>& children, AXIS axis) {
	vector<float> ratios;

	// check if any children shape has repeat rule
	float split_sizes = 0.0;
	float repeat_sizes = 0.0;
	float children_sizes = 0.0;
	bool hasRepeat = false;
	for (int i = 0; i < children.size(); ++i) {
		children_sizes += children[i].size[axis];

		auto ruleSearch = finalRuleTable.find(children[i].name);
		if (ruleSearch != finalRuleTable.end() && ruleSearch->second.type == repeat) {
			hasRepeat = true;
			repeat_sizes += children[i].size[axis];
		}
		else {
			split_sizes += children[i].size[axis];
		}

		
	}
	if (children_sizes > pred.size[axis]) {
		hasRepeat = false;  // if the pre size is smaller than init size
	} 
	float repeat_region_size = pred.size[axis] - split_sizes;

	if (hasRepeat) {
		for (int i = 0; i < children.size(); ++i) {
			auto ruleSearch = finalRuleTable.find(children[i].name);
			if (ruleSearch != finalRuleTable.end() && ruleSearch->second.type == repeat) {
				float repeat_ratio = children[i].size[axis] / repeat_sizes;
				float newSize = repeat_ratio * repeat_region_size;
				ratios.push_back(newSize / pred.size[axis]);
			} else {
				ratios.push_back(children[i].size[axis] / pred.size[axis]);
			}

		}
	} 
	else {
		float sum = 0;
		for (int i = 0; i < children.size(); ++i) {
			sum += children[i].size[axis];
		}
		for (int i = 0; i < children.size(); ++i) {
			ratios.push_back(children[i].size[axis] / sum);
		}
	}



	return ratios;
}

int Rule::calcRepeatTimes(const Shape& pred, const vector<Shape>& repeatChildren, AXIS axis) {
	float sum = 0;
	for (int i = 0; i < repeatChildren.size(); ++i) {
		sum += repeatChildren[i].size[axis];
	}
	return pred.size[axis] / sum;
	
}

