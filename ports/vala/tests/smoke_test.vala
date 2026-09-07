/* Exercises a representative slice of the generated .vapi against the
 * real library: value classes + operators, enums, multiple constructor
 * overloads, and the string-ownership convention. Not a build dependency
 * of anything -- see ports/vala/README.md for how to run this by hand,
 * and the optional `simulant-vala-check` CMake target that transpiles it
 * on every regenerate if `valac` is available.
 */
using Smlt;

int main() {
    // Plain value-like class, operators, and overloaded constructors
    // (only the zero-arg one keeps the bare class name; the others become
    // named constructors, either ".overloadN" -- numbered the same way
    // cgen.py numbers the underlying smlt_color_createN() C functions --
    // or, when tools/cgen/renames.json gives the overload a friendlier
    // name, that name instead, as it does here).
    var c1 = new Color.from_rgba(1.0f, 0.5f, 0.25f, 1.0f);
    var c2 = new Color.from_rgba(1.0f, 0.5f, 0.25f, 1.0f);
    assert(c1.get_r() == 1.0f);
    assert(c1.equals(c2));

    // Enums map straight across, including their explicit numeric values.
    assert((int) KeyboardCode.KEYBOARD_CODE_A == 0);

    // Instance methods returning a fresh owned instance (Vec3.cross()).
    var a = new Vec3.xyz(1.0f, 0.0f, 0.0f);
    var b = new Vec3.xyz(0.0f, 1.0f, 0.0f);
    var cross = a.cross(b);
    assert(cross.get_x() == 0.0f && cross.get_y() == 0.0f && cross.get_z() == 1.0f);

    // String returns are always `unowned string`, even when the
    // underlying C function hands over ownership (see the doc comment on
    // Smlt.free_string()) -- malloc()'d strings must never be bound to a
    // plain `string` local, which would trigger a hidden g_strdup() on
    // assignment and free the wrong pointer.
    var p = new Smlt.Path.from_cstr("/tmp/some/file.txt");
    unowned string name = p.name();
    assert(name == "file.txt");
    Smlt.free_string((void*) name);

    stdout.printf("OK\n");
    return 0;
}
