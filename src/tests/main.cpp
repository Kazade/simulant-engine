#include "GL/window.h"

int main(int argc, char* argv[]) {
	GL::Window window;
	while(window.update()) {}
	return 0;
}
