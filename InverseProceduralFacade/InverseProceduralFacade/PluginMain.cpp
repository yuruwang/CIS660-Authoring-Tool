#include <maya/MPxCommand.h>
#include <maya/MFnPlugin.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MSimple.h>
#include <maya/MDoubleArray.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MDGModifier.h>
#include <maya/MPlugArray.h>
#include <maya/MVector.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MStringArray.h>
#include <list>

#include "ProceduralFacadeCmd.h"
#include "PrimitiveInstanceNode.h"

using namespace std;

//int main(int argc, char **argv)
//{
//	Facade facade("Layout", vec3(1, 0.68371, 0.3));
//	Grammar testGrammar = facade.grammar;
//	facade.expand();
//
//	return 0;
//}


MStatus initializePlugin(MObject obj)
{
	MStatus   status = MStatus::kSuccess;
	MFnPlugin plugin(obj, "MyPlugin", "1.0", "Any");

	// Register Command
	status = plugin.registerCommand("ProceduralFacadeCmd", ProceduralFacadeCmd::creator, ProceduralFacadeCmd::newSyntax);
	if (!status) {
		status.perror("registerCommand");
		return status;
	}

	status = plugin.registerNode("PrimitiveInstanceNode", PrimitiveInstanceNode::id, PrimitiveInstanceNode::creator, PrimitiveInstanceNode::initialize);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	// call initialize.mel
	char buffer[2048];
	sprintf_s(buffer, 2048, "source \"%s/createMenu\";", plugin.loadPath().asChar());
	MGlobal::executeCommand(buffer, true);

	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus   status = MStatus::kSuccess;
	MFnPlugin plugin(obj);

	status = plugin.deregisterCommand("ProceduralFacadeCmd");
	if (!status) {
		status.perror("deregisterCommand");
		return status;
	}

	status = plugin.deregisterNode(PrimitiveInstanceNode::id);
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}


