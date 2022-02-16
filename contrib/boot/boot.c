/* V7/x86 source code: see www.nordier.com/v7x86 for details. */
/* Copyright (c) 1999 Robert Nordier.  All rights reserved. */

#include "sys.h"
#include "pt.h"

#define KLOAD   0x10000

#define EUNKOPT  1
#define EUNKDEV  2
#define EBDUNIT  3
#define ESYNTAX  4
#define ENFOUND  5
#define EBLKCHR  6
#define EINVFMT  7
#define ELOADER  8

extern char buf1[];
extern char buf2[];
extern char buf3[];
extern daddr_t ind1[NINDIR], ind2[NINDIR];

extern int putchar();
extern int getchar();
extern int ddread();
extern dcopy();
extern exec();

#define NDEV  2
#define NOPT  2

#define cv3(p)  ((long)((unsigned char *)p)[0] |          \
                ((long)((unsigned char *)p)[1] << 010) |  \
                ((long)((unsigned char *)p)[2] << 020))

typedef unsigned size_t;

struct boot {
    int type;
    int unit;
    daddr_t boff;
    char name[128];
    int opts;
};

struct ino_info {
    unsigned mode;
    off_t size;
    daddr_t addr[NADDR];
};

static char *errmsg[] = {
    NULL,
    "Unknown option",
    "Unknown device",
    "Bad unit specifier",
    "Syntax error",
    "File not found",
    "Block or character special",
    "Invalid format",
    "Load error"
};

static char *devnm[] = {"fd", "hd"};
static char *optst = "dv";
static int drv;
static unsigned long off;

static int parse();
static getstr();
static unsigned long ptoff();
static ino_t lookup();
static ino_t fsfind();
static int load();
static int fsread();
static getii();
static getblk();
static panic();
static printf();
static int memcmp();
static char *memcpy();
static char *memset();
static int strncmp();
static char *strncpy();
static int getsect();

int
main()
{
    static struct boot bt = {1, 0, 0, "unix", 0};
    char cmd[128];
    struct ino_info ii;
    ino_t ino;
    int i, e;

    for (;;) {
        printf("\nBOOT [");
        printf("%s(%u,%D)%s", devnm[bt.type], bt.unit, bt.boff, bt.name);
        if (bt.opts) {
            printf(" -");
            for (i = 0; i < NOPT; i++)
                if (bt.opts & 1 << i)
                    putchar(optst[i]);
        }
        printf("]: ");
        getstr(cmd, sizeof(cmd));
        e = parse(&bt, cmd);
        if (e == 0) {
            drv = bt.unit;
            off = 0;
            if (bt.type != 0) {
                drv |= 0x80;
                off = ptoff();
            }
            off += bt.boff;
            ino = lookup(bt.name);
            if (ino < 2)
                e = ENFOUND;
            else {
                getii(ino, &ii);
                if ((ii.mode & IFMT) == IFDIR)
                    fsfind(&ii, NULL);
                else if ((ii.mode & IFMT) == IFREG)
                    e = load(&ii, &bt);
                else
                    e = EBLKCHR;
            }
        }
        if (e)
            printf("%s\n", errmsg[e]);
    }
}

static
getstr(str, size)
char *str;
int size;
{
    char *s;
    int c;

    s = str;
    do {
        switch (c = getchar()) {
        case 0:
            break;
        case '\b':
            if (s > str) {
                s--;
                putchar('\b');
                putchar(' ');
            } else
                c = 0;
            break;
        case '\n':
            *s = 0;
            break;
        default:
            if (s - str < size - 1)
                *s++ = c;
        }
        if (c)
            putchar(c);
    } while (c != '\n');
}

