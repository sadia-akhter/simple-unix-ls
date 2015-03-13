/*
 * This program is a basic implementation of Unix ls command.
 * Author: Sadia Akhter
 * Date: Oct 20, 2014
 * Version: 1.0 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fts.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <bsd/string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <sys/ioctl.h>

#define NOFLAG 0
#define FLAG_d 1
#define FLAG_a 2
#define FLAG_A 3
#define FLAG_f 4
#define FLAG_S 5
#define FLAG_c 6
#define FLAG_u 7

#define FILE_ATIME 100
#define FILE_MTIME 101
#define FILE_CTIME 102

const int FTS_PATH = 0;
const int FTS_NAME = 1;
const int LINKED_TO = 2;

const int IS_DIR = 1;
const int NOT_DIR = 0;

const int IS_FIRST = 1;
const int NOT_FIRST = 0;

const int GET_MAX_WIDTHS = 1;
const int PRINT_FILES = 2;

const char *progname;

struct winsize w;
int currentNameColumn, currentPathColumn;

int flagR, flaga, flagA, flagd;
int flag1, flagl, flagn;
int flagC;
int flagt, flagS, flagr;
int flagi;
int flagF;
int flags;

int flagk;
int flagh;

int flagq;
int flagw;

int sortFlag;
int timeFlag;

int maxWidthFileInode;
int maxWidthFileBlocks;
int maxWidthFileLink;
int maxWidthFileUsername;
int maxWidthFileGroupname; 
int maxWidthFileSize;
int maxWidthFileMajor;
int maxWidthFileMinor;
int maxWidthFileName;
int maxWidthFilePath;
blkcnt_t fileTotalSystemBlocks;


FTS * getFileHierarchy(char **, int);
int entcmp(const FTSENT **, const FTSENT **);
int cmpLexicograph(const void *, const void *);

void initMaxWidthFiles();
void updateMaxWidthFiles(FTSENT *);

void handleFiles(char **, int, int, int); 
void handleFlagRecursive(char **, int); 
void handleFlagNonRecursive(char **, int, int, int);

void print(FTSENT *, int, int, int);
void printFlag1(FTSENT *, int, int, int);
void printFlagln(FTSENT *, int, int, int);
void printFlagC(FTSENT *, int, int, int);
void printDefault(FTSENT *, int, int, int);
void printInode(struct stat *);
void printBlocks(struct stat *);
void printMode(struct stat *);
void printUid(struct stat *, int);
void printGid(struct stat *, int);
void printSize(struct stat *, int, int, int);
void printDate(struct stat *);
void printNameWithLinkedToFile(FTSENT *, int, int);
void printName(char *, int, int);
void printFilename(char *, int, int);
void printFileTypeSuffix(struct stat *);
void printTotalSystemBlocks();
void humanizeFilesize(long long, char *, int);
void replaceNonPrintableChar(char *);
void leftAllign(char *);

int main(int argc, char **argv)
{
	int ch;

	progname = argv[0];
	ioctl(0, TIOCGWINSZ, &w);

	sortFlag = NOFLAG;
	timeFlag = FILE_MTIME;

	if (isatty(fileno(stdout))) {
		flagq = 1;
		flag1 = 1;
	} else {
		flagw = 1;
		flag1 = 1;
	}

	if (getuid() == 0 || geteuid() == 0) {
		flagA = 1;
	}

	// parse options
	while ((ch = getopt(argc, argv, "AaCcdFfhiklnqRrSstuwx1")) != -1) {
		switch (ch) {
			case 'A':
				flagA = 1;
				flaga = 0;
				flagd = 0;
				break;
			case 'a':
				flaga = 1;
				flagA = 0;
				flagd = 0;
				break;
			case 'C':
				flagC = 1;
				flagl = 0;
				flagn = 0;
				flag1 = 0;
			case 'c':
				timeFlag = FILE_CTIME;
				break;
			case 'd':
				flagd = 1;
				flagA = 0;
				flaga = 0;
				break;
			case 'F':
				flagF = 1;
				break;
			case 'f':
				sortFlag = FLAG_f;
				break;
			case 'h':
				flagh = 1;
				flagk = 0;
				break;
			case 'i':
				flagi = 1;
				break;
			case 'k':
				flagk = 1;
				flagh = 0;
				break;
			case 'l':
				flagl = 1;
				flag1 = 0;
				flagn = 0;
				flagC = 0;
				break;
			case 'n':
				flagn = 1;
				flagl = 0;
				flag1 = 0;
				flagC = 0;
				break;
			case 'q':
				flagq = 1;
				flagw = 0;
				break;
			case 'R':
				flagR = 1;
				break;
			case 'r':
				flagr = 1;
				break;
			case 'S':
				flagS = 1;
				flagt = 0;
				break;
			case 's':
				flags = 1;
				break;
			case 't':
				flagt = 1;
				flagS = 0;
				break;
			case 'u':
				timeFlag = FILE_ATIME;
				break;
			case 'w':
				flagw = 1;
				flagq = 0;
				break;
			case 'x':
				flagC = 1;
				flagl = 0;
				flagn = 0;
				flag1 = 0;
				break;
			case '1':
				flag1 = 1;
				flagl = 0;
				flagn = 0;
				flagC = 0;
				break;
			default:
				fprintf(stderr, "usage: %s [-AaCcdfhiklnqRrSstuwx1] [file ...]\n",progname);
				exit(1); 
		}
	}

	if (flagS == 1) {
		sortFlag = FLAG_S;
	} else if (flagt == 1) {
		sortFlag = timeFlag;
	} 
	
	argc -= optind;
	argv += optind;

	// separate files and dirs
	char **files;
	char **dirs;
	struct stat sb;
	int fileCount;
	int dirCount;
	int i;

	fileCount = 0;
	dirCount = 0;
	

	if (argc == 0) {
		if ((dirs = malloc(2 * sizeof(char *))) == NULL) {
			perror("malloc");
			exit(1);
		}
		dirs[0] = ".";
		dirs[1] = NULL;
		dirCount = 1;
	} else {
		if ((files = malloc((argc + 1) * sizeof(char *))) == NULL) {
			perror("malloc");
			exit(1);
		}

		if ((dirs  = malloc((argc + 1) * sizeof(char *))) == NULL) {
			perror("malloc");
			exit(1);
		}

		for (i=0; i<argc; i++) {
			if (lstat(argv[i], &sb) == -1) {
				perror("lstat:");
				exit(1);
			}
	
			if (S_ISDIR(sb.st_mode)) {
				dirs[dirCount] = argv[i];
				dirCount++;	
			} else {
				files[fileCount] = argv[i];
				fileCount++;
			}
		}

		files[fileCount] = NULL;
		dirs[dirCount] = NULL;
	}

	
	if (flagd == 1) {
		if (argc == 0) {
			handleFiles(dirs, dirCount, GET_MAX_WIDTHS, FLAG_d);
			handleFiles(dirs, dirCount, PRINT_FILES, FLAG_d);
		} else {
			qsort(argv, argc, sizeof(argv[0]), cmpLexicograph);
			handleFiles(argv, argc, GET_MAX_WIDTHS, FLAG_d);
			handleFiles(argv, argc, PRINT_FILES, FLAG_d);
		}
	} else {
		if (fileCount > 0) {
			qsort(files, fileCount, sizeof(files[0]), cmpLexicograph);
			handleFiles(files, fileCount, GET_MAX_WIDTHS, NOFLAG);
			handleFiles(files, fileCount, PRINT_FILES, NOFLAG);
		}

		if (dirCount > 0) {
			if (fileCount > 0) {
				printf("\n");
			}

			qsort(dirs, dirCount, sizeof(dirs[0]), cmpLexicograph);
			
			if (flagR == 1) {
				if (flaga == 1) {
					handleFlagRecursive(dirs, FLAG_a); 
				} else if (flagA == 1) {
					handleFlagRecursive(dirs, FLAG_A);
				} else {
					handleFlagRecursive(dirs, NOFLAG);
				}

			} else { // R = 0 ; non-recursive
				if (flaga == 1) {
					handleFlagNonRecursive(dirs, dirCount, fileCount, FLAG_a);
				} else if (flagA == 1) {
					handleFlagNonRecursive(dirs, dirCount, fileCount, FLAG_A);
				} else {
					handleFlagNonRecursive(dirs, dirCount, fileCount, NOFLAG);
				}
			}
		}
	}

	if (flagC == 1) {
		printf("\n");
	}
	exit(0);
}

int 
cmpLexicograph(const void *p1, const void *p2)
{
	char *s1 = *(char * const *)p1;
	char *s2 = *(char * const *)p2;

	return (flagr == 1) ? strcasecmp(s2,s1) : strcmp(s1, s2);
}

int 
entcmp(const FTSENT **a, const FTSENT **b)
{
	const char *s1, *s2; 
	off_t sz1, sz2;
	time_t time1, time2;
	
	switch (sortFlag) {
		case NOFLAG:
			s1 = (*a)->fts_name;
			s2 = (*b)->fts_name;
			return (flagr == 1) ? strcasecmp(s2, s1) : strcasecmp(s1, s2);
		case FLAG_S:
			sz1 = (*a) -> fts_statp -> st_size;
			sz2 = (*b) -> fts_statp -> st_size;
			return (flagr == 1) ? sz1 - sz2 : sz2 - sz1;
		case FILE_ATIME:
			time1 = (*a) -> fts_statp -> st_atime;
			time2 = (*b) -> fts_statp -> st_atime;
			return (flagr == 1) ? time1 - time2 : time2 - time1;
		case FILE_MTIME:
			time1 = (*a) -> fts_statp -> st_mtime;
			time2 = (*b) -> fts_statp -> st_mtime;
			return (flagr == 1) ? time1 - time2 : time2 - time1;
		case FILE_CTIME:
			time1 = (*a) -> fts_statp -> st_ctime;
			time2 = (*b) -> fts_statp -> st_ctime;
			return (flagr == 1) ? time1 - time2 : time2 - time1;
	}

	fprintf(stderr, "problem with sorting\n");
	return 0;
}

// dispFlag = {NOFLAG, FLAG_f}
// flag = {NOFLAG, FLAG_A, FLAG_a}
FTS * 
getFileHierarchy(char **files, int flag)
{
	FTS *tree;
	tree = NULL;

	switch(sortFlag) {
		case FLAG_f:
			switch(flag) {
				case NOFLAG:
				case FLAG_A:
					tree = fts_open(files, FTS_PHYSICAL, NULL);
					break;
				case FLAG_a:
					tree = fts_open(files, FTS_PHYSICAL | FTS_SEEDOT, NULL);
					break;
			}
			break;
		default:
			switch(flag) {
				case NOFLAG:
				case FLAG_A:
					tree = fts_open(files, FTS_PHYSICAL, entcmp);
					break;
				case FLAG_a:
					tree = fts_open(files, FTS_PHYSICAL | FTS_SEEDOT, entcmp);
					break;
			}
	}

	if (tree == NULL) {
		perror("fts_open");
		exit(1);
	} 

	return tree;	
}


void 
handleFiles(char **files, int fileCount, int action, int flag) 
{
	FTS *tree;
	FTSENT *f;

	if (fileCount > 0) {
		tree = getFileHierarchy(files, NOFLAG);

		if (action == GET_MAX_WIDTHS) {
			initMaxWidthFiles();

			if (flag == NOFLAG) {
				while ((f = fts_read(tree)) != NULL) {
					updateMaxWidthFiles(f);
				}
			}
 
			if (flag == FLAG_d) {
				while ((f = fts_read(tree)) != NULL) {
					if (f->fts_info == FTS_DP) {
						fts_set(tree, f, FTS_SKIP);
						continue;
					}
					updateMaxWidthFiles(f);
					fts_set(tree, f, FTS_SKIP);
				}
			}
		} 
 

		if (action == PRINT_FILES){
			if (flag == NOFLAG) {
				while ((f = fts_read(tree)) != NULL) {
					print(f, FTS_PATH, NOT_DIR, NOT_FIRST);
				}
			} 

			if (flag == FLAG_d) {
				while ((f = fts_read(tree)) != NULL) {
					if (f->fts_info == FTS_DP) {
						fts_set(tree, f, FTS_SKIP);
						continue;
					}
					print(f, FTS_PATH, NOT_DIR, NOT_FIRST);
					fts_set(tree, f, FTS_SKIP);
				}
			}
		} 

		if (f == NULL && errno != 0) {
			perror("fts_read");
			exit(1);
		}

		if (fts_close(tree) == -1) {
			fprintf(stderr, "error: fts_close\n");
			exit(1);
		}
	}
}

void 
initMaxWidthFiles()
{
	maxWidthFileInode = 0;
	maxWidthFileBlocks = 0;
	maxWidthFileLink = 0;
	maxWidthFileUsername = 0;
	maxWidthFileGroupname = 0;
	maxWidthFileSize = 0;
	maxWidthFileMajor = 0;
	maxWidthFileMinor = 0;
	maxWidthFileName = 0;
	maxWidthFilePath = 0;
	currentNameColumn = 0;
	currentPathColumn = 0;
	fileTotalSystemBlocks = 0;
}


void
updateMaxWidthFiles(FTSENT *f)
{
	int width;
	char str[100];

	struct passwd *userInfo;
	struct group *groupInfo;

	// get max width of inode
	memset(str, 0, 100);
	snprintf(str, 100, "%lld", (long long) (f -> fts_statp -> st_ino));
	width = strlen(str);
	if (maxWidthFileInode < width) {
		maxWidthFileInode = width;
	}

	// get max with of blocks
	memset(str, 0, 100);
	snprintf(str, 100, "%lld", (long long) (f -> fts_statp -> st_blocks));
	width = strlen(str);
	if (maxWidthFileBlocks < width) {
		 maxWidthFileBlocks = width;
	}

	// get max width of links
	memset(str, 0, 100);
	snprintf(str, 100, "%ld", (long)(f->fts_statp->st_nlink));
	width = strlen(str);
	if (maxWidthFileLink < width) {
		maxWidthFileLink = width;
	}

	// get max width of username
	if (flagl == 1) {
		if((userInfo = getpwuid(f->fts_statp->st_uid)) != NULL) {
			width = strlen(userInfo->pw_name);
		} else {
			memset(str, 0, 100);
			snprintf(str, 100, "%d", f->fts_statp->st_uid);
			width = strlen(str);
		}
	} else if (flagn == 1) {
		memset(str, 0, 100);
		snprintf(str, 100, "%d", f->fts_statp->st_uid);
		width = strlen(str);
	}
	
	if (maxWidthFileUsername < width) {
		maxWidthFileUsername = width;
	}

	// get max width of groupname
	if (flagl == 1) {
		if((groupInfo = getgrgid(f->fts_statp->st_gid)) != NULL) {
			width = strlen(groupInfo->gr_name);
		} else {
			memset(str, 0, 100);
			snprintf(str, 100, "%d", f->fts_statp->st_gid);
			width = strlen(str);
		}
	} else if (flagn == 1) {
		memset(str, 0, 100);
		snprintf(str, 100, "%d", f->fts_statp->st_gid);
		width = strlen(str);
	}

	if (maxWidthFileGroupname < width) {
		maxWidthFileGroupname = width;
	}

	// get max width of size
	memset(str, 0, 100);
	snprintf(str, 100, "%lld", (long long) (f->fts_statp->st_size));
	width = strlen(str);

	if (maxWidthFileSize < width) {
		maxWidthFileSize = width;
	}
	
	if (S_ISCHR(f->fts_statp->st_mode) || S_ISBLK(f->fts_statp->st_mode)) {
		// get max width of major
		memset(str, 0, 100);
		snprintf(str, 100, "%d", major(f->fts_statp->st_rdev));
		width = strlen(str);

		if (maxWidthFileMajor < width) {
			maxWidthFileMajor = width;
		}

		// get max width of minor
		memset(str, 0, 100);
		snprintf(str, 100, "%d", minor(f->fts_statp->st_rdev));
		width = strlen(str);

		if (maxWidthFileMinor < width) {
			maxWidthFileMinor = width;
		}
	}

	// get max width of filename
	width = strlen(f -> fts_name);
	if (maxWidthFileName < width) {
		maxWidthFileName = width;
	}

	// get max width of filepath	
	width = strlen(f -> fts_path);
	if (maxWidthFilePath < width) {
		maxWidthFilePath = width;
	}

	// get total system blocks
	fileTotalSystemBlocks += f->fts_statp->st_blocks;

}

// R = 1
void handleFlagRecursive(char **dirs, int flag) 
{
	FTS *tree;
	FTSENT *f1, *f2;
	FTSENT *temp;
	int i;

	tree = getFileHierarchy(dirs, flag);
	
	i = 0;
	while ((f1 = fts_read(tree)) != NULL) {
		
		if (f1->fts_info == FTS_DP) {
			continue;
		}

		if (flag == NOFLAG) {
			// skip hidden files
			if (f1->fts_name[0] == '.' && strcmp(f1->fts_name, f1->fts_path) != 0) {
				fts_set(tree, f1, FTS_SKIP);
				continue;
			}
		}

		if (f1->fts_info == FTS_D) {
			if (i == 0) {
				print(f1, FTS_PATH, IS_DIR, IS_FIRST);
				i = 1;
			} else {
				print(f1, FTS_PATH, IS_DIR, NOT_FIRST);
			}
			f2 = fts_children(tree, 0);
			
			temp = f2;
			initMaxWidthFiles();
			while (temp != NULL) {
				if (temp -> fts_name[0] != '.') {
					updateMaxWidthFiles(temp);
				}
				temp = temp -> fts_link;
			}

			if (flagl == 1 || flagn == 1 || (flags == 1 && isatty(fileno(stdout)))) {
				printTotalSystemBlocks();
			}

			if (flag == FLAG_a || flag == FLAG_A) {
				while (f2 != NULL) {
					print(f2, FTS_NAME, NOT_DIR, NOT_FIRST);
					f2 = f2 -> fts_link;
				}
			}

			if (flag == NOFLAG) {
				while (f2 != NULL) {
					if (f2->fts_name[0] != '.') {
						print(f2, FTS_NAME, NOT_DIR, NOT_FIRST);
					}
					f2 = f2 -> fts_link;
				}
			}
		}
	}

	if (f1 == NULL && errno != 0) {
		perror("fts_read");
		exit(1);
	}

	if (fts_close(tree) == -1) {
		fprintf(stderr, "error: fts_close\n");
		exit(1);
	}
}


void 
handleFlagNonRecursive(char **dirs, int dirCount, int fileCount, int flag)
{
	FTS *tree;
	FTSENT *f1, *f2;
	int i;
	FTSENT *temp;

	if (dirCount <= 0) {
		return;
	}

	tree = getFileHierarchy(dirs, flag);

	i = 0;
	while ((f1 = fts_read(tree)) != NULL) {
		
		if (f1->fts_info == FTS_DP){
			continue;
		}
 
		if (dirCount > 1 || fileCount > 0) {
			print(f1, FTS_PATH, IS_DIR, IS_FIRST);
		}
		
		f2 = fts_children(tree, 0);

		temp = f2;
		initMaxWidthFiles();

		if (flag == FLAG_A || flag == FLAG_a) {
			while (temp != NULL) {
				updateMaxWidthFiles(temp);
				temp = temp -> fts_link;
			}
		}

		if (flag == NOFLAG) {
			while (temp != NULL) {
				if (temp -> fts_name[0] != '.') {
					updateMaxWidthFiles(temp);
				}
				temp = temp -> fts_link;
			}
		}

		if (flagl == 1 || flagn == 1 || (flags == 1 && isatty(fileno(stdout)))) {
			printTotalSystemBlocks();
		}
		if (flag == FLAG_a || flag == FLAG_A) {
			while (f2 != NULL) {
				print(f2, FTS_NAME, NOT_DIR, i);
				fts_set(tree, f2, FTS_SKIP);
				f2 = f2->fts_link;
			}
		}

		if (flag == NOFLAG) {
			while (f2 != NULL) {
				// skip hidden files
				if (f2->fts_name[0] == '.' && strcmp(f2->fts_name, ".") != 0 && strcmp(f2->fts_name, "..") != 0) {
					fts_set(tree, f1, FTS_SKIP);
					f2 = f2 -> fts_link;
					continue;
				}
				print(f2, FTS_NAME, NOT_DIR, i);
				fts_set(tree, f2, FTS_SKIP);
				f2 = f2->fts_link;
			}
		}

		fts_set(tree, f1, FTS_SKIP);

		if (++i < dirCount)
			printf("\n");
	}

	if (f1 == NULL && errno != 0) {
		perror("fts_read");
		exit(1);
	}

	if (fts_close(tree) == -1) {
		fprintf(stderr, "error: fts_close\n");
		exit(1);
	}
}

void 
print(FTSENT *f, int isName, int isDir, int isFirst)
{
	if (flag1 == 1) {
		printFlag1(f, isName, isDir, isFirst);
	} else if(flagl == 1 || flagn == 1) { 
		printFlagln(f, isName, isDir, isFirst);
	} else if (flagC == 1) {
		printFlagC(f, isName, isDir, isFirst);
	} else {
		printDefault(f, isName, isDir, isFirst);
	}
}

void 
replaceNonPrintableChar(char *fileName)
{
	while (*fileName != '\0') {
		if (!isprint(*fileName)) { 
			*fileName = '?';
		}
		fileName++;
	}
}

void 
leftAllign(char *name)
{
	int len, i;

	len = strlen(name);
	while (*name == ' ') {
		for (i = 0; i < len - 1; i++) {
			name[i] = name[i+1];
		}

		for (;i < len; i++) {
			name[i] = ' ';
		}
		name[len] = '\0';
	}

}


// maxWidth = 0 : don't consider it
// otherwise consider it
void 
printName(char *filename, int maxWidth, int isName)
{
	char *name;
	char *temp;
	char ch;

	if (flagq == 1) {
		if ((name = malloc((strlen(filename) + 1) * sizeof(char))) == NULL) {
			perror("malloc");
			exit(1);
		}
		if ((temp = malloc((maxWidth + 2) * sizeof(char))) == NULL) {
			perror("malloc");
			exit(1);
		}

		memset(name, 0, sizeof(name));
		strcpy(name, filename);
		replaceNonPrintableChar(name);

		if (maxWidth == 0) {
			printf("%s", name);
		} else {
			memset(temp, 0, maxWidth + 2);
			snprintf(temp, maxWidth + 2, "%*s ", maxWidth, name);
			leftAllign(temp);
			printf("%s", temp);
		}

		free(name);
		free(temp);
		return;
	}

	if (flagw == 1) {
		if ((name = malloc((strlen(filename) * 4 + 2) * sizeof(char))) == NULL) {
			perror("malloc");
			exit(1);
		}
		if ((temp = malloc((strlen(filename) * 4 + 2) * sizeof(char))) == NULL) {
			perror("malloc");
			exit(1);
		}
	
		memset(name, 0, sizeof(name));
		
		while (*filename != '\0') {
			memset(temp, 0, sizeof(temp));
			ch = filename[0];
			if (ch < 32 && ch > 0) {
				snprintf(temp, sizeof(temp), "^%c", ch + 'A' - 1);
			} else if (ch >= 127) {
				snprintf(temp, sizeof(temp), "\\%o", ch);
			} else {
				snprintf(temp, sizeof(temp), "%c", ch);
			}
			strcat(name, temp);
			filename++;
		}

		if (isName == FTS_NAME) {
			if (strlen(name) > maxWidthFileName) {
				maxWidthFileName = strlen(name);
			}
		}

		if (isName == FTS_PATH) {
			if (strlen(name) > maxWidthFilePath) {
				maxWidthFilePath = strlen(name);
			}
		}

		if (maxWidth == 0) { 
			printf("%s", name);  
		} else {
			memset(temp, 0, sizeof(temp));
			if (strlen(name) < maxWidth) {
				snprintf(temp, maxWidth + 2, "%*s ", maxWidth, name);
				leftAllign(temp);
				printf("%s ", temp);
			} else {
				maxWidth = strlen(name);
				snprintf(temp, maxWidth + 2, "%*s ", maxWidth, name);
				leftAllign(temp);
				printf("%s ", temp);
			}

		}

		free(name);
		free(temp);
		return;

	}

	printf("%s", filename);
}

void
printFilename(char *filename, int isName, int isDir)
{
	int columns;

	if (flagC == 1) {

		if (isDir == IS_DIR) {
			printName(filename, 0, isName);
			return;	
		}
		if (isName == FTS_NAME) {
			columns = w.ws_col/(maxWidthFileName + 2);
			if (++currentNameColumn <= columns) {
				printName(filename,maxWidthFileName, isName);
			} else {
				currentNameColumn = 1;
				printf("\n");
				printName(filename,maxWidthFileName, isName);
			}
		} else if (isName == FTS_PATH) {
			columns = w.ws_col/(maxWidthFilePath + 2);
			if (++currentPathColumn <= columns) {
				printName(filename,maxWidthFileName, isName);
			} else {
				currentPathColumn = 1;
				printf("\n");
				printName(filename,maxWidthFileName, isName);
			}
			
		}

		return;
	}

	printName(filename, 0, isName);	

}

void 
printFlag1(FTSENT *f, int isName, int isDir, int isFirst)
{
	if (isName == FTS_NAME) { 
		printInode(f -> fts_statp);
		printBlocks(f -> fts_statp);
		printFilename(f -> fts_name, isName, isDir);
		printFileTypeSuffix(f->fts_statp);
		printf("\n");
	} else { 
		if (isDir == IS_DIR) {
			if (isFirst != IS_FIRST) {
				printf("\n");
			}

			printFilename(f -> fts_path, isName, isDir);
			printf(":\n");
		} else {
			printInode(f -> fts_statp);
			printBlocks(f -> fts_statp);
			printFilename(f -> fts_path, isName, isDir);
			printFileTypeSuffix(f->fts_statp);
			printf("\n");
		}
	}
}

void 
printFlagln(FTSENT *f, int isName, int isDir, int isFirst)
{
	if (isName == FTS_NAME) { 
		printInode(f -> fts_statp);
		printBlocks(f -> fts_statp);
		printMode(f -> fts_statp);
		printf("%*ld ", maxWidthFileLink, (long) f->fts_statp->st_nlink);
		printUid(f -> fts_statp, maxWidthFileUsername);
		printGid(f -> fts_statp, maxWidthFileGroupname);
		printSize(f -> fts_statp, maxWidthFileSize, maxWidthFileMajor, maxWidthFileMinor);
		printDate(f -> fts_statp);
		printNameWithLinkedToFile(f, isName, isDir);
		printf("\n");
	} else { 
		if (isDir == IS_DIR) {
			if (isFirst != IS_FIRST) {
				printf("\n");
			}

			printFilename(f -> fts_path, isName, isDir);
			printf(":\n");
		} else {
			printInode(f -> fts_statp);
			printBlocks(f -> fts_statp);
			printMode(f->fts_statp);
			printf("%*ld ", maxWidthFileLink, (long) f->fts_statp->st_nlink);
			printUid(f -> fts_statp, maxWidthFileUsername);
			printGid(f -> fts_statp, maxWidthFileGroupname);
			printSize(f -> fts_statp, maxWidthFileSize, maxWidthFileMajor, maxWidthFileMinor);
			printDate(f -> fts_statp);
			printNameWithLinkedToFile(f, isName, isDir);
			printf("\n");
		}
	}
}

// isName = 1 => fts_name, else  => fts_path
// isDir = 1 => directory, else => file
// isFirst = 1 => first directory, else => not first directory
void printFlagC(FTSENT *f, int isName, int isDir, int isFirst)
{
	if (isName == FTS_NAME) { 
		printInode(f -> fts_statp);
		printBlocks(f -> fts_statp);
		printNameWithLinkedToFile(f, isName, isDir);
	} else { 
		if (isDir == IS_DIR) {
			if (isFirst != IS_FIRST) {
				printf("\n");
			}

			printFilename(f -> fts_path, isName, isDir);
			printf(":\n");
		} else {
			printInode(f -> fts_statp);
			printBlocks(f -> fts_statp);
			printNameWithLinkedToFile(f, isName, isDir);
		}
	}
}

void 
printDefault(FTSENT *f, int isName, int isDir, int isFirst)
{
	printFlag1(f, isName, isDir, isFirst);
}


void 
printInode(struct stat *sb)
{
	if(flagi == 1) {
		printf("%*lld ", maxWidthFileInode, (long long) sb -> st_ino);
	}
}


void 
printMode(struct stat *sb)
{
	char mode[12];

	memset(mode, 0, 12);
	strmode(sb->st_mode, mode);

	if (mode[10] == ' ') {
		mode[10] = '\0';
	}
	
	printf("%s ", mode);
}

void
printUid(struct stat *sb, int maxWidth)
{
	struct passwd *userInfo;
	char uid[100];
	int len;

	memset(uid, 0, 100);
	
	if (flagn == 1) {
		snprintf(uid, 100, "%d", sb -> st_uid);
	} else if (flagl == 1) {
		if ((userInfo = getpwuid(sb -> st_uid)) != NULL) {
			snprintf(uid, 100, "%s", userInfo -> pw_name);
		} else {
			snprintf(uid, 100, "%d", sb -> st_uid);
		}
	}

	len = strlen(uid);
	while (len < maxWidth) {
		uid[len] = ' ';
		len++;
	}
	
	printf("%s ", uid);
}

void
printGid(struct stat *sb, int maxWidth)
{
	struct group *groupInfo;
	char gid[100];
	int len;

	memset(gid, 0, 100);

	if (flagn == 1) {
		snprintf(gid, 100, "%d", sb -> st_gid);
	} else if (flagl == 1) {
		if ((groupInfo = getgrgid(sb -> st_gid)) != NULL) {
			snprintf(gid, 100, "%s", groupInfo -> gr_name);
		} else {
			snprintf(gid, 100, "%d", sb -> st_gid);
		}
	}

	len = strlen(gid);
	while (len < maxWidth) {
		gid[len] = ' ';
		len++;
	}
	
	printf("%s ", gid);
}

void 
printSize(struct stat *sb, int maxWidthSize, int maxWidthMajor, int maxWidthMinor)
{
	int maxWidth;
	char *size;

	if (flagh == 1) {
		maxWidthSize = 4;
	}

	if (maxWidthSize > maxWidthMajor + maxWidthMinor + 2) {
		maxWidth = maxWidthSize;
	} else {
		maxWidth = maxWidthMajor + maxWidthMinor + 2;
	}

	if ((size = malloc((maxWidth + 1 ) * sizeof(char))) == NULL) {
		perror("malloc");
		exit(1);
	}

	if (S_ISCHR(sb -> st_mode) || S_ISBLK(sb -> st_mode)) {
		memset(size, 0, maxWidth + 1);
		snprintf(size, maxWidthMajor + 2, "%*d,", maxWidthMajor, major(sb -> st_rdev));
		printf("%s ", size);

		memset(size, 0, maxWidth + 1);
		snprintf(size, maxWidthMinor + 1, "%*d", maxWidthMinor, minor(sb -> st_rdev));
		printf("%s ", size);

	} else {
		if (flagh == 1) {
			memset(size, 0, maxWidth + 1);
			humanizeFilesize((long long) (sb -> st_size), size, maxWidth + 1);
		} else {
			memset(size, 0, maxWidth + 1);
			snprintf(size, maxWidth + 1, "%*lld", maxWidth, (long long) (sb -> st_size));
		}
		
		printf("%s ", size);	
	}

	free(size);
}

void
printBlocks(struct stat *sb)
{
	char *blocksize;
	char *endptr;
	long blksize;
	long double blocksFraction;
	long long blocks;

	char *size;

	if (flags == 0) {
		return;
	}

	blocks = (long long) sb -> st_blocks;
	blksize = 512;

	if (flagh == 1) {
		maxWidthFileBlocks = 4;
		if ((size = malloc(5 * sizeof (char))) == NULL) {
			perror("malloc");
			exit(1);
		}

		memset(size, 0 , 5);
		humanizeFilesize(blocks * 512, size, 5);
		printf("%s ", size);
		free(size);
		return;
	}

	if (flagk == 1) {
		blksize = 1024;
	} else if ((blocksize = getenv("BLOCKSIZE")) != NULL) {
		blksize = strtol(blocksize, &endptr, 10);
		if (blksize == 0) {
			blksize = 512;
		}
	}

	blocksFraction =  (blocks * 512.0 / blksize);
	blocks = (long long) blocksFraction;
	if (blocks < blocksFraction) {
		blocks += 1;
	}

	printf("%*lld ", maxWidthFileBlocks, blocks);
}
void 
printTotalSystemBlocks()
{
	char *blocksize;
	char *endptr;
	long blksize;
	long double blocksFraction;
	char *totalSize;
	int i;

	if (flagh == 1) {
		if ((totalSize = malloc(5 * sizeof(char))) == NULL) {
			perror("malloc");
			exit(1);
		}
		memset(totalSize, 0, 5);
		humanizeFilesize((long long) fileTotalSystemBlocks * 512, totalSize, 5);
		
		while (*totalSize == ' ') {
			for (i = 0; i < 4; i++) {
				totalSize[i] = totalSize[i+1];
			}
		}

		printf("total %s\n", totalSize);
		free(totalSize);
		return;
	}
	
	if ((blocksize = getenv("BLOCKSIZE")) != NULL) {
		blksize = strtol(blocksize, &endptr, 10);
		if (blksize != 0 && *blocksize != '\0' && *endptr == '\0') {
			blocksFraction =  (fileTotalSystemBlocks * 512.0 / blksize);
			fileTotalSystemBlocks = (long long) blocksFraction;
			if (fileTotalSystemBlocks < blocksFraction) {
				fileTotalSystemBlocks += 1;
			}
		} 
	}

	printf("total %lld\n", (long long) fileTotalSystemBlocks);

}

void
humanizeFilesize(long long filesize, char *size, int len)
{
	int unit = 1024;
	char units[] = {' ', 'K', 'M', 'G', 'T', 'P'};
	int i;
	long double fsize;

	if(filesize < unit) {
		snprintf(size, len, "%*d", len - 1, (int)filesize);
		return;
	}

	fsize = filesize;

	i = 0;
	while (fsize >= unit && i < 5) {
		fsize /= unit;
		i++;
	}

	memset(size, 0, len);
	if (fsize < 10) {
		snprintf(size, len, "%*.1Lf%c", len - 2, fsize, units[i]);
		return;
	}

	fsize += 0.5;
	snprintf(size, len, "%*d%c", len - 2, (int) fsize, units[i]);
}

void 
printDate(struct stat *sb)
{
	char printTime[40];
	time_t now;
	double diff, th;

	time(&now);
	th = 6 * 30 * 24 * 60 * 60;
	memset(printTime, 0, 40);
	
	switch(timeFlag) {
		case FILE_ATIME:
			diff = difftime(now, sb -> st_atime);
			if (diff < th) {
				strftime(printTime, 40, "%b %e %R", localtime(&(sb -> st_atime)));
			} else {
				strftime(printTime, 40, "%b %e  %G", localtime(&(sb -> st_atime)));
			}
			break;
		case FILE_MTIME:
			diff = difftime(now, sb -> st_mtime);
			if (diff < th) {
				strftime(printTime, 40, "%b %e %R", localtime(&(sb -> st_mtime)));
			} else {
				strftime(printTime, 40, "%b %e  %G", localtime(&(sb -> st_mtime)));
			}
			break;
		case FILE_CTIME:
			diff = difftime(now, sb -> st_ctime);
			if (diff < th) {
				strftime(printTime, 40, "%b %e %R", localtime(&(sb -> st_ctime)));
			} else {
				strftime(printTime, 40, "%b %e  %G", localtime(&(sb -> st_ctime)));
			}
			break;
	}

	printf("%s ", printTime);
}



void 
printNameWithLinkedToFile(FTSENT *f, int isName, int isDir)
{
	int len;
	char linkedToFile[PATH_MAX];
	char resolvedPath[PATH_MAX];
	char *path;
	char *pwd;
	struct stat sb;

	memset(linkedToFile,0,PATH_MAX);

	if(S_ISLNK(f->fts_statp->st_mode )) {
		pwd = getenv("PWD");
		if ((path = malloc(strlen(pwd) + (strlen(f->fts_path) + strlen(f->fts_name) + 3) * sizeof(char))) == NULL) {
			perror("malloc");
			exit(1);
		}

		memset(path, 0, sizeof(path));
		
		if (isName == FTS_PATH) {
			if ((len = readlink(f->fts_path, linkedToFile, sizeof(linkedToFile) - 1)) == -1) {
				perror("readlink");
				exit(1);
			}
			linkedToFile[len] = '\0';
			printf("%s -> %s", f->fts_path, linkedToFile);	

			
		} else {
			if (*(f->fts_path) != '/') {
				strcpy(path, pwd);
				strcat(path, "/");
				strcat(path, f->fts_path);
				strcat(path, "/");
				strcat(path, f->fts_name);
			} else {
				strcpy(path, f->fts_path);
				strcat(path, "/");
				strcat(path, f->fts_name);
			}
			if ((len = readlink(path, linkedToFile, sizeof(linkedToFile) - 1)) == -1) {
				perror("readlink");
				exit(1);
			}	
			linkedToFile[len] = '\0';

			printFilename(f -> fts_name, isName, isDir);
			if (flagl == 1 || flagn == 1) {
				printf(" -> ");
				printFilename(linkedToFile, LINKED_TO, isDir);
			}
		}
	
		if (flagF == 1) {
			if (realpath(path, resolvedPath) == NULL) {
				perror("realpath");
				exit(1);
			}

			if(lstat(resolvedPath, &sb) == -1) {
				perror("lstat");
				exit(1);
			}

			printFileTypeSuffix(&sb);
		}

		free(path);
	} else {
		if (isName == FTS_NAME) {
			printFilename(f -> fts_name, isName, isDir);
		} else {
			printFilename(f -> fts_path, isName, isDir);
		}
		printFileTypeSuffix(f -> fts_statp);
	}
}

void
printFileTypeSuffix(struct stat *sb)
{
	char mode[12];

	if (flagF == 0) {
		return;
	}

	memset(mode, 0, 12);
	strmode(sb->st_mode, mode);
	
	if (mode[0] == 'd') {
		printf("/");
		return;
	}
	
	if (mode[0] == 'l') {
		printf("@");
		return;
	}

	if (mode[0] == 'p') {
		printf("|");
		return;
	}

	if (mode[0] == 's') {
		printf("=");
		return;
	}

	if (mode[0] == 'w') {
		printf("%%");
		return;
	}

	if (mode[3] == 'x' && mode[6] == 'x' && mode[9] == 'x') {
		printf("*");
		return;
	}
}

