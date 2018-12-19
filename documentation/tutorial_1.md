# Writing your first Simulant Application

Welcome to the first Simulant tutorial!

In this tutorial we'll set out to do the following:

1. Create a new Simulant project
2. Learn about the Simulant project structure
3. Compile and run your Simulant project
4. Learn how render pipelines work
5. Load and manipulate a 3D model

This tutorial assumes that you are running the Fedora 29 operating system (either natively, or in a Virtual Machine) and you have followed the "Prerequisites" and "Installation" sections in the [Fedora installation instructions](install_fedora.md).

If you are using another Linux distribution the instructions should be very similar.

>>> Why Fedora?
>>>
>>> Fedora is a user-friendly operating system which stays up-to-date with the latest advances
>>> in the Linux ecosystem. It has good support for Docker and cross-compilation which Simulant depends on.

## Starting your project

We're going to create a project called "monsters". Assuming you have Simulant correctly installed then this is very simple. Open a terminal (command prompt) and type the following commands and then hit enter after each:

```
mkdir ~/Projects
cd ~/Projects
simulant start monsters
```

This will create a Projects folder in your home folder, navigate to it, and then start creating your Simulant project.

The `start` command will download several things:

 - The Simulant engine compiled for multiple platforms
 - The Simulant core assets
 - The Simulant test generator
 
It may take a little while. When the process finishes you'll find a new directory called "monsters" has been created.

Now open the file manager ("Files") and navigate to the Projects folder.

![figure 1](/documentation/images/tutorial_1_1.png?raw=true)

As you can see there are a number of folders and files here, let's go through the folders first:

 - `assets` - This is where you put your game assets like textures, models, materials etc.
 - `libraries` - This is where the Simulant libraries live. If you depend on third party libraries you can also put them here.
 - `packages` - This folder is where your final distributable packages are generated.
 - `sources` - This is where the source code of your project are kept.
 - `tests` - This is where Simulant looks for unit tests for your project.
 - `tools` - This is where binary tools are found.

Now, there are a couple of files you should know about:

 - `simulant.json` - This is a configuration file for your project.
 - `CMakeLists.txt` - Simulant uses CMake to compile your project, this controls that.

It's recommended that you edit simulant.json at this point and customise it for your project.

## Building and Running your Project
 
Now, let's get something up and running! From a terminal, run the following commands:

```
cd ~/Projects/monsters
simulant build
```

This will compile your project executable. It will actually generate two binaries:

 - Your project executable
 - A `tests` executable that runs your unit tests
 
There'll be more on testing later.

Once your project has built, you can run it with the following command:

```
simulant run
```

If all goes well you should see a Simulant splash screen, followed by a blank screen. Press escape to quit.

If you have Docker installed, you can also build your project for other platforms, for example:

```
simulant build dreamcast
simulant build windows
```

And, if you want to rebuild and then run your project in a single command:

```
simulant run --rebuild
```

## Render Pipelines

Coming soon...

## Loading a 3D Model

Coming soon...




 

