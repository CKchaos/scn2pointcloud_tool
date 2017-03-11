#include <iostream>
#include <fstream>
#include <stdlib.h>  
#include <sstream>
#include <string>


#include "R3Graphics/R3Graphics.h"
#include "fglut/fglut.h"

static R3Scene *scene = NULL;
static char *filename = NULL;



static R3Scene * ReadScene(char *filename)
{
	R3Scene *scene = new R3Scene();
	if (!scene->ReadFile(filename)) 
	{
		delete scene;
		return NULL;
	}
	return scene;
}

int main(char* argv[])
{
	strcpy(filename,argv); 
	
	scene = ReadScene(filename);
	printf("%d\n",scene->NNodes);
	return 0;
}