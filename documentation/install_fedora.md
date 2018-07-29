
# Fedora (Linux) Installation

The following installation process is probably very similar for other Linux distributions (such as Ubuntu, Debian etc.) the main difference should
only be the packages which must be installed.

# Prerequisites

First you need `pip` available, this may be installed already but if not run:

```
sudo dnf install python-pip
```

# Installation

Next, you need to install the `simulant-tools` pip package:

```
pip3 install -U --user git+https://github.com/Kazade/simulant-tools.git
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

# Updating to latest Simulant

Simulant is under heavy development (and unstable), it is recommended you regularly update to get the latest fixes:

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
