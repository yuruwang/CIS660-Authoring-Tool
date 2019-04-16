#include "PrimitiveInstanceNode.h"

MTypeId PrimitiveInstanceNode::id(0x10001);

MObject PrimitiveInstanceNode::shapeName;
MObject PrimitiveInstanceNode::outPoints;

PrimitiveInstanceNode::PrimitiveInstanceNode()
{
}


PrimitiveInstanceNode::~PrimitiveInstanceNode()
{

}

MStatus PrimitiveInstanceNode::initialize()
{
	MStatus status = MStatus::kSuccess;
	MFnTypedAttribute tAttr;

	shapeName = tAttr.create("shapeName", "n", MFnData::kString);
	tAttr.setWritable(true);
	tAttr.setStorable(true);

	outPoints = tAttr.create("outPoints", "o", MFnArrayAttrsData::kDynArrayAttrs);
	tAttr.setWritable(false);
	tAttr.setStorable(false);

	addAttribute(shapeName);
	addAttribute(outPoints);

	attributeAffects(shapeName, outPoints);


	return status;
}

MStatus PrimitiveInstanceNode::compute(const MPlug& plug, MDataBlock& dataBlock)
{
	MStatus status = MStatus::kSuccess;

	if (plug == outPoints) {
		// retrive parameters
		MDataHandle shapeNameData = dataBlock.inputValue(shapeName, &status);
		MString shapeName = shapeNameData.asString();

		MFnArrayAttrsData AAD;
		MObject instancesObject = AAD.create();
		MVectorArray instancesPositionArray = AAD.vectorArray("position");
		MVectorArray instancesScaleArray = AAD.vectorArray("scale");
		MDoubleArray instancesIdArray = AAD.doubleArray("id");

		if (expandResultsTable.find(shapeName.asChar()) != expandResultsTable.end()) {
			vector<Shape> shapeList = expandResultsTable.find(shapeName.asChar())->second;

			// create input arry to instancer node
			for (int i = 0; i < shapeList.size(); ++i) {
				Shape instance = shapeList[i];
				MVector pos(instance.position[0], instance.position[1], instance.position[2]);
				MVector scale(instance.scale[0], instance.scale[1], instance.scale[2]);
				instancesPositionArray.append(pos);
				instancesScaleArray.append(scale);
				instancesIdArray.append(i);
			}
		}
		else {
			MGlobal::displayInfo(MString("no corresponding shape founded in expandResultsTable"));
		}

		MDataHandle outputPointsHandle = dataBlock.outputValue(outPoints, &status);
		outputPointsHandle.set(instancesObject);

		dataBlock.setClean(plug);

	}
	else {
		return MS::kUnknownParameter;
	}
	return status;
}
