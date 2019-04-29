#pragma once
#include <maya/MPxNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MFnNurbsSurfaceData.h>

#include "ProceduralFacade.h";
#include <fstream>
#include <sstream>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MVectorArray.h>
#include <maya/MDoubleArray.h>



class PrimitiveInstanceNode : public MPxNode
{
public:
	PrimitiveInstanceNode();
	virtual ~PrimitiveInstanceNode();
	static void* creator() { return new PrimitiveInstanceNode(); }
	virtual MStatus compute(const MPlug& plug, MDataBlock& dataBlock);
	static MStatus initialize();

	static MTypeId id;

	static MObject shapeName;
	static MObject outPoints;


};


