/*
 * AXON Ingest Tool
 * Simple command-line tool to add documents to AXON
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

char *axon_addr = "tcp!localhost!17020";

void
usage(void)
{
	fprint(2, "usage: axoningest [-a address] [-t title] file\n");
	fprint(2, "  -a address  AXON address (default: tcp!localhost!17020)\n");
	fprint(2, "  -t title    Entry title (default: filename)\n");
	exits("usage");
}

char*
read_file(char *path)
{
	int fd;
	Dir *d;
	char *buf;
	ulong n;

	fd = open(path, OREAD);
	if(fd < 0)
		return nil;

	d = dirfstat(fd);
	if(d == nil) {
		close(fd);
		return nil;
	}

	n = d->length;
	free(d);

	buf = malloc(n + 1);
	if(buf == nil) {
		close(fd);
		return nil;
	}

	if(readn(fd, buf, n) != n) {
		free(buf);
		close(fd);
		return nil;
	}
	close(fd);
	buf[n] = '\0';

	return buf;
}

char*
escape_content(char *content)
{
	char *escaped;
	int i, j, len;

	len = strlen(content);
	escaped = malloc(len * 2 + 1);
	if(escaped == nil)
		return nil;

	for(i = 0, j = 0; i < len; i++) {
		if(content[i] == '\n') {
			escaped[j++] = '\\';
			escaped[j++] = 'n';
		} else if(content[i] == '\\' || content[i] == '"') {
			escaped[j++] = '\\';
			escaped[j++] = content[i];
		} else {
			escaped[j++] = content[i];
		}
	}
	escaped[j] = '\0';

	return escaped;
}

int
add_entry(char *address, char *title, char *content)
{
	int fd;
	char *ctl_path;
	char *cmd;
	int len;

	/* Build ctl path */
	ctl_path = smprint("%s/ctl", address);
	if(ctl_path == nil)
		return -1;

	/* Open control file */
	fd = open(ctl_path, OWRITE);
	free(ctl_path);
	if(fd < 0) {
		fprint(2, "axoningest: cannot open control file: %r\n");
		return -1;
	}

	/* Build command */
	cmd = smprint("add_entry \"%s\" \"%s\"", title, content);
	if(cmd == nil) {
		close(fd);
		return -1;
	}

	len = strlen(cmd);
	if(write(fd, cmd, len) != len) {
		fprint(2, "axoningest: write failed: %r\n");
		free(cmd);
		close(fd);
		return -1;
	}

	free(cmd);
	close(fd);
	return 0;
}

void
main(int argc, char **argv)
{
	char *file;
	char *title;
	char *content;
	char *escaped;

	ARGBEGIN {
	case 'a':
		axon_addr = EARGF(usage());
		break;
	case 't':
		title = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND;

	if(argc != 1)
		usage();

	file = argv[0];

	/* Read file content */
	content = read_file(file);
	if(content == nil) {
		fprint(2, "axoningest: cannot read %s: %r\n", file);
		exits("read");
	}

	/* Use filename as title if not specified */
	if(title == nil) {
		char *slash;
		title = strdup(file);
		slash = strrchr(title, '/');
		if(slash != nil)
			title = slash + 1;

		/* Remove extension */
		{
			char *dot;
			dot = strrchr(title, '.');
			if(dot != nil && dot > title)
				*dot = '\0';
		}

		/* Replace underscores with spaces */
		{
			char *p;
			for(p = title; *p != '\0'; p++) {
				if(*p == '_' || *p == '-')
					*p = ' ';
			}
		}
	}

	/* Escape content for safe transmission */
	escaped = escape_content(content);
	if(escaped == nil) {
		fprint(2, "axoningest: memory allocation failed\n");
		free(content);
		exits("memory");
	}

	/* Add entry to AXON */
	if(add_entry(axon_addr, title, escaped) < 0) {
		free(escaped);
		free(content);
		exits("add");
	}

	print("Added entry '%s' to AXON\n", title);

	free(escaped);
	free(content);
	exits(nil);
}
