#pragma once

/* t: Normalised elapsed lifetime
 * s: Elapsed lifetime in seconds
 * initial: Baseline value (e.g. initial size, or speed or whatever)
 */

float linear_curve(float initial, float t, float s, float rate);
float bell_curve(float initial, float t, float s, float peak, float deviation);
