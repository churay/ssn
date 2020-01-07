# ssn: Spaceship Navigator #

An abstract proof of concept game application that uses the framework established by the
[Handmade Hero][hmh] project. This project is intended to fit in as a plugin for the
[llce][llce] application harness, which implements the aforementioned framework to alive
for loop-live code editing. In order to set this project up as a plugin, see the
[install](https://github.com/churay/ssn#install) section below.

## Demo ##

!['ssn' demo](TODO)

## Libraries ##

See the [llce](https://github.com/churay/hmp#libraries) documentation for details.

## Install ##

After following the directions for installing the [llce](https://github.com/churay/hmp#install)
project, installing `ssn` is a simple matter of setting proper symbolic links and
configuration options.

Here is a list of instructions for setting these links/options:

1. `cd /path/to/llce/`: Navigate to the base directory of the `llce` project.
1. `ln -s /path/to/ssn /path/to/llce/src`: Create a symbolic link to `ssn` in the `llce` source directory.
1. `./etc/build_linux.sh Debug Rebuild ssn`: Build the `llce` harness with `ssn` as the chosen harness code.


[pong]: https://en.wikipedia.org/wiki/Pong
[llce]: https://github.com/churay/llce
[hmh]: https://handmadehero.org/
