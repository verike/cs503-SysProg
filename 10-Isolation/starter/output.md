# Linux User Namespace Lab Assignment (No Root Required)

## Overview
In this lab, you will explore Linux user namespaces, which are a fundamental building block of container technology. User namespaces are the only namespace type that can be manipulated without root privileges, making them perfect for student environments. You'll learn how user namespaces provide isolation and capabilities to unprivileged users.

## Learning Objectives
- Understand the concept of Linux user namespaces
- Learn how to create and manage user namespaces without root privileges
- Experience how UID/GID mapping works in practice
- Observe how capabilities function inside user namespaces

## Prerequisites
- Access to a Linux machine (Ubuntu 20.04+ or other modern distribution)
- Unprivileged user namespace support must be enabled
- Basic command-line familiarity

## Checking System Compatibility
Before starting the lab, check if your system supports unprivileged user namespaces:

```
cat /proc/sys/kernel/unprivileged_userns_clone
```

output 
```
1
```

If this returns `1`, your system supports unprivileged user namespaces. If it returns `0`, you'll need to request your system administrator to enable this feature (or use a different system).

## Part 1: Exploring User Namespaces Basics

1. Examine your current user ID and groups:

```
id
```
Output
```
uid=1713871019(vce27) gid=1151600513(domain users) groups=1151600513(domain users),945102275(coe software solidworks student),945104461(du-students),945121613(coe software university-wide access),945192752(cci tux students),945192772(cci tux everyone),1151604605(grad),1151788533(sophosuser),1713816984(du-cci-students),1713816996(du-graduate-students)
```

2. List your current namespaces:

```
vce27@tux4:/proc/sys/kernel$ ls -la /proc/self/ns
```
Output
```
total 0
dr-x--x--x 2 vce27 domain users 0 Mar 17 01:11 .
dr-xr-xr-x 9 vce27 domain users 0 Mar 17 01:11 ..
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 cgroup -> 'cgroup:[4026531835]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 ipc -> 'ipc:[4026531839]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 mnt -> 'mnt:[4026531841]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 net -> 'net:[4026531840]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 pid -> 'pid:[4026531836]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 pid_for_children -> 'pid:[4026531836]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 time -> 'time:[4026531834]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 time_for_children -> 'time:[4026531834]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 user -> 'user:[4026531837]'
lrwxrwxrwx 1 vce27 domain users 0 Mar 17 01:11 uts -> 'uts:[4026531838]'
```

3. Create a new user namespace:

```
unshare --user bash
```
Visual change after execution: ```nobody@tux4:/proc/sys/kernel$```

4. Inside this new bash shell (in the new user namespace), check your user and group IDs:

```
id
```
Output
```
uid=65534(nobody) gid=65534(nogroup) groups=65534(nogroup)
```

Notice how your UID and GID appear as 65534 (nobody).

5. Examine the namespace identifier to confirm you're in a different user namespace:

```
ls -la /proc/self/ns/user
```
Output
```
total 0
dr-x--x--x 2 nobody nogroup 0 Mar 17 01:15 .
dr-xr-xr-x 9 nobody nogroup 0 Mar 17 01:15 ..
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 cgroup -> 'cgroup:[4026531835]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 ipc -> 'ipc:[4026531839]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 mnt -> 'mnt:[4026531841]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 net -> 'net:[4026531840]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 pid -> 'pid:[4026531836]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 pid_for_children -> 'pid:[4026531836]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 time -> 'time:[4026531834]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 time_for_children -> 'time:[4026531834]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 user -> 'user:[4026532793]'
lrwxrwxrwx 1 nobody nogroup 0 Mar 17 01:15 uts -> 'uts:[4026531838]'
```
Compare it with the value you noted earlier.

You might also try `whoami` and observe that it prints "nobody".
Output ```nobody```

6. Check your new namespace's UID mapping:

```
cat /proc/self/uid_map
cat /proc/self/gid_map
```
Note that these files are likely empty, indicating no mappings are established.
Output: These files were empty.

7. Exit the namespace:

```
exit
```

## Part 2: Creating User Namespaces with UID/GID Mappings

