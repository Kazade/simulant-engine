# Logging

Simulant implements a named logging system (similar to that of Python). You can get access to a logger by calling `get_logger(name)`.

# Logging Macros

Simulant supplies some basic logging macros which log using the root logger and use some basic Python format()-like formatting. The macros available are:

 - `S_DEBUG(fmt, params...)`
 - `S_INFO(fmt, params...)`
 - `S_WARN(fmt, params...)`
 - `S_ERROR(fmt, params...)`
 
To access the variadic params in your format string, you must use `{X}` syntax where `X` is the 0-based index of the parameter. If your parameter is a floating point number
then you can specify the precision by using `{X:.Y}` where Y is the precision to use. 

