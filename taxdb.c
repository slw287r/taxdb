#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/ioctl.h>

#include "kvec.h"
#include "ketopt.h"

#define VERSION "0.1.0"
extern const char *__progname;
typedef kvec_t(uint32_t) kv_t;

static void usage();
static void chomp(char *str);
static void error(const char *format, ...);
static uint32_t be(const uint32_t l);
static void ld_tids(const char *fn, kv_t *a);
static void get_names(const uint64_t *dat, const size_t ntx, const size_t fs,
		const char *line, const uint32_t tid, FILE *fp);

int main(int argc, char *argv[])
{
	if (argc == 1)
		usage();
	int c = 0, i, tax = 0;
	FILE *fp = NULL;
	char *btd = NULL, *bti = NULL, *txd = NULL, *out = NULL;
	ketopt_t opt = KETOPT_INIT;
	kv_t a;
	kv_init(a);
	while ((c = ketopt(&opt, argc, argv, 1, ":t:i:d:o:vh", 0)) >= 0)
	{
		if (c == 'h') usage();
		else if (c == 'v') return(puts(VERSION));
		else if (c == 'i') bti = opt.arg;
		else if (c == 'd') btd = opt.arg;
		else if (c == 'o') out = opt.arg;
		else if (c == 't')
		{
			tax = 1;
			if (!strchr(opt.arg, ','))
			{
				if (access(opt.arg, R_OK))
					kv_push(uint32_t, a, atoi(opt.arg));
				else
					ld_tids(opt.arg, &a);
			}
			else
			{
				char *p, *q, *t = strdup(opt.arg);
				p = q = t;
				while ((p = strchr(p, ',')))
				{
					*p = '\0';
					kv_push(uint32_t, a, atoi(q));
					q = ++p;
				}
				kv_push(uint32_t, a, atoi(q));
				free(t);
			}
		}
		else if (c == '?') error("Unknown option -%c\n", opt.opt ? opt.opt : ':');
		else if (c == ':') error("Missing option argument: -%c\n", opt.opt ? opt.opt : ':');
		else error("Unknown option encountered\n");
	}
	size_t sz = 0;
	char *line = NULL;
	if (!tax)
		error("Taxonomy ID is required via -t\n");
	if (access(btd, R_OK) || access(bti, R_OK))
		error("Error accessing taxdb file [%s] or [%s]\n", btd, bti);
	fp = fopen(btd, "r");
	if (getline(&line, &sz, fp) < 0)
		error("Error reading [%s]\n", btd);
	fclose(fp);

	fp = fopen(bti, "rb");
	fseek(fp, 0L, SEEK_END);
	size_t fs = ftell(fp);
	//printf("File size: %zu\n", fs);
	rewind(fp);
	uint32_t magic = 0, txnum = 0;
	fread(&magic, sizeof(uint32_t), 1, fp);
	if (be(magic) != 0x8739)
		error("Invalid taxdb.bti: [%s]\n", bti);
	fread(&txnum, sizeof(uint32_t), 1, fp);
	size_t ntx = be(txnum);
	// skip reserved fields
	uint8_t reserved[16] = {0};
	fread(reserved, sizeof(uint8_t), 16, fp);
	uint64_t *dat = calloc(sizeof(uint64_t), ntx);
	fread(dat, sizeof(uint64_t), ntx, fp);
	fclose(fp);
	uint64_t d0 = dat[0], d1 = dat[1], d2 = dat[2], dz = dat[ntx - 1];
	fp = out ? fopen(out, "w") : stdout;
	// header
	if (kv_size(a))
		fprintf(fp, "#TaxID\tScientificName\tCommonName\tBlastName\tKingdom\n");
	// proc tids
	for (i = 0; i < kv_size(a); ++i)
	{
		uint32_t tid = kv_A(a, i);
		get_names(dat, ntx, fs, line, tid, fp);
	}
	kv_destroy(a);
	free(line);
	return 0;
}

