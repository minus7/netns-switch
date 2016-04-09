# netns-switch
*netns-switch* executes a command in a network namespace, much like [iproute2](http://www.linuxfoundation.org/collaborate/workgroups/networking/iproute2)'s `ip netns exec`, but with the difference that it executes the command as a specific, non-root user. The user, group and network namespace to execute as are compile-time options. In order to be able to switch into a network namespace, netns-switch *must run as root* which is achieved by setting the setuid flag on the executable.

***WARNING***: Previously, anyone with access to the executable was be able to run commands as the user/group specified at compile-time. netns-switch now contains a check to verify the real user and group id of the executing user.

The actual namespace switching code (`netns_switch`) is borrowed from iproute2. Like `ip netns exec`, netns-switch bind-mounts everything in `/etc/netns/NSNAME` into `/etc`. This can be used to have a specifc `resolv.conf` in the namespace.


To use this program you will need to have the network namespace configured.

## Example usage:

    netns-switch curl ip.mnus.de

# Compiling
Compile by running make, optionally change the default defines:
 - `UID`/`GID`: User/group id to execute as
 - `CHECK_UID`/`CHECK_GID`: User/group id combination allowed to run netns-switch
 - `NSNAME`: Network namespace to run in. Must exist prior to running. (default: `vpn`)

``

    make
	# or if you want to change defaults:
    make CFLAGS="-DNSNAME=vpn -DUID=1000 -DGID=1000 -DCHECK_UID=1000 -DCHECK_GID=1000"

Install per user/as necessary (requires root privileges for chown root + setuid):

    sudo make BINDIR="$HOME/bin" install

# License
Since the program contains a big chunk of code from iproute2 it is licensed under the GNU GPL Version 2.
