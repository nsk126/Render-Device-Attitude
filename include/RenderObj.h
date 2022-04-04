#include <stdio.h>
#include <GL/glew.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

void Ren();
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);