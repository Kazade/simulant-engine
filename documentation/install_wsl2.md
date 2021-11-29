# Windows WSL2 Installation

Installing Simulant within a WSL2 environment allows you to use Simulant as if you're running on Linux. 

> Note that, as far as Simulant commands are concerned, the native platform is Linux, and not Windows. If you want to run your application natively on Windows you'll need to explicitly specify 'windows' as the platform when running the `simulant` tool.

# Prerequisites

WSL2 (Window Subsystem for Linux) must be installed and configured before starting the Simulant installation process. Once WSL2 is enabled, you should install the latest Ubuntu LTS release.

The next step is to install the [Docker Desktop for Windows](https://docs.docker.com/desktop/windows/wsl/#download) with WSL support. This will run a Docker service that Simulant can use to build for non-Linux platforms (including Windows itself).

Once these Windows tools are installed (you'll need to reboot!) you can open up your Ubuntu shell from the Start Menu, and start configuring the Linux environment. 

**The remainder of this tutorial is within the Ubuntu terminal environment**

# Installing dependencies 

Inside the Ubuntu terminal, run the following command:

```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install python3-pip git python3-setuptools cmake build-essential libsdl2-dev libopenal-dev docker.io
```

# Fixing the $PATH

For some reason, Ubuntu doesn't add user-installed Python executables to the $PATH so you'll
need to do that for the `simulant` command to work once installed:

```
echo "PATH=$HOME/.local/bin:$PATH" >> ~/.profile
source ~/.profile
```

# Authorising Docker

You must be a member of the `docker` group to use the `docker` command.

```
sudo usermod -aG docker $USER
```

You'll need to log out and log back in for this to take effect (easiest to reboot to be sure!)

# Installation

Next, you need to install the `simulant-tools` pip package:

```
pip3 install -U --user git+https://gitlab.com/simulant/simulant-tools.git
```

One installation is complete, you will have access to the `simulant` command.

# Downloading the Windows SDK Docker image

Because you'll likely want to build a native Windows executable, you should go ahead and pull the latest Simulant Windows SDK Docker image. 

```
docker pull kazade/windows-sdk
```

# Creating your first project

Getting started is easy:

```
$ simulant start myprojectname
$ cd myprojectname
$ simulant run windows --rebuild
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

*This command will take a while on first run as it downloads the Docker image*

This will generate a .cdi image ready to burn. If you have 'lxdream' installed, you can run the CDI:

```
$ simulant run dreamcast
```
