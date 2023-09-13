// txt2wav.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define PCM 1
#define BUFSIZE (1000)

#ifndef UCHAR_MAX
#define UCHAR_MAX (255)
#endif

#ifndef UCHAR_MIN
#define UCHAR_MIN (0)
#endif

#ifndef SHRT_MAX
#define SHRT_MAX (32767)
#endif

#ifndef SHRT_MIN
#define SHRT_MIN (-32768)
#endif

#ifndef _MAX_PATH
#define _MAX_PATH (255)
#endif

static char cmdname[] = "txt2wav";

void usage(void)
	{
	fprintf(stderr,"\n""ASCII �ƥ����ȥǡ��� ---> wav �ե����ޥå��Ѵ��ץ����...\n"
	"�Ȥ��� : %s [sampling_rate] <text_file> [wav_file]\n""\t sampling_rate �� "
	"- ��³���ƥ���ץ�󥰥졼�� (ñ�� Hz) ����ꤷ�ޤ�\n""\t (wav_file �λ��̵꤬����� "
	"text_file.wav �Υե����뤬��������ޤ�)\n""\n"
	"�� : %s -44100 filename.txt\n"" (filename.txt �Υǡ����� 44.1kHz �� ""filename.wav �˽񤭽Ф��ޤ�)\n"
	" %s -48000 filename.txt output.wav\n"" (filename.txt �Υǡ����� 48kHz �� ""output.wav �˽񤭽Ф��ޤ�)\n""\n"
	"�ǡ����� 0���255 �ʤ�� 8bit wav ����Ϥ��ޤ�\n"
	"����ʳ��ʤ�� 16bit wav ����Ϥ��ޤ�\n"
	"( 8bit �ǡ��� : 0��� +255, ̵��=128)\n"
	"(16bit �ǡ��� : -32768���+32767, ̵��= 0)\n"
	"\n"
	"�ƥ����ȥǡ������� : \n"
	"\t1ch(��Υ��) �ξ�� 2ch(���ƥ쥪) �ξ��\n"
	"\tdata Lch Rch\n"
	"\tdata Lch Rch\n"
	"\t : : :\n"
	"\n",
	cmdname, cmdname, cmdname);
	exit(1);
	}

void openerror(char *filename)
	{
	fprintf(stderr, "Can't open file: %s\n", filename);
	exit(1);
	}

void formaterror(FILE *fp)
	{
	fprintf(stderr, "File format error : %ld\n", ftell(fp));
	exit(1);
	}

int getbasename(char *dest, char *src)
	{
	int i, start, end, ret;
	i = -1;
	start = 0;
	end = 0;
	// �ե�����̾�ΤϤ���Ƚ����򸡽�
	while (src[++i])
 		{
		if (src[i] == '\\' || src[i] ==':') 
			{	start = i + 1;		end = 0;		}
		if (src[i] == '.') 
			{	end = i;			}
		}
	if (end == 0) 
		{	end = i;	}

	// �ե�����̾��ͭ����
	if (start < end)
	 	{
		for (i = 0; i < end; i++) 
			{
			dest[i] = src[i];
			}
		dest[i] = '\0';
		ret = 1;
		}
		else
		{
		dest[0] = '\0';
		ret = 0;
		}
	return ret;
	}

unsigned long filesize(FILE *fp)
	{
	int c;
	long count = 0;
	rewind(fp);
	while ((c = getc(fp)) != EOF) 
		{
		if (c == '\n') {	count++;	}
		}
	return count;
	}


short bytecheck(FILE *fp)
	{
	int data;
	char buf[BUFSIZE];
	rewind(fp);
	while (fscanf(fp, "%s", buf) != EOF) 
		{
		data = strtol(buf, (char **) NULL, 10);
		if (UCHAR_MAX < data) 
			{	return 2;	}
		else if (data < UCHAR_MIN) 
			{	return 2; }
		}

	// �ǡ����� 0���255 �ʤ�� 1byte(8bit)
	return 1;
	}


