/* UNIX V7 source code: see /COPYRIGHT or www.tuhs.org for details. */
/* Changes: Copyright (c) 1999 Robert Nordier. All rights reserved. */

/* all from V7 header files */

#define BSIZE   512             /* size of secondary block (bytes) */
#define NINDIR  (BSIZE/sizeof(daddr_t))
#define BSHIFT  9               /* LOG2(BSIZE) */
#define NULL    0
#define ROOTINO ((ino_t)2)      /* i number of all roots */
#define DIRSIZ  14              /* max characters per directory */

/* inumber to disk address */
#define itod(x) (daddr_t)((((unsigned)x+15)>>3))

/* inumber to disk offset */
#define itoo(x) (int)((x+15)&07)

typedef long            daddr_t;
typedef unsigned short  ino_t;
typedef long            time_t;
typedef long            off_t;

struct  direct
{
        ino_t   d_ino;
        char    d_name[DIRSIZ];
};

#define NADDR   13

#define IFMT    0170000 /* type of file */
#define IFDIR   0040000 /* directory */
#define IFREG   0100000 /* regular */

struct dinode
{
        unsigned short  di_mode;        /* mode and type of file */
        short   di_nlink;       /* number of links to file */
        short   di_uid;         /* owner's user id */
        short   di_gid;         /* owner's group id */
        off_t   di_size;        /* number of bytes in file */
        char    di_addr[40];    /* disk block addresses */
        time_t  di_atime;       /* time last accessed */
        time_t  di_mtime;       /* time last modified */
        time_t  di_ctime;       /* time created */
};

#define PGSZ	4096
