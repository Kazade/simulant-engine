#include "GL/window.h"
#include "GL/types.h"
#include "GL/shortcuts.h"

int main(int argc, char* argv[]) {
	GL::Window window;
	window.set_title("KGLT Sample");

    window.scene().render_options.texture_enabled = false;
    window.scene().render_options.backface_culling_enabled = false;
    window.scene().render_options.point_size = 1;
    /**
        FIXME: Syntax should be:
        window.loader_for("sample.bsp").into(window.scene());
    */
    window.loader("bsp").load_into(window.scene(), "sample.bsp");
    window.scene().mesh(1).move(0.0f, 200.0f, -200.0f);

	while(window.update()) {}
	return 0;
}
