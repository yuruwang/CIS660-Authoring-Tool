#ifndef CreateProceduralFacadeCmd_H_
#define CreateProceduralFacadeCmd_H_

#include <maya/MPxCommand.h>
#include <string>
#include "ProceduralFacade.h";
#include <maya/MFnArrayAttrsData.h>
#include <maya/MVectorArray.h>
#include <maya/MDoubleArray.h>

class ProceduralFacadeCmd : public MPxCommand
{
public:
	ProceduralFacadeCmd();
    virtual ~ProceduralFacadeCmd();
    static void* creator() { return new ProceduralFacadeCmd(); }
	static MSyntax newSyntax();
    MStatus doIt( const MArgList& args );
};

#endif