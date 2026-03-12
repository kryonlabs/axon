/*
 * AXON Document Parser
 * Parse various document formats (txt, md, pdf)
 */

#include <lib9.h>
#include "axon.h"

/*
 * Parse a text document
 */
char*
parse_text(const char *path)
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

    buf = emalloc9p(n + 1);
    if(readn(fd, buf, n) != n) {
        free(buf);
        close(fd);
        return nil;
    }
    close(fd);
    buf[n] = '\0';

    return buf;
}

/*
 * Parse a markdown document
 */
char*
parse_markdown(const char *path)
{
    /* For now, same as plain text */
    return parse_text(path);
}

/*
 * Parse a PDF document (placeholder)
 */
char*
parse_pdf(const char *path)
{
    /* PDF parsing requires additional library */
    fprint(2, "parse_pdf: not yet implemented\n");
    return nil;
}
