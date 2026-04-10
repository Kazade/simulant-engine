# Arg Parsing

Simulant provides a simple way to pass command line options to your application. To use this functionality
you'll need to do the following:

 1. Define arguments in your `smlt::Application` subclass constructor
 2. Pass `argc`, and `argv` to `Application::run()`
 3. Access the values
 
## Defining arguments

Defining arguments is simple, here's an example from the Particles sample:

```
    args->define_arg("--stress", smlt::ARG_TYPE_BOOLEAN, "stress test the particle system");
    args->define_arg("--filename", smlt::ARG_TYPE_STRING, "display the selected file");
```

`args` is a property on the `Application`. The first param must start with a double-hyphen and 
is the argument that can be passed on the command line. The type can be: boolean, string, float or integer.

Finally the last argument is the help text displayed when someone runs your application with the `--help` flag.

## Accessing values

You can access the values using the `arg_value` template function:

```
    auto maybe_value = app->args->arg_value<bool>("stress");  // Access the value
    auto maybe_value = app->args->arg_value<bool>("stress", false); // Specify a default
```

The returned value is *not* the final value. `arg_value<T>` returns an `optional<T>`:

```
    if(maybe_value.has_value()) {
        auto final_value = maybe_value.value();
    }
```

If you passed a default to `arg_value<T>()` then `has_value()` will always return true. Otherwise
`has_value()` will return false if the user didn't pass a value for this param.

Notice that the name passed to `arg_value<T>()` doesn't need the leading hyphens. The name of this
variable can be defined by passing a custom `var_name` to `define_arg`.

