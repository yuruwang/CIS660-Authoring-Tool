#include "ProceduralFacadeCmd.h"


#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <list>

const char *facadeNameFlag = "-n", *facadeNameLongFlag = "-facadeName";
const char *widthFlag = "-w", *widthLongFlag = "-width";
const char *heightFlag = "-h", *heightLongFlag = "-height";
const char *depthFlag = "-d", *depthLongFlag = "-depth";

ProceduralFacadeCmd::ProceduralFacadeCmd() : MPxCommand()
{
}

ProceduralFacadeCmd::~ProceduralFacadeCmd()
{
}

MStatus ProceduralFacadeCmd::doIt( const MArgList& args )
{
	MGlobal::displayInfo("running ProceduralFacadeCmd...!");

	MString facadeName;
	double width;
	double height;
	double depth;

	MArgDatabase argData(syntax(), args);
	if (argData.isFlagSet(facadeNameFlag)) {
		argData.getFlagArgument(facadeNameFlag, 0, facadeName);
	}
	if (argData.isFlagSet(widthFlag)) {
		argData.getFlagArgument(widthFlag, 0, width);
	}
	if (argData.isFlagSet(heightFlag)) {
		argData.getFlagArgument(heightFlag, 0, height);
	}
	if (argData.isFlagSet(depthFlag)) {
		argData.getFlagArgument(depthFlag, 0, depth);
	}


	MGlobal::displayInfo("facadeName = " + facadeName +  ", width = " + width +", height = " + height + ", depth = " + depth);

	Facade facade(facadeName.asChar(), vec3(width, height, depth));
	facade.expand();

	MGlobal::displayInfo("after facade.expand()...");
	MGlobal::displayInfo("start debugging....");


	// using MEL to create Instancer nodes and connet them
	MString MELCommand;
	int id = 1;
	MGlobal::displayInfo("resultTable length:" + expandResultsTable.size());
	for (auto entry : expandResultsTable) {
		string shapeName = entry.first;
		string shapePath;
		if (facade.materialTable.find(shapeName) != facade.materialTable.end()) {
			shapePath = facade.materialTable.find(shapeName)->second;
		}
		MGlobal::displayInfo("shapeName:" + MString(shapeName.c_str()) + "shapePath: " + MString(shapePath.c_str()) + ", id:" + id);


		// create nodes
		MELCommand = MString("createNode PrimitiveInstanceNode;\n")
			+ MString("setAttr -type \"string\" \"PrimitiveInstanceNode") + id + MString(".shapeName\" ") + MString(shapeName.c_str()) + MString(";")
			+ MString("instancer;\n")

			// load primitive obj
			+ MString("$nodes = `file -import -type \"OBJ\" -rnn -ignoreVersion -ra true -mergeNamespacesOnClash false -options \"mo = 1\"  -pr \"") + MString(shapePath.c_str()) + MString("\"`;")
			+ MString("select -r $nodes[4];")
			+ MString("rename ") + MString(shapeName.c_str()) + MString(";")
			+ MString("hide;")

			+ MString("connectAttr ") + MString(shapeName.c_str()) + MString(".matrix ") + MString("instancer") + id + MString(".inputHierarchy[0];\n")
			+ MString("connectAttr PrimitiveInstanceNode") + id + MString(".outPoints ") + MString("instancer") + id + MString(".inputPoints;\n");
		MGlobal::executeCommand(MELCommand);

		//MELCommand = MString("$nodes = `file -import -type \"OBJ\" -rnn -ignoreVersion -ra true -mergeNamespacesOnClash false -options \"mo = 1\"  -pr \"") + MString(shapePath.c_str()) + MString("\"`;");
		//MGlobal::executeCommand(MELCommand);
		//MELCommand = MString("print($nodes[4]);");
		//MGlobal::executeCommand(MELCommand);
		//MELCommand = MString("select -r $nodes[4];");
		//MGlobal::executeCommand(MELCommand);
		//MELCommand = MString("rename ") + MString(shapeName.c_str()) + MString(";");
		//MGlobal::executeCommand(MELCommand);


		id++;
	}

    return MStatus::kSuccess;
}

MSyntax ProceduralFacadeCmd::newSyntax() {
	MSyntax syntax;

	syntax.addFlag(facadeNameFlag, facadeNameLongFlag, MSyntax::kString);
	syntax.addFlag(widthFlag, widthLongFlag, MSyntax::kDouble);
	syntax.addFlag(heightFlag, heightLongFlag, MSyntax::kDouble);
	syntax.addFlag(depthFlag, depthLongFlag, MSyntax::kDouble);

	return syntax;

}