For this part, we'll use a simple C program to create a user namespace with custom mappings. The C program is `userns_child.c` in the starter folder

Compile and run this program:

```
make
./userns_child
```
Output
```
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ ./userns_child
Parent's PID: 142310
Parent process UID: 1000, GID: 1000
Child PID: 142311
Child process UID: 65534, GID: 65534
write: Operation not permitted
Child's PID: 142311
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ Current: =ep
Bounding set =cap_chown,cap_dac_override,cap_dac_read_search,cap_fowner,cap_fsetid,cap_kill,cap_setgid,cap_setuid,cap_setpcap,cap_linux_immutable,cap_net_bind_service,cap_net_broadcast,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_ipc_owner,cap_sys_module,cap_sys_rawio,cap_sys_chroot,cap_sys_ptrace,cap_sys_pacct,cap_sys_admin,cap_sys_boot,cap_sys_nice,cap_sys_resource,cap_sys_time,cap_sys_tty_config,cap_mknod,cap_lease,cap_audit_write,cap_audit_control,cap_setfcap,cap_mac_override,cap_mac_admin,cap_syslog,cap_wake_alarm,cap_block_suspend,cap_audit_read,cap_perfmon,cap_bpf,cap_checkpoint_restore
Ambient set =
Current IAB: 
Securebits: 00/0x0/1'b0 (no-new-privs=0)
 secure-noroot: no (unlocked)
 secure-no-suid-fixup: no (unlocked)
 secure-keep-caps: no (unlocked)
 secure-no-ambient-raise: no (unlocked)
uid=0(root) euid=0(root)
gid=65534(nogroup)
groups=65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup),65534(nogroup)
Guessed mode: HYBRID (4)
```
Observe the output and answer these questions:

- What is your UID in the new user namespace?
`0`
- What capabilities do you have in the user namespace?
```
cap_chown,cap_dac_override,cap_dac_read_search,cap_fowner,cap_fsetid,cap_kill,cap_setgid,cap_setuid,cap_setpcap,cap_linux_immutable,cap_net_bind_service,cap_net_broadcast,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_ipc_owner,cap_sys_module,cap_sys_rawio,cap_sys_chroot,cap_sys_ptrace,cap_sys_pacct,cap_sys_admin,cap_sys_boot,cap_sys_nice,cap_sys_resource,cap_sys_time,cap_sys_tty_config,cap_mknod,cap_lease,cap_audit_write,cap_audit_control,cap_setfcap,cap_mac_override,cap_mac_admin,cap_syslog,cap_wake_alarm,cap_block_suspend,cap_audit_read,cap_perfmon,cap_bpf,cap_checkpoint_restore
```
- How do the UID mappings work?
 UID mappings in Linux user namespaces allow a process inside a namespace to have different UIDs than on the host. By writing to `/proc/[pid]/uid_map`, you can map a namespace UID to a real UID. This enables unprivileged users to create isolated environments while controlling permissions and access rights.

## Part 3: Exploring Isolation with User Namespaces

Run `check_isolation.sh` in the starter folder:

```bash
chmod +x check_isolation.sh
./check_isolation.sh
```

Note the differences in the output before and after entering the user namespace.

