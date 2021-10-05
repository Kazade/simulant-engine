# IDE Setup

Code completion and navigation are a must for large projects. Since Simulant provides its entire suite of includes and libraries, there is minimal configuration needed to setup your development environment. After generating a new project with `simulant start <your project name>` you should be able to build your project successfully. In order for your local tools to be able to look up Simulant types and methods, three lines in the `CMakeLists.txt` file at the root of your project need to be changed. Update the following lines to match the configuration below:

```cmake
SET(SIMULANT_INCLUDE_FOLDER "<absolute path to project>/libraries/linux-x64-gcc/include" CACHE STRING "Specify the path to the Simulant includes")
SET(SIMULANT_LIBRARY_FOLDER "<absolute path to project>/libraries/linux-x64-gcc/lib" CACHE STRING "Specify the path to the Simulant libraries")
SET(USE_GLOBAL_SIMULANT FALSE)
```

**Note:** This will not interfere with the `simulant build|package|run` commands as the tool will automatically override your settings.

Now that `CMakeLists.txt` has been configured correctly, we can set up our development environment.

## Visual Studio Code

### Installation

[Visual Studio Code](https://code.visualstudio.com/) is a general-purpose editor created by Microsoft. It includes support for a vast array of programming languages and extensions created by the community.

After installing Visual Studio Code, you can use your terminal of choice to navigate to your Simulant project folder and run the command `code .` . This will open a new editor window with your project open.

### CMake Extension

The easiest way to enable code completion is to use the [CMake Extension](https://marketplace.visualstudio.com/items?itemName=twxs.cmake) on the VS Code Extension Marketplace. All that is needed is to install the extension and allow the CMake extension to handle IntelliSense when asked.

### Building

A new CMake icon will be added to the toolbar on the left side of your editor. Click on the icon and select the "Configure All Projects" button from the top of the tab. This will configure your CMake project and prepare your environment for building. Once completed, you should now see your project listed in the CMake window with a "Build" icon beside the name. Clicking on the "Build" icon will trigger a build.

## JetBrains CLion

### Installation

[JetBrains CLion](https://www.jetbrains.com/clion/) is a C and C++ (as well as a few other languages) IDE that uses CMake as its system for generating code completion and navigation.

JetBrains provides a 30-day free trial of CLion. After the 30 days, a [license will need to be purchased](https://www.jetbrains.com/clion/buy/#personal?billing=yearly) to use the program for longer than 30 minutes.

### Configuration

All that CLion requires is to open the `CMakeLists.txt` file in the editor and it will ask if you would like to open the file as a project. Afterwards, CLion will automatically configure your environment and begin indexing your code.

**Note:** When opening your Simulant project in CLion on Windows, make sure to open the project within the WSL2 container. Look for `\\wsl$` in the file open dialog when navigating to your `CMakeLists.txt` file.