static int
parse(bt, cmd)
struct boot *bt;
char *cmd;
{
    char *p, *q;
    size_t n;
    int c, i;

    p = cmd;
    for (;;) {
        while (*p == ' ')
            p++;
        if (*p == '-') {
            for (q = ++p; (c = *q) && c != ' '; q++) {
                for (i = 0; i < NOPT; i++)
                    if (c == optst[i])
                        break;
                if (i >= NOPT)
                    return EUNKOPT;
                bt->opts ^= 1 << i;
            }
            if (q - p < 1)
                return ESYNTAX;
        } else if (*p) {
            for (q = p; *q && *q != ' ' && *q != '('; q++)
                ;
            if (*q == '(') {
                n = q - p;
                for (i = 0; i < NDEV; i++)
                    if (!strncmp(p, devnm[i], n) && devnm[i][n] == 0)
                        break;
                if (i >= NDEV)
                    return EUNKDEV;
                bt->type = i;
                p = q + 1;
                c = *p++;
                if (c < '0' || c > '7')
                    return EBDUNIT;
                bt->unit = c - '0';
                bt->boff = 0;
                c = *p++;
                if (c == ',')
                    while ((c = *p++) >= '0' && c <= '9')
                        bt->boff = bt->boff * 10 + c - '0';
                if (c != ')')
                    return ESYNTAX;
                for (q = p; *q && *q != ' '; q++)
                    ;
            }
            if ((n = q - p))
                memcpy(bt->name, p, n);
            bt->name[n] = 0;
        } else
            break;
        p = q;
    }
    return 0;
}

static ino_t
lookup(path)
char *path;
{
    char name[DIRSIZ];
    struct ino_info ii;
    char *p, *q;
    ino_t ino;
    int n;

    ino = ROOTINO;
    for (p = path; ino != 0 && *p != 0; p = q) {
        getii(ino, &ii);
        if ((ii.mode & IFMT) != IFDIR)
            return 0;
        while (*p == '/')
            p++;
        for (q = p; *q && *q != '/'; q++)
            ;
        if ((n = q - p)) {
            if (n > DIRSIZ)
                n = DIRSIZ;
            memset(name, 0, sizeof(name));
            strncpy(name, p, n);
            ino = fsfind(&ii, name);
        }
    }
    return ino;
}

static ino_t
fsfind(ii, name)
struct ino_info *ii;
char *name;
{
    struct direct *d;
    int ent, n, i;

    ent = 0;
    while ((n = fsread(ii, buf1, ent == 0)) > 0) {
        n /= sizeof(struct direct);
        if (n == 0)
            break;
        for (d = (struct direct *)buf1; n--; d++) {
            if (name == NULL) {
                if (ent != 0)
                    putchar(' ');
                for (i = 0; i < DIRSIZ && d->d_name[i] != 0; i++)
                    putchar(d->d_name[i]);
            } else if (!memcmp(name, d->d_name, DIRSIZ))
                return d->d_ino;
            ent++;
        }
    }
    if (name == NULL && ent != 0)
        putchar('\n');
    return 0;
}

static int
load(ii, bt)
struct ino_info *ii;
struct boot *bt;
{
    char *p;
    long *h, hdr[8], addr, n;
    unsigned m, u;
    int i;

    h = (long *)buf2;
    n = fsread(ii, buf2, 1);
    if (n != BSIZE || h[0] != 0410)
        return EINVFMT;
    for (i = 0; i < 8; i++)
        hdr[i] = h[i];
    printf("\ntext=%D  data=%D  bss=%D\n", hdr[1], hdr[2], hdr[3]);
    addr = KLOAD;
    u = 32;
    for (i = 1; i <= 2; i++) {
        addr = (addr + (PGSZ - 1)) & ~(PGSZ - 1);
        for (n = hdr[i]; n > 0; n -= m) {
            if (u > 0)
                m = BSIZE - u;
            else {
                m = fsread(ii, buf2, 0);
                if (m != BSIZE)
                    return ELOADER;
            }
            p = buf2 + u;
            u = 0;
            if (m > n)
                u = m = n;
            dcopy(p, addr, m);
            putchar('.');
            addr += m;
        }
    }
    putchar('\n');
    exec((long)KLOAD, bt->opts);
    return 0;
}

static int
fsread(ii, buf, new)
struct ino_info *ii;
char *buf;
int new;
{
    static off_t sz;
    static int i0, i1, i2;
    daddr_t bn;
    int n;

    if (new) {
        sz = ii->size;
        i2 = i1 = i0 = 0;
    }
    if (sz != 0) {
        if (i0 < NADDR - 3)
            bn = ii->addr[i0++];
        else {
            if (i1 < 1 || i1 > NINDIR - 1) {
                if (i1 == 0)
                    bn = ii->addr[NADDR - 3];
                else {
                    if (i2 < 1 || i2 > NINDIR - 1) {
                        if (i2 == 0)
                            getblk(ind2, ii->addr[NADDR - 2]);
                        else
                            panic("File too big");
                    }
                    bn = ind2[i2++];
                    i1 = 0;
                }
                getblk(ind1, bn);
            }
            bn = ind1[i1++];
        }
        getblk(buf, bn);
    }
    n = sz < BSIZE ? sz : BSIZE;
    sz -= n;
    return n;
}

