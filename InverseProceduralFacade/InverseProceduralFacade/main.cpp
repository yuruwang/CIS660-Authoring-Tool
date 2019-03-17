#include <stdio.h>
#include <iostream> 
#include "ProceduralFacade.h"

using namespace std;

int main(int argc, char **argv)
{
	Facade facade("Layout");
	Grammar testGrammar = facade.grammar;

	return 0;
}
