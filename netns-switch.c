#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/mount.h>
#include <sched.h>

#ifndef UID
#define UID 1000
#endif

#ifndef GID
#define GID 1000
#endif

#ifndef CHECK_UID
#define CHECK_UID UID
#endif

#ifndef CHECK_GID
#define CHECK_GID GID
#endif

#ifndef NSNAME
#define NSNAME vpn
#endif
#define STR_EXPAND(x) #x
#define STR(x) STR_EXPAND(x)
#define NSNAMESTR STR(NSNAME)

// bind_etc and netns_switch copied from iproute2 lib/namespace.c

#define NETNS_DIR "/var/run/netns"
#define NETNS_ETC_DIR "/etc/netns"

static void bind_etc(const char *name)
{
	char etc_netns_path[PATH_MAX];
	char netns_name[PATH_MAX];
	char etc_name[PATH_MAX];
	struct dirent *entry;
	DIR *dir;

	snprintf(etc_netns_path, sizeof(etc_netns_path), "%s/%s", NETNS_ETC_DIR, name);
	dir = opendir(etc_netns_path);
	if (!dir)
		return;

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0)
			continue;
		if (strcmp(entry->d_name, "..") == 0)
			continue;
		snprintf(netns_name, sizeof(netns_name), "%s/%s", etc_netns_path, entry->d_name);
		snprintf(etc_name, sizeof(etc_name), "/etc/%s", entry->d_name);
		if (mount(netns_name, etc_name, "none", MS_BIND, NULL) < 0) {
			fprintf(stderr, "Bind %s -> %s failed: %s\n",
				netns_name, etc_name, strerror(errno));
		}
	}
	closedir(dir);
}

int netns_switch(const char *name)
{
	char net_path[PATH_MAX];
	int netns;

	snprintf(net_path, sizeof(net_path), "%s/%s", NETNS_DIR, name);
	netns = open(net_path, O_RDONLY | O_CLOEXEC);
	if (netns < 0) {
		fprintf(stderr, "Cannot open network namespace \"%s\": %s\n",
			name, strerror(errno));
		return -1;
	}

	if (setns(netns, CLONE_NEWNET) < 0) {
		fprintf(stderr, "setting the network namespace \"%s\" failed: %s\n",
			name, strerror(errno));
		close(netns);
		return -1;
	}
	close(netns);

	if (unshare(CLONE_NEWNS) < 0) {
		fprintf(stderr, "unshare failed: %s\n", strerror(errno));
		return -1;
	}
	/* Don't let any mounts propagate back to the parent */
	if (mount("", "/", "none", MS_SLAVE | MS_REC, NULL)) {
		fprintf(stderr, "\"mount --make-rslave /\" failed: %s\n",
			strerror(errno));
		return -1;
	}
	/* Mount a version of /sys that describes the network namespace */
	if (umount2("/sys", MNT_DETACH) < 0) {
		fprintf(stderr, "umount of /sys failed: %s\n", strerror(errno));
		return -1;
	}
	if (mount(name, "/sys", "sysfs", 0, NULL) < 0) {
		fprintf(stderr, "mount of /sys failed: %s\n", strerror(errno));
		return -1;
	}

	/* Setup bind mounts for config files in /etc */
	bind_etc(name);
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s command [argument...]\n", argv[0]);
		return -1;
	}

#ifdef DEBUG
	fprintf(stderr, "started as effective/real uid=%d/%d gid=%d/%d\n", geteuid(), getuid(), getegid(), getgid());
#endif

	if (CHECK_GID != getgid() || CHECK_UID != getuid()) {
		fprintf(stderr, "error: only uid/gid %d/%d is allowed to execute netns-switch\n", CHECK_UID, CHECK_GID);
		return -1;
	}

	int ret = netns_switch(NSNAMESTR);
	if (ret < 0) {
		return -1;
	}

	ret = setgid(GID);
	if (ret < 0) {
		fprintf(stderr, "setgid %d failed: %s\n", GID, strerror(errno));
		return -1;
	}

	ret = setuid(UID);
	if (ret < 0) {
		fprintf(stderr, "setuid %d failed: %s\n", UID, strerror(errno));
		return -1;
	}

#ifdef DEBUG
	fprintf(stderr, "running command as effective/real uid=%d/%d gid=%d/%d\n", geteuid(), getuid(), getegid(), getgid());
#endif

	ret = execvp(argv[1], argv+1);
	if (ret < 0) {
		fprintf(stderr, "execvp failed: %s\n", strerror(errno));
		return -1;
	}
}