short chcheck(FILE *fp)
	{
	int a, b;
	short ch;
	char buf[BUFSIZE];
	char s[2];

	rewind(fp);
	fgets(buf, BUFSIZE, fp);
	ch = sscanf(buf, "%d %d %1s", &a, &b, s);
	if (ch == 1 || ch == 2)
		{	;	}
	else
		{
		fprintf(stderr, "channel must be 1 or 2\n");
		formaterror(fp);
		}
	return ch;
	}


short datacheck(short data, short max, short min)
	{
	// �����С��ե������� + �� - ��ɽ��
	if (data > max) {
		fprintf(stderr, "+");
		data = max;
		}
	if (data < min) {
		fprintf(stderr, "-");
		data = min;
		}
	return data;
	}


long datawrite(unsigned short ch, unsigned short byte, FILE *txt, FILE *wav)
	{
	short left, right;
	int n;
	unsigned long count = 0;
	char s[2], buf[BUFSIZE];

	rewind(txt);

	while ((fgets(buf, BUFSIZE, txt) != NULL)) {
		n = sscanf(buf, "%hd %hd %1s", &left, &right, s);
		if (feof(txt)) 	{	break;		}
		if (ch != n) 	{	formaterror(txt);	}
	// ��Υ��
		else if (ch == 1) 
		{
	// 8 �ӥå�
			if (byte == 1) {
			datacheck(left, UCHAR_MAX, UCHAR_MIN);
			fputc(left, wav);
			count++;
			}
	// 16 �ӥå�
			else if (byte == 2) {
			datacheck(left, SHRT_MAX, SHRT_MIN);
			fwrite(&left, sizeof(short), 1, wav);
			count++;
			}
		}
	// ���ƥ쥪
		else if (ch == 2) 
		{
	// 8 �ӥå�
			if (byte == 1) {
			datacheck(left, UCHAR_MAX, UCHAR_MIN);
			datacheck(right, UCHAR_MAX, UCHAR_MIN);
			fputc(left, wav);
			fputc(right, wav);
			count++;
			}

	// 16 �ӥå�
			else if (byte == 2) {
			datacheck(left, SHRT_MAX, SHRT_MIN);
			datacheck(right, SHRT_MAX, SHRT_MIN);
			fwrite(&left, sizeof(short), 1, wav);
			fwrite(&right, sizeof(short), 1, wav);
			count++;
			}
		}

		else 	{	formaterror(txt);	}
		}
	return count;
	}


