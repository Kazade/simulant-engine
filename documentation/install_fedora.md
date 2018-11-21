
# Fedora (Linux) Installation

The following installation process is probably very similar for other Linux distributions (such as Ubuntu, Debian etc.) the main difference should
only be the packages which must be installed.

# Prerequisites

First you need `pip` available, this may be installed already but if not run:

```
sudo dnf install python-pip
```

You'll probably need a bunch of other dependencies too for actually compiling programs:

```
sudo cmake make gcc-c++ SDL2-devel openal-soft-devel zlib-devel mesa-libGL-devel docker
```

# Installation

Next, you need to install the `simulant-tools` pip package:

```
pip3 install -U --user git+https://gitlab.com/simulant/simulant-tools.git
```

One installation is complete, you will have access to the `simulant` command.


# Creating your first project

Getting started is easy:

```
$ simulant start myprojectname
$ cd myprojectname
$ simulant run --rebuild
```

That's it!

# Configuring your project

There are a few settings you need to configure, which are mainly used while packaging. Inside
your template project you'll find a file called `simulant.json`. If you open it in a text editor
you'll see something like this:

```
{
    "name": "myproject",
    "package": "tld.domain.Myproject",
    "executable": "myproject",
    "description": "...",
    "author": "...",
    "license": "..."
}
```

You should change the values in the quotes (the ones on the right hand side).

 - `name` - This is the name of the project, don't make it too long
 - `package` - This is a domain-name style name for the package. It's recommended you keep this
 all lower case.
 - `executable` - This is the name of the generated executable file. If you change this you 
 should edit the `add_executable` line in `CMakeLists.txt` too.
 - `description` - A brief description of your project.
 - `author` - This is you, probably in the format: `First Last <email@example.com>`
 - `license` - The license your project is released under (e.g. BSD)


# Updating to latest Simulant

Simulant is under heavy development (and unstable), it is recommended you regularly update to get the latest fixes by running this from the project folder (where `simulant.json` is):

```
$ simulant update
```

# Building for the Dreamcast

To build for the Dreamcast you must have Docker and have downloaded the Docker kazade/dreamcast-sdk container.

Then, simply:

```
$ simulant package dreamcast
```

Will generate a .cdi image ready to burn. If you have 'lxdream' installed, you can run the CDI:

```
$ simulant run dreamcast
```
