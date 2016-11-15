#include <ctype.h>
#include <stdio.h>

#define BUFSIZE 16

int main(int argc, char *argv[])
{

	if (argc != 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return 1;
	}

	FILE* in = fopen(argv[1], "r");
	if (in == NULL) {
		fprintf(stderr, "failed to open input file '%s'\n", argv[1]);
		return 1;
	}

	char buf[BUFSIZE];
	int bread;
	int i;

	int lines = 0;
	int words = 0;
	int bytes = 0;

	int inword = 0;

	while (!feof(in)) {
		bread = fread(buf, 1, BUFSIZE, in);

		for (i = 0; i < bread; i++) {
			if (buf[i] == '\n') {
				lines++;
			}
			if (isspace(buf[i])) {
				inword = 0;
			} else {
				if (isprint(buf[i])) {
					if (!inword) {
						words++;
					}
					inword = 1;
				}
			}
		}

		bytes += bread;
	}

	fclose(in);
	in = NULL;

	printf("%d %d %d %s\n", lines, words, bytes, argv[1]);

	return 0;
}
