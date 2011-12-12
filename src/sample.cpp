#include "GL/window.h"

int main(int argc, char* argv[]) {
	GL::Window window;

	window.set_title("KGLT Sample");

	while(window.update()) {}
	return 0;
}
