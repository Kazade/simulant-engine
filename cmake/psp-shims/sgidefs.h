/* PSPSDK's newlib has no <sgidefs.h> (an SGI/IRIX-originated header
 * defining the classic MIPS ABI identifiers -- O32/N32/64 -- that a real
 * MIPS Linux/IRIX libc ships). libffi's src/mips/ffitarget.h needs it
 * (or these exact macros already predefined) to pick which of its
 * hand-written assembly calling-convention trampolines to use.
 *
 * The PSP toolchain builds for MIPS "eabi" (a *different*, embedded-
 * targeted ABI family that predates/sits outside this O32/N32/64
 * classification entirely -- confirmed by checking psp-gcc's own
 * predefined macros: it defines none of _MIPS_SIM/_ABIO32/etc at all).
 * This shim claims O32 so libffi's build picks its o32.S trampoline,
 * since eabi's *integer* argument-passing convention (first four args in
 * $a0-$a3) matches O32.
 *
 * CAVEAT, genuinely unverified: eabi and O32 differ in floating-point
 * register conventions and stack framing, which the o32.S trampoline
 * does not account for. This has only been confirmed to make the build
 * *compile* against a real PSPSDK toolchain (see cmake/BuildGLib.cmake) --
 * not to produce a correct calling convention at runtime. This is
 * currently harmless in practice because Simulant's generated Vala code
 * never creates a GObject GClosure/connects a signal via the generic
 * marshaller (the only things in glib/gobject that actually invoke a
 * libffi trampoline) -- if that ever changes, this needs real hardware
 * verification (or a from-scratch eabi trampoline) before trusting it,
 * particularly for any closure taking or returning a float/double.
 */
#ifndef SIMULANT_PSP_SGIDEFS_H
#define SIMULANT_PSP_SGIDEFS_H

#define _MIPS_SIM_ABI32   1
#define _MIPS_SIM_NABI32  2
#define _MIPS_SIM_ABI64   3
#define _MIPS_SIM         _MIPS_SIM_ABI32

#endif