unsigned long sampling(void)
{
unsigned long sr = 0;
char buf[BUFSIZE];
while (sr == 0) {
fprintf(stderr, "sampling rate [Hz] : ");
fgets(buf, BUFSIZE, stdin);
sr = labs(strtoul(buf, (char **) NULL, 10));
}
return sr;
}
unsigned long parameter(FILE *fp,
unsigned long *points,
unsigned short *channel,
unsigned short *byte)
{
// �ǡ�����
*points = filesize(fp);
// �̻Ҳ��Х��ȿ� 1Byte = 8bit, 2Byte = 16bit
*byte = bytecheck(fp);
// �����ͥ��
*channel = chcheck(fp);
return (unsigned long) (*points * *byte * *channel);
}
void wavwrite(char *datafile, char *wavfile, unsigned long sr)
{
unsigned long points, file_size, count, var_long;
unsigned short ch, bytes, var_short;
char s[4];
FILE *fp1, *fp2;
// �ƥ����ȥե�����
if ((fp1 = fopen(datafile, "r")) == NULL) {
openerror(datafile);
}
file_size = parameter(fp1, &points, &ch, &bytes);
printf("%s : %ld Hz sampling, %d bit, %d channel\n",
wavfile, sr, bytes * 8, ch);
// WAV �ե�����


if ((fp2 = fopen(wavfile, "wb")) == NULL) {
openerror(wavfile);
}
// RIFF �إå�
s[0] = 'R';
s[1] = 'I';
s[2] = 'F';
s[3] = 'F';
fwrite(s, 1, 4, fp2);
// �ե����륵����
var_long = file_size + 36;
fwrite(&var_long, sizeof(long), 1, fp2);
printf(" file size (header + data) = %ld [Byte]\n", var_long);
// WAVE �إå�
s[0] = 'W';
s[1] = 'A';
s[2] = 'V';
s[3] = 'E';
fwrite(s, 1, 4, fp2);
// chunkID (fmt �����)
s[0] = 'f';
s[1] = 'm';
s[2] = 't';
s[3] = ' ';
fwrite(s, 1, 4, fp2);
// chunkSize (fmt ����󥯤ΥХ��ȿ� ̵���� wav �� 16)
var_long = 16;
fwrite(&var_long, sizeof(long), 1, fp2);
// wFromatTag (̵���� PCM = 1)
var_short = PCM;
fwrite(&var_short, sizeof(short), 1, fp2);
// dwChannels (��Υ�� = 1, ���ƥ쥪 = 2)
fwrite(&ch, 2, 1, fp2);
printf(" PCM type = %hu\n", var_short);
// dwSamplesPerSec (����ץ�󥰥졼�� (Hz))
fwrite(&sr, sizeof(long), 1, fp2);
printf(" sampling rate = %ld [Hz]\n", sr);
// wdAvgBytesPerSec (Byte/��)
var_long = bytes * ch * sr;
fwrite(&var_long, sizeof(long), 1, fp2);
printf(" Byte / second = %ld [Byte]\n", var_long);
// wBlockAlign (Byte/����ץ�*�����ͥ�)
var_short = bytes * ch;
fwrite(&var_short, sizeof(short), 1, fp2);
printf(" Byte / block = %hu [Byte]\n", var_short);
// wBitsPerSample (bit/����ץ�)
var_short = bytes * 8;

fwrite(&var_short, sizeof(short), 1, fp2);
printf(" bit / sample = %hu [bit]\n", var_short);
// chunkID (data �����)
s[0] = 'd';
s[1] = 'a';
s[2] = 't';
s[3] = 'a';
fwrite(s, 1, 4, fp2);
// chunkSize (�ǡ���Ĺ Byte)
fwrite(&file_size, 4, 1, fp2);
printf(" file size (data) = %ld [Byte]\n", file_size);
// �إå� (44 Byte) �񤭹��ߤ����ޤ�
if (ftell(fp2) != 44) {
fprintf(stderr, "%s : wav header write error\n", wavfile);
exit(1);
}
// waveformData (�ǡ���) �񤭹���
count = datawrite(ch, bytes, fp1, fp2);
// �ǡ����񤭹��ߤ����ޤ�
if (count != points) {
fprintf(stderr, "%s : data write error\n", wavfile);
exit(1);
}
fclose(fp1);
fclose(fp2);
printf(" %ld points write\n", count);
printf("%s : %ld bytes (%g sec)\n",
wavfile, count * bytes * ch + 44, (double) count / sr);
}


int main(int argc, char *argv[])
{
int ac = 0, optc = 0;
unsigned long sr = 0;
char txtfile[_MAX_PATH], wavfile[_MAX_PATH];
	if (argc != 1) {
	int i;
	ac = argc - 1;
	for (i = 1; i < argc; i++) {
	// ����ץ�󥰥졼�Ȼ��ꤢ��
			if (argv[i][0] == '-') {
			sr = labs(strtoul(&argv[i][1], (char **) NULL, 10));

			ac = argc - 2;
			optc = 1;	
			}	
		}
	}
	else
	usage();

// ���ޥ�ɥ����å� (���ץ����ʳ��ΰ���)
if (ac < 1) {
usage();
}
else if (ac == 1) {
char basename[_MAX_PATH];
strcpy(txtfile, argv[optc + 1]);
getbasename(basename, argv[optc + 1]);
sprintf(wavfile, "%s.wav", basename);
}
else if (ac == 2) {
strcpy(txtfile, argv[optc + 1]);
strcpy(wavfile, argv[optc + 2]);
}
else {
usage();
}
// ����ץ�󥰥졼��
if (sr == 0) {
sr = sampling();
}
wavwrite(txtfile, wavfile, sr);
return 0;
}


