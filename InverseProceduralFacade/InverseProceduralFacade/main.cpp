#include <stdio.h>
#include <iostream> 
#include "ProceduralFacade.h"

using namespace std;

int main(int argc, char **argv)
{
	Facade facade("Layout", vec3(10, 8, 5));
	Grammar testGrammar = facade.grammar;

	return 0;
}