static
getii(ino, ii)
ino_t ino;
struct ino_info *ii;
{
    struct dinode *dp;
    int i;

    getblk(buf3, itod(ino));
    dp = (struct dinode *)buf3;
    dp += itoo(ino);
    ii->mode = dp->di_mode;
    ii->size = dp->di_size;
    for (i = 0; i < NADDR; i++)
        ii->addr[i] = cv3(dp->di_addr + i * 3);
}

static unsigned long
ptoff()
{
    struct ptent *p;
    int i;

    getblk(buf1, (daddr_t)0);
    if (*(short *)(buf1 + BSIZE - 2) != PTMAGIC)
        return 0;
    for (i = 0; i <= 1; i++)
        for (p = (struct ptent *)(buf1 + PTOFF);
             p < (struct ptent *)(buf1 + BSIZE);
             p++)
            if (p->id == PTID && (p->bi == PTBOOT || i))
                return p->off;
    return 0;
}

static
getblk(buf, bn)
char *buf;
daddr_t bn;
{
    if (getsect(drv, off + bn, buf, 1))
        printf("disk error: %u\n", (unsigned)bn);
}

static
panic(msg)
char *msg;
{
    printf("** panic: %s **\n", msg);
    for (;;)
        ;
}

static
printf(fmt, x)
char *fmt;
{
    static char digits[] = "0123456789abcdef";
    unsigned *ap;
    char buf[10];
    char *s;
    long u, r;
    int c;

    ap = (unsigned *)&x;
    while ((c = *fmt++)) {
        if (c == '%') {
            c = *fmt++;
            switch (c) {
            case 'c':
                putchar(*(int *)ap++);
                continue;
            case 's':
                for (s = *(char **)ap++; *s; s++)
                    putchar(*s);
                continue;
            case 'D':
            case 'u':
            case 'x':
                if (c == 'D') {
                    u = *(long *)ap;
                    ap += sizeof(long) / sizeof(unsigned);
                } else
                    u = *(unsigned *)ap++;
                r = c == 'x' ? 16 : 10;
                s = buf;
                do
                    *s++ = digits[u % r];
                while (u /= r);
                while (--s >= buf)
                    putchar(*s);
                continue;
            }
        }
        putchar(c);
    }
}

static int 
memcmp(p1, p2, n)
char *p1;
char *p2;
size_t n;
{
    for (; n--; p1++, p2++)
        if (*p1 != *p2)
            return *(unsigned char *)p1 - *(unsigned char *)p2;
    return 0;
}

static char *
memcpy(p1, p2, n)
char *p1;
char *p2;
size_t n;
{
    char *p;

    for (p = p1; n--; p1++, p2++)
        *p1 = *p2;
    return p;
}

static char *
memset(p, c, n)
char *p;
int c;
size_t n;
{
    char *r;

    for (r = p; n--; p++)
        *p = c;
    return r;
}

static int
strncmp(s1, s2, n)
char *s1;
char *s2;
size_t n;
{
    for (; n--; s1++, s2++) {
        if (*s1 != *s2)
            return (unsigned char)*s1 - (unsigned char)*s2;
        if (*s1 == 0)
            break;
    }
    return 0;
}

static char *
strncpy(s1, s2, n)
char *s1;
char *s2;
size_t n;
{
    char *s;

    s = s1;
    while (n) {
        n--;
        if (!(*s1++ = *s2++))
            break;
    }
    while (n--)
        *s1++ = 0;
    return s;
}

static int
getsect(dn, bn, buf, n)
int dn;
daddr_t bn;
char *buf;
int n;
{
    static struct {
        short siz;
        short cnt;
        unsigned short off;
        unsigned short seg;
        unsigned long lba;
        long res;
    } dp;

    dp.siz = sizeof(dp);
    dp.cnt = n;
    dp.off = (unsigned)buf;
    dp.lba = bn;
    return ddread(dn, &dp);
}
