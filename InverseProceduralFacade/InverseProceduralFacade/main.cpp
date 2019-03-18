#include <stdio.h>
#include <iostream> 
#include "ProceduralFacade.h"

using namespace std;

int main(int argc, char **argv)
{
	Facade facade("Layout", vec3(1, 0.68371, 0.3));
	Grammar testGrammar = facade.grammar;
	facade.expand();

	return 0;
}
