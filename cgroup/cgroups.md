# libcgroups

libcgroups offers a minimalistic implementation to access information
for Linux Resource Control Group V1 and V2.

## Limitations

V1 and V2 differs in the information they provide, so the library
(currently) won't provide any information which isn't available in
both underlying implementations. This may however change in the
future if we decide to drop support for V1 as distributions migrate
over to V2.

The cgroup for the current process gets determined during _startup_
and won't change for the lifetime of the process. That was a design
decision and could "easily" be modified (by ripping out the
singleton) logic. The reason for this is that in a typical deployment
the cgroup doesn't change so we wouldn't have to locate the current
cgroup every time. I would call it a design flaw in Linux that
`/proc/self/cgroups` is a file and not a directory where one could
have found the actual files; Instead one _could_ parse that file
and do another mapping with the information from `/proc/mounts`
and `/proc/cgroups`. Another "flaw" in the current design is
the content of `/proc/self/cgroup`. It reports the path relative
to the resource root for the current group. The problem arise
when you try to do this from within a docker container which
did an overlay mount of `/sys/fs/cgroup` so that the path in
`/proc/self/cgroups` can't be accessed from the process itself.

## How it works

During initialization the library try to determine the current
mount points for the cgroups by reading `/proc/mounts`. With
that information in place all the various mount points are
traversed and we try to look for the current pid in the
`cgroup.procs`. If the current pid is located in the file
the process lives in that control group. In V1 we need
to scan all of the mount points as different controllers
may be mounted at different locations. For V2 this is the
cgroup the process belongs to.

## Resources

 * https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v1/cgroups.html
 * https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html
 * https://www.kernel.org/doc/html/latest/scheduler/sched-bwc.html