Output
```
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ chmod +x check_isolation.sh
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ ./check_isolation.sh
*** BEFORE ENTERING USER NAMESPACE ***
=== Process Information ===
PID: 143980
UID/GID: 1000/1000
Username: verike
Groups: 1000 4 20 24 25 27 29 30 44 46 100 107

=== Namespace Information ===
user namespace: user:[4026531837]
mnt namespace: mnt:[4026532256]
pid namespace: pid:[4026532259]
net namespace: net:[4026531840]
ipc namespace: ipc:[4026532244]
uts namespace: uts:[4026532257]

=== Capability Information ===
Bounding set =cap_chown,cap_dac_override,cap_dac_read_search,cap_fowner,cap_fsetid,cap_kill,cap_setgid,cap_setuid,cap_setpcap,cap_linux_immutable,cap_net_bind_service,cap_net_broadcast,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_ipc_owner,cap_sys_module,cap_sys_rawio,cap_sys_chroot,cap_sys_ptrace,cap_sys_pacct,cap_sys_admin,cap_sys_boot,cap_sys_nice,cap_sys_resource,cap_sys_time,cap_sys_tty_config,cap_mknod,cap_lease,cap_audit_write,cap_audit_control,cap_setfcap,cap_mac_override,cap_mac_admin,cap_syslog,cap_wake_alarm,cap_block_suspend,cap_audit_read,cap_perfmon,cap_bpf,cap_checkpoint_restore

=== Process List (as seen by this process) ===
UID          PID    PPID  C STIME TTY          TIME CMD
root           1       0  0 Mar15 ?        00:00:18 /sbin/init
root           2       1  0 Mar15 ?        00:00:01 /init
root           6       2  0 Mar15 ?        00:00:00 plan9 --control-socket 7 --log-level 4 --server-fd 8 --pipe-fd 10 --log-truncate
root          51       1  0 Mar15 ?        00:00:22 /usr/lib/systemd/systemd-journald

*** AFTER ENTERING USER NAMESPACE ***
=== Process Information ===
PID: 143995
UID/GID: 65534/65534
Username: nobody
Groups: 65534

=== Namespace Information ===
user namespace: user:[4026532265]
mnt namespace: mnt:[4026532256]
pid namespace: pid:[4026532259]
net namespace: net:[4026531840]
ipc namespace: ipc:[4026532244]
uts namespace: uts:[4026532257]

=== Capability Information ===
Bounding set =cap_chown,cap_dac_override,cap_dac_read_search,cap_fowner,cap_fsetid,cap_kill,cap_setgid,cap_setuid,cap_setpcap,cap_linux_immutable,cap_net_bind_service,cap_net_broadcast,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_ipc_owner,cap_sys_module,cap_sys_rawio,cap_sys_chroot,cap_sys_ptrace,cap_sys_pacct,cap_sys_admin,cap_sys_boot,cap_sys_nice,cap_sys_resource,cap_sys_time,cap_sys_tty_config,cap_mknod,cap_lease,cap_audit_write,cap_audit_control,cap_setfcap,cap_mac_override,cap_mac_admin,cap_syslog,cap_wake_alarm,cap_block_suspend,cap_audit_read,cap_perfmon,cap_bpf,cap_checkpoint_restore

=== Process List (as seen by this process) ===
UID          PID    PPID  C STIME TTY          TIME CMD
nobody         1       0  0 Mar15 ?        00:00:18 /sbin/init
nobody         2       1  0 Mar15 ?        00:00:01 /init
nobody         6       2  0 Mar15 ?        00:00:00 plan9 --control-socket 7 --log-level 4 --server-fd 8 --pipe-fd 10 --log-truncate
nobody        51       1  0 Mar15 ?        00:00:22 /usr/lib/systemd/systemd-journald
```

## Part 4: Practical Applications - Running a Program with "Root" Privileges

1. Compile and use it to run commands as "root" (in the namespace):

```
make
./fake_root id
./fake_root bash -c "id && whoami && ls -la /proc/self/ns/"
```

Output
```
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ ./fake_root id
Parent process: UID=1000, GID=1000, PID=144342
Created child process with PID: 144343
Running as UID: 0, GID: 0 (should be 0 for both)
Successfully set hostname in new UTS namespace
uid=0(root) gid=0(root) groups=0(root),65534(nogroup)
Child exited with status: 0
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ 
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ ./fake_root bash -c "id && whoami && ls -la /proc/self/ns/"
Parent process: UID=1000, GID=1000, PID=144495
Created child process with PID: 144496
Running as UID: 0, GID: 0 (should be 0 for both)
Successfully set hostname in new UTS namespace
uid=0(root) gid=0(root) groups=0(root),65534(nogroup)
root
total 0
dr-x--x--x 2 root root 0 Mar 17 01:41 .
dr-xr-xr-x 9 root root 0 Mar 17 01:41 ..
lrwxrwxrwx 1 root root 0 Mar 17 01:41 cgroup -> 'cgroup:[4026531835]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 ipc -> 'ipc:[4026532244]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 mnt -> 'mnt:[4026532256]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 net -> 'net:[4026531840]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 pid -> 'pid:[4026532259]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 pid_for_children -> 'pid:[4026532259]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 time -> 'time:[4026531834]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 time_for_children -> 'time:[4026531834]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 user -> 'user:[4026532265]'
lrwxrwxrwx 1 root root 0 Mar 17 01:41 uts -> 'uts:[4026532266]'
Child exited with status: 0
```

