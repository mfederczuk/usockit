<!--
  Copyright (c) 2022 Michael Federczuk
  SPDX-License-Identifier: CC-BY-SA-4.0
-->

# usockit #

[version_shield]: https://img.shields.io/badge/version-N%2FA_(in_development)-important.svg
![version: N/A (in development)][version_shield]

## About ##

`usockit` is a program to asynchronously send data to another program's standard input stream.

### The Problem ###

Given a program (e.g.: some server application) that reads commands from its standard input.  
It is desired that this program runs in the background, even without a controlling terminal — i.e.: `nohup <program> &`

Without a controlling terminal, the standard input can not be accessed.

Letting the program read from a regular file or from a FIFO special file (named pipe) wouldn't work since an EOF would
be sent, which would stop any command processing form the programs side.

### A Solution ###

Have an extra program (the `usockit` server) listen to a [UNIX domain socket]. Any data sent to this socket is forwarded
to the original program's standard input.  
Yet another program (the `usockit` client) would connect to the UNIX domain socket and would forward **its**
standard input to the socket.  
Since the client programs are completely decoupled from the original program, client connections are able to be closed
and reopened without influencing the original program.

![Component diagram](.github/usockit.png)

[UNIX domain socket]: <https://en.wikipedia.org/wiki/Unix_domain_socket> "Unix domain socket - Wikipedia"

## Contributing ##

Read through the [Contribution Guidelines](CONTRIBUTING.md) if you want to contribute to this project.

## License ##

**usockit** is licensed under both the [**Mozilla Public License 2.0**](LICENSES/MPL-2.0.txt) AND the
[**Apache License 2.0**](LICENSES/Apache-2.0.txt).  
For more information about copying and licensing, see the [`COPYING.txt`](COPYING.txt) file.
