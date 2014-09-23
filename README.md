# DaZeus

This is DaZeus 2.0, a modular IRC bot that utilizes out-of-process plugins. It
provides a configuration, database and IRC interface through UNIX and TCP sockets to
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
as configured.

To get a useful interface with the bot, plugins are required. The section below covers a few of the basic plugins.

# Plugins

The API that plugins use over the socket interface is described in the `doc`
directory. Bindings are available for Perl, C, C++/Qt and NodeJS at the DaZeus
project on GitHub.

**Please note:** bindings need to be installed separately!

A list of supported plugins is available at <https://github.com/dazeus>. A few commonly used examples are:

* `dazeus-plugin-channels`: manages DaZeus's presence in channels, including automatic (re)joining after (re)connecting.
* `dazeus-plugin-channellink`: a plugin for linking two IRC channels together,
  causing their message lines to be shared (one-way or two-way)
* `dazeus-plugin-commits`: a plugin for reporting Git commits on IRC
* `dazeus-plugin-factoids`: allows users to set and recall _factoids_ from DaZeus' database.
* `dazeus-plugin-karma`: A plugin for keeping karma scores
* `dazeus-plugin-mediawiki`: a plugin for integrating MediaWiki edits with IRC
* `dazeus-plugin-twitter`: a plugin for integrating with Twitter users, lists
  and search queries

# Contributing

Contributions, be it amendments to the core bot or to any plugins, are most welcome! Simply fork one or more of our repositories and submit pull requests for each of your patches.

# Acknowledgements

The lead developer for DaZeus is Sjors Gielen <dazeus@sjorsgielen.nl>. Currently it is co-authored by Ruben Nijveld <ruben@gewooniets.nl> and Aaron van Geffen <aaron@aaronweb.net>.

Several more contributors can be found in Git history.

To everyone who helped make DaZeus what it is today: thank you!