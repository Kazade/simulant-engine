
# Math Classes

Simulant ships with a library of 3D math classes, including:

 - Vec2
 - Vec3
 - Vec4
 - Quaternion
 - Mat3
 - Mat4
 - Ray
 
## Testing Equality

Testing exact equality for floating point values is problematic. For this reason Simulant follows the common practice of not exactly comparing vector components directly when checking for equality. If you do want to check for an exact match then you should use the `.equals(rhs)` methods. E.g.

```
Quaternion q1, q2;
q1 == q2;  // Approximately equal
q1.equals(q2); // Exactly equal
```
