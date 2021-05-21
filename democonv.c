#include <libgen.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int demoToTas(char *demo, char *tas) {
	FILE *demoFile = fopen(demo, "rb");
	FILE *tasFile = fopen(tas, "rb+");

	if (demoFile == NULL || tasFile == NULL) {
		fprintf(stderr, "Could not open file %s\n", demoFile == NULL ? demo : tas);
		return 1;
	}

	fseek(demoFile, 4, SEEK_SET);
	fseek(tasFile, 0x400, SEEK_SET);

	unsigned char demoBuf[4];
	unsigned char tasBuf[4];

	unsigned int inputs;

	while (fread(demoBuf, 4, 1, demoFile), !feof(demoFile)) {
		tasBuf[0] = demoBuf[3] & 0xF0;
		tasBuf[1] = demoBuf[3] & 0x0F;
		tasBuf[2] = demoBuf[1];
		tasBuf[3] = demoBuf[2];

		while (demoBuf[0]--) {
			fwrite(tasBuf, 4, 1, tasFile);
			inputs++;
		}
	}

	fseek(tasFile, 0x00C, SEEK_SET);
	fwrite("\xFF\xFF\xFF\xFF", 4, 1, tasFile);
	fwrite("\x00\x00\x00\x00", 4, 1, tasFile);
	fwrite("\x3C\x01\x00\x00", 4, 1, tasFile);
	fwrite((char *) &inputs, 4, 1, tasFile);
	fwrite("\x01\x00\x00\x00", 4, 1, tasFile);
	fwrite("\x01\x00\x00\x00", 4, 1, tasFile);

	fseek(tasFile, 0x300, SEEK_SET);
	fwrite(basename(demo), 0x100, 1, tasFile);

	fclose(demoFile);
	fclose(tasFile);
}

int tasToDemo(char *demo, char *tas, int levelNum) {
	FILE *demoFile = fopen(demo, "wb");
	FILE *tasFile = fopen(tas, "rb");

	if (demoFile == NULL || tasFile == NULL) {
		fprintf(stderr, "Could not open file %s\n", demoFile == NULL ? demo : tas);
		return 1;
	}

	fseek(tasFile, 0x400, SEEK_SET);

	fwrite((char *) &levelNum, 4, 1, demoFile);

	unsigned char demoBuf[4];
	unsigned int tasBuf;

	union {
		unsigned char buf[4];
		unsigned int asInt;
	} last;

	unsigned char count = 0;

	while (true) {
		fread((char *) &tasBuf, 4, 1, tasFile);

		if (count) {
			if (!feof(tasFile) && tasBuf == last.asInt && count < 0xFF) {
				count++;
				continue;
			} else {
				demoBuf[0] = count;
				demoBuf[1] = last.buf[2];
				demoBuf[2] = last.buf[3];
				demoBuf[3] = (last.buf[0] & 0xF0) | (last.buf[1] & 0x0F);

				fwrite(demoBuf, 4, 1, demoFile);

				if (feof(tasFile)) {
					break;
				}
			}
		}

		last.asInt = tasBuf;
		count = 1;
	}

	fwrite("\x00\x00\x00\x00", 4, 1, demoFile);

	fclose(demoFile);
	fclose(tasFile);
}

int usage(char *filename) {
	fprintf(stderr, "Usage: %s <demo file> [--to/--from] <tas file> [level num]\n", filename);
	return 1;
}

int main(int argc, char **argv) {
	if (argc < 4) {
		return usage(argv[0]);
	}

	if (strcmp(argv[2], "--to") == 0 && argc == 4) {
		return demoToTas(argv[1], argv[3]);
	} else if (strcmp(argv[2], "--from") == 0 && argc == 5) {
		return tasToDemo(argv[1], argv[3], strtol(argv[4], NULL, 0));
	} else {
		return usage(argv[0]);
	}
}
