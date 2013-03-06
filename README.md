# DaZeus

This is DaZeus 2.0, a tiny IRC bot that works with out-of-process plugins. It
provides a configuration, database and IRC interface via UNIX or TCP sockets to
its plugins.

# Installing

This application has the following dependencies:

* libircclient (tested against 1.3 and 1.6)
* libmongo-client (tested against 0.1.4)
* cmake (tested against 2.8.7 and 2.8.9)

To compile DaZeus, first checkout the Git submodules, then use CMake:

    git submodule update --init
    mkdir -p build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=.. ..
    make install

This creates a `dazeus` binary in the `bin` directory that you can run. If you
intend to run DaZeus 2.0 elsewhere, you can manually copy the binary there, or
simply not give the `-DCMAKE_INSTALL_PREFIX=..` option to CMake if you want to
install DaZeus systemwide.

# Running

When the `dazeus` binary is run, it reads the `dazeus.conf` configuration file
(unless you give the `-c` option). A sample configuration file is in the
project source root, called `dazeus.sample.conf`.

When DaZeus is started, it connects to the database and to the IRC servers as
defined in the configuration file. It also sets up the UNIX and TCP sockets
as configured. To get a useful interface with the bot, the DaZeus 1 Legacy
Daemon is required for now, and can be retrieved at
<https://github.com/dazeus/dazeus-legacyd>.

# Plugins

The API that plugins use over the socket interface is described in the `doc`
directory. Bindings are available for Perl, C, C++/Qt and NodeJS at the DaZeus
project on GitHub.

A list of supported plugins is available at <https://github.com/dazeus>, for
example:

* `dazeus-legacyd`: A Perl daemon that runs the DaZeus 1 Legacy plugins as
  DaZeus 2 plugins
* `dazeus-plugin-twitter`: A plugin for integrating with Twitter users, lists
  and search queries
* `dazeus-plugin-channellink`: A plugin for linking two IRC channels together,
  causing their message lines to be shared (one-way or two-way)
* `dazeus-plugin-commits`: A plugin for reporting Git commits on IRC
* `dazeus-plugin-mediawiki`: A plugin for integrating MediaWiki edits with IRC
* `dazeus-plugin-karma`: A plugin for keeping karma scores

# Authors

The maintainer of DaZeus is Sjors Gielen <dazeus@sjorsgielen.nl>. Also,
Ferdi van der Werf <pyromani@gmail.com> provided patches to the core bot.
