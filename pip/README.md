
# Simulant Tool

This package provides the `simulant` command for creating and managing projects using the Simulant game engine.

For more information on Simulant, visit [simulant-engine.appspot.com](https://simulant-engine.appspot.com)

*Note: Simulant is in rapid development and is reasonably unstable. Currently only Linux and OSX are supported as
development platforms with the SEGA Dreamcast as an additional target platform.*

*Android support is currently incomplete and non-functional.*

# Usage

To start a new project, first run:

`simulant start myproject`

This will create a template Simulant project in a folder called `myproject`. It will also download pre-compiled
libraries for your platform.

If you then change to the myproject folder, you can run your new project:

`cd myproject; simulant run --rebuild`

You should regularly stay updated with latest Simulant, and you can do this by running:

`simulant update`

Be aware though that the API is still unstable, and updating may require you to make changes to your code
or may introduce bugs. On the flipside though it will fix bugs and introduce new features and better API.

For more information run `simulant --help`.
