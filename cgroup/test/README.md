# Test files used by the unit tests

To make the life easier for unit testing this directory
contains files collected from various deployments.

We don't use all the files (yet?), but instead of just adding
the ones we currently use I decided to add all of them to make
the life easier _when_ we're adding support for additional
features without having to spin up a V1 and V2 instance and collect
the new files.

## V1

The `proc/mounts` file was collected from Docker Desktop on Windows
as part of MB-52817. The rest of the files comes from Ubuntu 20.04 LTS
where both `cpuacct` and `cpu` are symbolic links to a common directory.
I didn't want to mess up the git repo by using symbolic links (nor
did I want to add special hacks to the parsing) so I ended up copying
the same files into both directories.

## V2

The directory v2 contains files collected on a docker instance
running on Ubuntu 22.04 LTS.