Notice that inside the user namespace, your UID is 0 (root), but this is only within the namespace!

2. Try to create a file as "root" in your home directory, using your program:

```
./fake_root touch ~/root_test_file
ls -la ~/root_test_file
```

Output
```
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ ./fake_root touch ~/root_test_file
Parent process: UID=1000, GID=1000, PID=144790
Created child process with PID: 144791
Running as UID: 0, GID: 0 (should be 0 for both)
Successfully set hostname in new UTS namespace
Child exited with status: 0
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ ls -la ~/root_test_file
-rw-r--r-- 1 verike verike 0 Mar 17 01:42 /home/verike/root_test_file
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ 
```

The file is created, but notice who owns it on the real system!

## Part 5: Exploring the Limitations

1. Try to perform privileged operations in your user namespace:

```
mkdir -p /tmp/test_mount
./fake_root bash -c "mount -t tmpfs none /tmp/test_mount"
```

This will likely fail with "Operation not permitted" because of missing capabilities or namespace configurations.

Output
```
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ mkdir -p /tmp/test_mount
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ ./fake_root bash -c "mount -t tmpfs none /tmp/test_mount"
Parent process: UID=1000, GID=1000, PID=145335
Created child process with PID: 145336
Running as UID: 0, GID: 0 (should be 0 for both)
Successfully set hostname in new UTS namespace
mount: /tmp/test_mount: permission denied.
       dmesg(1) may have more information after failed mount system call.
Child exited with status: 32
```

2. Try to access network namespaces (which usually require real root):

```
./fake_root ip link add dummy0 type dummy
```

Output
```
verike@VERIKE-PC:~/cs503-SysProg/10-Isolation/directions/starter$ ./fake_root ip link add dummy0 type dummy
Parent process: UID=1000, GID=1000, PID=145456
Created child process with PID: 145457
Running as UID: 0, GID: 0 (should be 0 for both)
Successfully set hostname in new UTS namespace
RTNETLINK answers: Operation not permitted
Child exited with status: 2
```
Note and document what errors you encounter.

## Deliverables

Prepare a report in plain .txt or markdown .md containing:

1. A brief introduction to Linux user namespaces (in your own words)
2. Terminal outputs from each part of the lab
3. Answers to the following questions:
- How do user namespaces provide the illusion of having root privileges? 
`// User namespaces provide root-like privileges by mapping a non-root user to UID 0 inside thenamespace, allowing certain privileged operations without affecting the host.
`
- What is the purpose of UID/GID mapping in user namespaces?

` // UID/GID mapping ensures controlled privilege escalation, allowing processes to run as rootinside a namespace while remaining unprivileged outside.
`

- What limitations did you encounter when working with user namespaces?
` // Limitations of user namespaces include restricted access to devices, mount and networklimitations, blocked setuid binaries, and certain kernel features being unavailable.
`
- How might user namespaces be used in container technology?

` // User namespaces improve security by allowing containers to run as root inside but asunprivileged users on the host, reducing attack risks.
`
- What security implications do user namespaces have? 

` // Security implications that involve reduced risk of host privileges escalation, but potentialnew attack surfaces, improper UID/GID configurations and interactions with the other namespacesintroducting tisks.
`

- Why are other namespace types typically not available to unprivileged users?

`// Other namespaces are restricted to unprivileged users due to security risks like unauthorizednetwork access, hidden processes, and filesystem tampering.
`

4. A conclusion section with your insights and any challenges you faced.

- One of the noted issues I had, I tried to cat ./proc/sys/kernel/unprivileged_userns_clone on my WSL [Ububtu] -> Response was: "no such file or directory", then I switched to the tux machine; logged in and tried again, this time it worked.