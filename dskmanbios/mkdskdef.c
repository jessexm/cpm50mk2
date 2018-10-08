/*
    mkdiskdef.c -- CP/M disk definition utility

    Based on:

    CP/M 2.0 disk re-definition library

    Copyright (c) 1979
    Digital Research
    Box 579
    Pacific Grove, CA
    93950

*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

int als[32];
int css[32];

void dskhdr(int dn)
{
    fprintf(stdout, "dpe%d:   .dw     xlt%d, 0, 0, 0, dirbuf, dpb%d, csv%d, alv%d\n", dn, dn, dn, dn, dn);
}

void disks(int nd)
{
    int i;

    fprintf(stdout, "dpbase  .equ    .\n");

    for (i = 0; i < nd; i++) {
        dskhdr(i);
    }
}

void dpbhdr(int dn)
{
    fprintf(stdout, "dpb%d    .equ    .\n", dn);
}

int gcd(int a, int b)
{
    int r;

    while (b > 0) {
        r = a % b;
        a = b;
        b = r;
    }

    return a;
}

void diskdef(int dn, int fsc, int lsc, int skf, int bls, int dks, int dir, int cks, int ofs, int f16k)
{
    int i;
    int secmax, sectors;
    int blkval, blkshf, blkmsk;
    int extmsk;
    int dirrem, dirbks, dirblk;

    if (lsc == 0) {
        fprintf(stdout, "dpb%d    .equ    dpb%d\n", dn, fsc);
        als[dn] = als[fsc];
        css[dn] = css[fsc];
        fprintf(stdout, "xlt%d    .equ    xlt%d\n", dn, fsc);
    } else {
        secmax = lsc - fsc;     // sectors 0...secmax
        sectors = secmax + 1;   // number of sectors
        als[dn] = dks / 8;      // size of allocation vector
        if (dks % 8) {
            als[dn]++;
        }
        css[dn] = cks / 4;      // number of checksum elements

        // generate the block shift value
        blkval = bls / 128;     // number of sectors/block
        blkshf = 0;             // counts right 0's in blkval
        blkmsk = 0;             // fills with 1's from right
        for (i = 0; i < 16; i++ ) { // once for each bit position
            if (blkval == 1) {
                break;
            }
            // otherwise, high order 1 not found yet
            blkshf++;
            blkmsk <<= 1;
            blkmsk |= 1;
            blkval >>= 1;
        }

        // generate the extent mask byte
        blkval = bls / 1024;    // number of kilobytes/block
        extmsk = 0;             // fill from right with 1's
        for (i = 0; i < 16; i++) {
            if (blkval == 1) {
                break;
            }
            // otherwise more to shift
            extmsk <<= 1;
            extmsk |= 1;
            blkval >>= 1;
        }

        // maybe double byte allocation
        if (dks > 256) {
            extmsk >>= 1;
        }

        // maybe optional [0] in last position
        if (f16k != -1) {
            extmsk = f16k;
        }

        // now generate directory reservation bit vector
        dirrem = dir;           // # remaining to process
        dirbks = bls / 32;      // number of entries per block
        dirblk = 0;             // fill with 1's on each loop
        for (i = 0; i < 16; i++) {
            if (dirrem == 0) {
                break;
            }
            // not complete, iterate once again
            // shift right and add 1 high order bit
            dirblk >>= 1;
            dirblk |= 0x8000;
            if (dirrem > dirbks) {
                dirrem -= dirbks;
            } else {
                dirrem = 0;
            }
        }

        dpbhdr(dn);
        fprintf(stdout, "        .dw     %d\n", sectors);
        fprintf(stdout, "        .db     %d, %d, %d\n", blkshf, blkmsk, extmsk);
        fprintf(stdout, "        .dw     %d, %d\n", dks - 1, dir - 1);
        fprintf(stdout, "        .db     %d, %d\n", dirblk >> 8, dirblk & 0xFF);
        fprintf(stdout, "        .dw     %d, %d\n", cks / 4, ofs);

        // translate table
        if (skf == 0) {
            fprintf(stdout, "xlt%d    .equ    0\n", dn);
        } else {
            int nxtsec = 0;     // next sector to fill
            int nxtbas = 0;     // moves by one on overflow
            int neltst = sectors / gcd(sectors, skf);
            // neltst is number of elements to generate
            // before we overlap previous elements
            int nelts = neltst; // counter

            fprintf(stdout, "xlt%d    .equ    .\n", dn);

            for (i = 0; i < sectors; i++) {
                if (!(i % (sectors / 2))) {
                    fprintf(stdout, "%s        .d%c     ", (i != 0) ? "\n" : "", (sectors < 256) ? 'b' : 'w');
                } else {
                    fprintf(stdout, ", ");
                }

                fprintf(stdout, "%d", nxtsec + fsc);

                nxtsec += skf;
                if (nxtsec >= sectors) {
                    nxtsec -= sectors;
                }

                nelts--;
                if (nelts == 0) {
                    nxtbas++;
                    nxtsec = nxtbas;
                    nelts = neltst;
                }
            }

            fprintf(stdout, "\n");
        }
    }
}

void defds(char *lab, int space)
{
    char label[9];

    sprintf(label, "%s:", lab);
    fprintf(stdout, "%-8s.ds     %d\n", label, space);
}

void lds(char *lb, int dn, int *val)
{
    char buf[32];

    sprintf(buf, "%s%d", lb, dn);
    defds(buf, val[dn]);
}

void endef(int nd)
{
    int i;

    fprintf(stdout, "dirbuf: .ds     128\n");

    for (i = 0; i < nd; i++) {
        lds("alv", i, als);
        lds("csv", i, css);
    }
    fprintf(stdout, "\n");
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    time_t t = time(NULL);;
    char line[256];
    int lineno;
    int ignored;
    unsigned int i;
    unsigned int n;
    int nd;
    int dn, dm;
    int fsc, lsc, skf, bls, dks, dir, cks, ofs, f16k;

    fprintf(stdout, ";\n; Disk tables generated with mkdskdef -- %s", ctime(&t));

    /*
     dn     is the disk number 0,1,...,n-1
     fsc    is the first sector number (usually 0 or 1)
     lsc    is the last sector number on a track
     skf    is optional "skew factor" for sector translate
     bls    is the data block size (1024,2048,...,16384)
     dks    is the disk size in bls increments (word)
     dir    is the number of directory elements (word)
     cks    is the number of dir elements to checksum
     ofs    is the number of tracks to skip (word)
     f16k   is an optional 0 which forces 16K/directory entry
    */

    lineno = 0;
    ignored = 0;
    nd = 0;
    while (fgets(line, sizeof(line), stdin) != NULL) {
        lineno++;

        for (i = 0; (i < sizeof(line)) && isspace(line[i]); i++);

        if ((i == sizeof(line)) || (line[i] == '#') || (line[i] == '\0')) {
            continue;
        }

        line[strcspn(line, "\r\n")] = '\0';

        fprintf(stdout, ";\n; %d \"%s\"\n;\n", lineno, line);

        if (nd == 0) {
            if (sscanf(line, "disks %d", &nd) == 1) {
                disks(nd);
            }
        } else {
            f16k = -1;

            if ((sscanf(line, "diskdef %d,%d,%d,%d,%d,%d,%d,%d,%d,%d %n", &dn, &fsc, &lsc, &skf, &bls, &dks, &dir, &cks, &ofs, &f16k, (int *)&n) == 10) &&
                ((strlen(line) == n) || ('#' == line[n]))) {
                diskdef(dn, fsc, lsc, skf, bls, dks, dir, cks, ofs, f16k);
            } else if ((sscanf(line, "diskdef %d,%d,%d,%d,%d,%d,%d,%d,%d %n", &dn, &fsc, &lsc, &skf, &bls, &dks, &dir, &cks, &ofs, (int *)&n) == 9) &&
                       ((strlen(line) == n) || ('#' == line[n]))) {
                diskdef(dn, fsc, lsc, skf, bls, dks, dir, cks, ofs, f16k);
            } else if ((sscanf(line, "diskdef %d,%d,%d, ,%d,%d,%d,%d,%d,%d %n", &dn, &fsc, &lsc, &bls, &dks, &dir, &cks, &ofs, &f16k, (int *)&n) == 9) &&
                       ((strlen(line) == n) || ('#' == line[n]))) {
                diskdef(dn, fsc, lsc, 0, bls, dks, dir, cks, ofs, f16k);
            } else if ((sscanf(line, "diskdef %d,%d,%d, ,%d,%d,%d,%d,%d %n", &dn, &fsc, &lsc, &bls, &dks, &dir, &cks, &ofs, (int *)&n) == 8) &&
                       ((strlen(line) == n) || ('#' == line[n]))) {
                diskdef(dn, fsc, lsc, 0, bls, dks, dir, cks, ofs, f16k);
            } else if ((sscanf(line, "diskdef %d,%d %n", &dn, &dm, (int *)&n) == 2) &&
                       ((strlen(line) == n) || ('#' == line[n]))) {
                diskdef(dn, dm, 0, 0, 0, 0, 0, 0, 0, -1);
            } else if ((sscanf(line, "endef %n", (int *)&n) == 0) &&
                       ((strlen(line) == n) || ('#' == line[n]))) {
                endef(nd);
            } else {
                fprintf(stdout, "*** ignoring line %d ***\n", lineno);
                ignored++;
            }
        }
    }

    return (ignored) ? 1 : 0;
}