static uint32_t be(const uint32_t n)
{
#ifdef WORDS_BIGENDIAN
	return n;
#else
	uint32_t b0, b1, b2, b3;
	b0 = (n & 0x000000FF) << 24U;
	b1 = (n & 0x0000FF00) << 8U;
	b2 = (n & 0x00FF0000) >> 8U;
	b3 = (n & 0xFF000000) >> 24U;
	return(b0 | b1 | b2 | b3);
#endif
}

static void ld_tids(const char *fn, kv_t *a)
{
	FILE *fp = fopen(fn, "r");
	if (!fp)
		error("Error reading [%s]\n", fn);
	size_t sz = 0;
	char *line = NULL;
	while (getline(&line, &sz, fp) >= 0)
	{
		if (*line == '#')
			continue;
		chomp(line);
		if (strlen(line) == 0)
			continue;
		kv_push(uint32_t, *a, atoll(line));
	}
	fclose(fp);
	free(line);
}

static void get_names(const uint64_t *dat, const size_t ntx, const size_t fs,
		const char *line, const uint32_t tid, FILE *fp)
{
	uint32_t idx_low = 0, idx_high = ntx - 1;
	uint32_t tid_low = be((uint32_t)dat[idx_low]), tid_high = be((uint32_t)dat[idx_high]);
	if (tid < tid_low || tid > tid_high)
		fprintf(fp, "%"PRIu32"\tn/a\tn/a\tn/a\tn/a\n", tid);
	else
	{
		size_t idx_new = (idx_low + idx_high) / 2;
		size_t idx_old = idx_new;
		while (true)
		{
			uint32_t tid_cur = be((uint32_t)dat[idx_new]);
			if (tid < tid_cur)
				idx_high = idx_new;
			else if (tid > tid_cur)
				idx_low = idx_new;
			else
				break;
			idx_new = (idx_low + idx_high) / 2;
			if (idx_new == idx_old)
			{
				idx_new += (bool)(tid > idx_old);
				break;
			}
			idx_old = idx_new;
		}
		if (tid == be((uint32_t)dat[idx_new]))
		{
			uint32_t dat_begin = be((uint32_t)(dat[idx_new] >> 32)), dat_end = 0;
			if (idx_new == idx_high)
				dat_end = fs;
			else
				dat_end = be((uint32_t)(dat[idx_new + 1] >> 32));
			//printf("dat_begin: %d\tdat_end: %d\n", dat_begin, dat_end);
			char *s = strndup(line + dat_begin, dat_end - dat_begin);
			fprintf(fp, "%"PRIu32"\t%s\n", tid, s);
			free(s);
		}
		else
			fprintf(fp, "%"PRIu32"\tn/a\tn/a\tn/a\tn/a\n", tid);
	}
}

static void chomp(char *str)
{
	if (strchr(str, '\n'))
		str[strchr(str, '\n') - str] = '\0';
}

static void error(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	fputs("\e[1;31m\u2718\e[0;0m ", stderr);
	vfprintf(stderr, format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void horiz(const int _n)
{
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	int i, n = (w.ws_col >= _n) ? _n : w.ws_col;
	for (i = 0; i < n; ++i) fputs("\e[2m\u2014\e[0m", stdout);
	fputc('\n', stdout);
}

static void usage()
{
	int w = 49;
	horiz(w);
	puts("Get names by taxid from NCBI taxdb");
	horiz(w);
	printf("Usage: \e[31m%s\e[0m \e[2m[options]\e[0m\n", __progname);
	puts("  -d FILE taxdb.btd file");
	puts("  -i FILE taxdb.bti file");
	puts("  -t INT\e[2m,INT,...\e[0m  taxid(s) to query or");
	puts("          taxid list file, one per line");
	puts("  -o FILE output file \e[2m[stdout]\e[0m");
	putchar('\n');
	puts("  -h      print help");
	puts("  -v      print version");
	putchar('\n');
	puts("taxdb:");
	puts("ftp://ftp.ncbi.nlm.nih.gov/blast/db/taxdb.tar.gz");
	horiz(w);
	exit(1);
}
