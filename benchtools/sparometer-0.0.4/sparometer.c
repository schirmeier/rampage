/*
 * Copyright © 2008 by Hans Schou.
 * License: GPL
 *
 * For further reading:
 * http://sparel.teknologisk.dk/index.php?id=42 COM_Class.cs
 * http://www.easysw.com/~mike/serial/serial.html#5_1_1
 *
 * Compile:
 *   gcc -Wall -o simplesparometer simplesparometer.c
 *
 */

/*
Example of output:

./sparometer --device=/dev/tts/0

./sparometer --get-sample-effect --get-sample-current --get-sample-voltage --loop=10 --time-stamp --delay=0.5

./sparometer --set-interval=01*min --set-date=$(date +%y%m%d) --set-time=$(date +%H%M%S) --get-date --get-time
./sparometer --reset-log
# wait a minute
./sparometer --get-logdata
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <getopt.h>

#include "config.h"

/*
#ifndef DEBUG
#	define DEBUG 0
#endif
#define DEBUG 1
*/

static char *o_date = "991231";
static char *o_interval = "01*min";
static char *o_owner_nr = "99";
static char *o_periode_effect = "1";
static char *o_tarif = "1.550";
static char *o_text = "SPAROMETER";
static char *o_time = "235959";

static char *o_help = "0";
static char *o_debug = "0";
static char *o_delay = "0.0";
static char *o_device = SDEVICE;
static char *o_loop = "1";
static char *o_time_stamp = "0";
static char *o_verbose = "0";

static int help = 0;
static int debug = DEBUG;
static struct timeval delay = {0,0};
static int loop = 1;
static int time_stamp = 0;
static int verbose = 0;

static const char *cs_data_readout         = "\x06" "030" "\x0D" "\x0A";
static const char *cs_get_date             = "\x01" "R1" "\x02" "0.9.2()" "\x03";
static const char *cs_get_interval         = "\x01" "R1" "\x02" "0.8.4()" "\x03";
static const char *cs_get_kwh              = "\x01" "R1" "\x02" "1.8.1()" "\x03";
static const char *cs_get_log_time         = "\x01" "R1" "\x02" "130.130()" "\x03";
static const char *cs_get_logdata          = "\x01" "R5" "\x02" "P.01(;)" "\x03";
static const char *cs_get_max_effect       = "\x01" "R1" "\x02" "1.6.1()" "\x03";
static const char *cs_get_min_effect       = "\x01" "R1" "\x02" "1.3.1()" "\x03";
static const char *cs_get_owner_nr         = "\x01" "R1" "\x02" "0.0.0()" "\x03";
static const char *cs_get_periode_effect   = "\x01" "R1" "\x02" "130.128()" "\x03";
static const char *cs_get_price_periode    = "\x01" "R1" "\x02" "129.130()" "\x03";
static const char *cs_get_price_year       = "\x01" "R1" "\x02" "129.129()" "\x03";
static const char *cs_get_productid        = "/?!" "\x0D" "\x0A";
static const char *cs_get_sample_current   = "\x01" "R1" "\x02" "11.7()" "\x03";
static const char *cs_get_sample_effect    = "\x01" "R1" "\x02" "1.7.1()" "\x03";
static const char *cs_get_sample_freq      = "\x01" "R1" "\x02" "14.7()" "\x03";
static const char *cs_get_sample_voltage   = "\x01" "R1" "\x02" "12.7()" "\x03";
static const char *cs_get_serial           = "\x01" "R1" "\x02" "96.1.0()" "\x03";
static const char *cs_get_software_version = "\x01" "R1" "\x02" "0.2.0()" "\x03";
static const char *cs_get_tarif            = "\x01" "R1" "\x02" "129.128()" "\x03";
static const char *cs_get_text             = "\x01" "R1" "\x02" "128.128()" "\x03";
static const char *cs_get_time             = "\x01" "R1" "\x02" "0.9.1()" "\x03";
static const char *cs_get_up_time          = "\x01" "R1" "\x02" "130.131()" "\x03";
static const char *cs_get_weekday          = "\x01" "R1" "\x02" "0.9.5()" "\x03";
static const char *cs_init_end             = "\x01" "B0" "\x03";
static const char *cs_manufacturer_mode    = "\x06" "056" "\x0D" "\x0A";
static const char *cs_periode_log          = "\x01" "W1" "\x02" "130.128(0*Tage)" "\x03";
static const char *cs_programming_mode2400 = "\x06" "031" "\x0D" "\x0A";
static const char *cs_programming_mode300  = "\x06" "001" "\x0D" "\x0A";
static const char *cs_programming_mode4800 = "\x06" "041" "\x0D" "\x0A";
static const char *cs_programming_mode9600 = "\x06" "051" "\x0D" "\x0A";
static const char *cs_reset_log            = "\x01" "E1" "\x02" "131.132(zero)" "\x03";
static const char *cs_set_date             = "\x01" "W1" "\x02" "0.9.2(" "%-6s" ")" "\x03"; /* arg: str_date; 'YYMMDD' '991231' */
static const char *cs_set_interval         = "\x01" "W1" "\x02" "0.8.4(" "%-6s" ")" "\x03"; /* arg: interval; '01*min' */
static const char *cs_set_owner_nr         = "\x01" "W1" "\x02" "96.1.0(" "%s" ")" "\x03"; /* arg: str_nr; 'NN' '00' max_length=? */
static const char *cs_set_periode_effect   = "\x01" "W1" "\x02" "130.128()" "\x03";
static const char *cs_set_tarif            = "\x01" "W1" "\x02" "129.128(" "%s" ")" "\x03"; /* '1.550' */
static const char *cs_set_text             = "\x01" "W1" "\x02" "128.128(" "%s" ")" "\x03"; /* 30 characters */
static const char *cs_set_time             = "\x01" "W1" "\x02" "0.9.1(" "%-6s" ")" "\x03"; /* arg: str_time; 'HHMMSS' '235959' */
static const char *cs_start_log            = "\x01" "E1" "\x02" "131.130(start)" "\x03";
static const char *cs_stop_log             = "\x01" "E1" "\x02" "131.131(stop)" "\x03";

typedef enum {
	non = 0,
	req = 1,
	opt = 2
} has_arg_t;

typedef enum {
	none,
	p300,	/* Programming mode 300 baud */
	p9600,	/* Programming mode 9600 baud */
	m9600,	/* Manufactor mode 9600 baud */
	d300	/* Data readout 300 baud */
} mode_baud_t;

struct command_serial_t {
	const char *name;
	has_arg_t has_arg;
	int exec_order;
	int short_option;
	void *command_arg;
	mode_baud_t mode_baud;
	char response_term_char;
	const char **command_data;
	char *helptext;
};

static struct command_serial_t command_serial[] = {
	{ "data-readout",         non, 0,   0, NULL,              d300,  '\x03', &cs_data_readout,         "data_readout" },
	{ "get-date",             non, 0, 'd', NULL,              p300,  '\x03', &cs_get_date,             "get_date" },
	{ "get-interval",         non, 0, 'I', NULL,              p300,  '\x03', &cs_get_interval,         "get_interval" },
	{ "get-kwh",              non, 0, 'H', NULL,              p300,  '\x03', &cs_get_kwh,              "get_kwh" },
	{ "get-log-time",         non, 0,   0, NULL,              p300,  '\x03', &cs_get_log_time,         "get_log_time" },
	{ "get-logdata",          non, 0, 'l', NULL,             p9600,  '\x03', &cs_get_logdata,          "get_logdata" },
	{ "get-max-effect",       non, 0, 'X', NULL,              p300,  '\x03', &cs_get_max_effect,       "get_max_effect" },
	{ "get-min-effect",       non, 0, 'N', NULL,              p300,  '\x03', &cs_get_min_effect,       "get_min_effect" },
	{ "get-owner-nr",         non, 0, 'O', NULL,              p300,  '\x03', &cs_get_owner_nr,         "get_owner_nr" },
	{ "get-periode-effect",   non, 0, 'E', NULL,              p300,  '\x03', &cs_get_periode_effect,   "get_periode_effect" },
	{ "get-price-periode",    non, 0, 'P', NULL,              p300,  '\x03', &cs_get_price_periode,    "get_price_periode" },
	{ "get-price-year",       non, 0, 'Y', NULL,              p300,  '\x03', &cs_get_price_year,       "get_price_year" },
	{ "get-productid",        non, 0, 'i', NULL,              p300,  '\x0A', &cs_get_productid,        "get_productID '/nzr4SEM16_S_v1.12'" },
	{ "get-sample-current",   non, 0, 'A', NULL,             p9600,  '\x03', &cs_get_sample_current,   "get_sample_current" },
	{ "get-sample-effect",    non, 0, 'w', NULL,             p9600,  '\x03', &cs_get_sample_effect,    "get_sample_effect --get-sample-effect --loop=3 --delay=1" },
	{ "get-sample-freq",      non, 0, 'f', NULL,             p9600,  '\x03', &cs_get_sample_freq,      "get_sample_freq" },
	{ "get-sample-voltage",   non, 0, 'v', NULL,             p9600,  '\x03', &cs_get_sample_voltage,   "get_sample_voltage" },
	{ "get-serial",           non, 0, 's', NULL,              p300,  '\x03', &cs_get_serial,           "get_serial" },
	{ "get-software-version", non, 0,   0, NULL,              p300,  '\x03', &cs_get_software_version, "get_software_version" },
	{ "get-tarif",            non, 0, 'F', NULL,              p300,  '\x03', &cs_get_tarif,            "get_tarif" },
	{ "get-text",             non, 0, 't', NULL,              p300,  '\x03', &cs_get_text,             "get_text" },
	{ "get-time",             non, 0, 'M', NULL,              p300,  '\x03', &cs_get_time,             "get_time" },
	{ "get-up-time",          non, 0, 'U', NULL,              p300,  '\x03', &cs_get_up_time,          "get_up_time" },
	{ "get-weekday",          non, 0, 'y', NULL,              p300,  '\x03', &cs_get_weekday,          "get_weekday" },
	{ "init-end",             non, 0,   0, NULL,              p300,  '\x03', &cs_init_end,             "init_end" },
	{ "manufacturer-mode",    non, 0, 'm', NULL,             m9600,  '\x06', &cs_manufacturer_mode,    "manufacturer_mode" },
	{ "periode-log",          non, 0,   0, NULL,             p9600,  '\x03', &cs_periode_log,          "periode_log" },
	{ "programming-mode2400", non, 0, '2', NULL,              p300,  '\x03', &cs_programming_mode2400, "programming_mode2400" },
	{ "programming-mode300",  non, 0, '3', NULL,              p300,  '\x03', &cs_programming_mode300,  "programming_mode300" },
	{ "programming-mode4800", non, 0, '4', NULL,              p300,  '\x03', &cs_programming_mode4800, "programming_mode4800" },
	{ "programming-mode9600", non, 0, '9', NULL,              p300,  '\x03', &cs_programming_mode9600, "programming_mode9600" },
	{ "reset-log",            non, 0, 'r', NULL,             m9600,  '\x06', &cs_reset_log,            "Clear all logging data stored internal." },
	{ "set-date",             opt, 0, 'D', &o_date,           p300,  '\x03', &cs_set_date,             "Set date in YYMMDD format. If no date given current day will be used. Syntax: --set-date[=<YYMMDD>]. --set-date=$(date +%y%m%d)" },
	{ "set-interval",         req, 0,   0, &o_interval,       p300,  '\x03', &cs_set_interval,         "set_interval" },
	{ "set-owner-nr",         req, 0,   0, &o_owner_nr,       p300,  '\x03', &cs_set_owner_nr,         "set_owner_nr" },
	{ "set-periode-effect",   req, 0, 'S', &o_periode_effect, p300,  '\x03', &cs_set_periode_effect,   "set_periode_effect" },
	{ "set-tarif",            req, 0, 'R', &o_tarif,          p300,  '\x03', &cs_set_tarif,            "set_tarif" },
	{ "set-text",             req, 0, 'T', &o_text,           p300,  '\x03', &cs_set_text,             "set_text max 30 ASCII characters except '()'. --set-text=$(hostname -f)" },
	{ "set-time",             opt, 0, '1', &o_time,           p300,  '\x03', &cs_set_time,             "Set time in HHMMSS format. If no time given current time will be used. Syntax: --set-time[=<HHMMSS>]. Example: --set-time=$(date +%H%M%S)" },
	{ "start-log",            non, 0, 'a', NULL,             m9600,  '\x03', &cs_start_log,            "start_log" },
	{ "stop-log",             non, 0, 'o', NULL,             m9600,  '\x03', &cs_stop_log,             "stop_log" },
	{ "delay",                req, 0,   0, &o_delay,          none,       0, NULL,                     "Delay in seconds between executing commands. Example: --delay=1.234567. If no delay is given together with the 'loop' command, there will be no delay. Due to slow communication and A/D conversion the minimum delay is about 0.125 seconds at 9600 baud." },
	{ "device",               req, 0,   0, &o_device,         none,       0, NULL,                     "List of serial port devices to try to open. --device=<serial>[:<serial>]. Example: --device=/dev/ttyS0:/dev/ttyS1" },
	{ "debug",                opt, 0,   0, &o_debug,          none,       0, NULL,                     "Turn on debug." },
	{ "help",                 non, 0, 'h', &o_help,           none,       0, NULL,                     "This help. Use '--verbose' for more details." },
	{ "loop",                 opt, 0,   0, &o_loop,           none,       0, NULL,                     "Loop commands given. 'loop=0' equals loop for ever." },
	{ "time-stamp",           non, 0,   0, &o_time_stamp,     none,       0, NULL,                     "Prefix each line with a time stamp. 0=false, 1=true." },
	{ "verbose",              opt, 0,   0, &o_verbose,        none,       0, NULL,                     "Turn on verbose." },
	{ NULL }
};

static const struct timeval timeval_1_0 = {1,0};
static const struct timeval timeval_2_0 = {2,0};

struct timeval atotimeval(char *argv) {
	struct timeval temp = {0, 0};
	char *usec;
	char u2[] = "000000";
	int i = 0;

	temp.tv_sec = atol(argv);
	if ( (usec = strstr(argv, ".")) != NULL ) {
		usec++;
		while (*usec && isdigit(*usec) && i < strlen(u2)) {
			u2[i++] = *usec++;
		}
		temp.tv_usec = atol(u2);
	}

	return temp;
}

/* Command Que - ordered linked list
 * Only sparometer commands are qued.
 */

struct command_que_t {
	int idx;
	struct command_que_t *next;
};

static struct command_que_t
	*cq_head = NULL,
	*cq_tail = NULL;

int cq_add(int idx) {
	struct command_que_t *this = (struct command_que_t *)malloc(sizeof(struct command_que_t));
	this->next = NULL;
	this->idx = idx;
	if (cq_tail) {
		cq_tail->next = this;
	} else {
		cq_head = this;
	}
	cq_tail = this;

	return idx;
}

void setdatetime(char **date, char **time)  {
	struct timeval now;
	time_t curtime;

	char *mdate = malloc(7); /* TODO: free this later */
	char *mtime = malloc(7); /* TODO: free this later */

	gettimeofday(&now, NULL);
	curtime=now.tv_sec;

	strftime(mdate, 7, "%y%m%d", localtime(&curtime));
	strftime(mtime, 7, "%H%M%S", localtime(&curtime));
	*date = mdate;
	*time = mtime;
}

/*
 * Pre-parse option arguments.
 * If device specified in environment use it.
 */
int pre_parse_arguments(char *argv0) {
	char *p = argv0 + strlen(argv0) -1;
	char *device;

	setdatetime(&o_date, &o_time);

	/* get program filename, strip backwards to slash */
	while (p != argv0 && *p != '/') {
		p--;
	}
	p++;

	device = getenv(p);
	if (device) {
		if (DEBUG) printf("Device: %s\n", device);
		o_device = device;
	}

	return device != NULL;
}

/*
 * Parse arguments to global vars and command que.
 */
int parse_arguments(int argc, char *argv[]) {
	unsigned int i;
	int c;
	struct option long_options[(sizeof(command_serial)/sizeof(command_serial[0]))+1];
	char short_options[2*(sizeof(command_serial)/sizeof(command_serial[0]))];
	int so = 0;
	int option_index;
	int map_short_longidx[128];

	memset(&map_short_longidx, 0, sizeof(map_short_longidx));

	/* Copy command_serial data to a temporary long_options, 
	 * and call getopt_long() with this data.
	 */
	i = 0;
	while (command_serial[i].name != NULL) {
		long_options[i].name = command_serial[i].name;
		long_options[i].has_arg = command_serial[i].has_arg;
		long_options[i].flag = 0;
		long_options[i].val = 0;
		if (command_serial[i].short_option != 0) {
			short_options[so++] = command_serial[i].short_option;
			if (command_serial[i].has_arg) {
				short_options[so++] = ':';
			}
			if (DEBUG && map_short_longidx[command_serial[i].short_option]) {
				printf("PROGRAM ERROR, option argument '%c' occour twice\n", command_serial[i].short_option);
				exit(1);
			}
			map_short_longidx[command_serial[i].short_option] = i;
		}
		if (DEBUG) printf("\n");
		i++;
	}
	short_options[so] = '\0';
	long_options[i].name = NULL;
	if (DEBUG) printf("short_options: '%s'\n", short_options);

	i = 0;
	while ( (c = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1 ) {
		switch (c) {
			case 0:
				if (command_serial[option_index].exec_order) {
					fprintf(stderr, "Command already in command chain: '%s'\n", command_serial[option_index].name);
				} else {
					command_serial[option_index].exec_order = ++i;
					if (command_serial[option_index].command_data != NULL) {
						cq_add(option_index);
					}
					if (DEBUG) printf(">>option %s", long_options[option_index].name);
					if (optarg) {
						if (DEBUG) printf(" with arg '%s'", optarg);
						if (command_serial[option_index].has_arg) {
							if (command_serial[option_index].command_arg != NULL) {
								*(char **)command_serial[option_index].command_arg = optarg;
							} else {
								if (DEBUG) printf("'%s' has_arg but command_arg pointer is NULL\n", long_options[option_index].name); 
							}
						}
					} else {
						switch (command_serial[option_index].has_arg) {
							case non:
								if (DEBUG) printf("no optarg, and non required\n");
								if (command_serial[option_index].command_arg != NULL) {
									if (DEBUG) printf("Set '%s' == 1\n", long_options[option_index].name);
									*(char **)command_serial[option_index].command_arg = "1";
								}
								break;
							case req:
								if (DEBUG) printf("no optarg, and required\n");
								break;
							case opt:
								if (DEBUG) printf("no optarg, and optional\n");
								/* TODO Set its default value */
								/* only set internal vars to "1", not sparometer data */
								if (!command_serial[option_index].command_data) {
									*(char **)command_serial[option_index].command_arg = "1";
								}
								break;
						}
					}
					if (DEBUG) printf("\n");
				}
				break;
			default:
				if (DEBUG) {
					printf("default: %c", c);
					if (optarg) {
						printf(" '%s'", optarg);
					}
					printf(" %s", command_serial[map_short_longidx[c]].name );
					printf("\n");
				}
		}
	}

	/* Convert some options to integer or double */
	help = atoi(o_help);
	debug = atoi(o_debug);
	delay = atotimeval(o_delay);
	loop = atoi(o_loop);
	time_stamp = atoi(o_time_stamp);
	verbose = atoi(o_verbose);

	return 0;
}

/* Meassure time */

static struct timeval lastnow = {0,0};

void now(int echo, char *text) {
	struct timeval now;
	struct tm *tmdata;
	char textdata[200];
	time_t sec;
	suseconds_t usec;

	gettimeofday(&now, NULL);
	tmdata = gmtime(&now.tv_sec);
	if (!lastnow.tv_sec) {
		lastnow = now;
	}
	sec = now.tv_sec - lastnow.tv_sec;
	usec = now.tv_usec - lastnow.tv_usec;
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}
	snprintf((char *)textdata, sizeof(textdata)-1,
			"%04d-%02d-%02d %02d:%02d:%02d.%06ld %3ld.%06ld ",
			tmdata->tm_year+1900,
			tmdata->tm_mon+1,
			tmdata->tm_mday,
			tmdata->tm_hour,
			tmdata->tm_min,
			tmdata->tm_sec,
			now.tv_usec,
			sec,
			usec);
	if (text) {
		strncat((char *)textdata, text, sizeof(textdata)-1);
	}
	if (echo) {
		fprintf(stderr, "%s\n", textdata);
	}
	lastnow = now;

	return;
}

void formatnice(void *buf2, int count, char *b) {
	int i = 0;
	unsigned char *buf = (unsigned char *)buf2;
	int wrout = 0;
	int chrwritten = 0;

	*b = '\0';
	while (i < count) {
		switch (buf[i]) {
			case ')':
				wrout = 0;
				if (chrwritten) {
					sprintf(b, " ");
					++b;
				}
				break;
			case '(':
				wrout = 1;
				break;
			case '*':
				break;
			default:
				if (wrout && isprint(buf[i])) {
					sprintf(b, "%c", buf[i]);
					++b;
					++chrwritten;
				}
		}
		i++;
	}
	if (debug) {
		fprintf(stderr, " %d bytes i/o ", count);
		now(1, NULL);
	} else {
		if (chrwritten) {
			if (!time_stamp) {
				sprintf(b, "\n");
				++b;
			}
		}
	}
}

void formatverbose(void *buf2, int count, char *b) {
	int i = 0;
	unsigned char *buf = (unsigned char *)buf2;
	int chrwritten = 0;
	int open_quote = 0;

	*b = '\0';
	if (!count) {
		fprintf(stderr, "Error: 0 bytes in buffer\n");
	}
	while (i<count) {
		if (isprint(buf[i])) {
			if (!open_quote) {
				if (chrwritten) {
					sprintf(b, " ");
					b += strlen(b);
				}
				sprintf(b, "\"");
				b += strlen(b);
				open_quote = 1;
			}
			sprintf(b, "%c", buf[i]);
			++b;
			++chrwritten;
		} else {
			if (open_quote) {
				sprintf(b, "\"");
				++b;
				open_quote = 0;
			}
			if (chrwritten) {
				sprintf(b, " ");
				++b;
			}
			sprintf(b, "\"\\x%02x\"", buf[i]);
			b += strlen(b);
			++chrwritten;
			open_quote = 0;
		}
		i++;
	}
	if (open_quote) {
		sprintf(b, "\"");
		++b;
		open_quote = 0;
	}
	if (chrwritten) {
		sprintf(b, " ");
		++b;
		if (!time_stamp) {
			sprintf(b, "\n");
			++b;
		}
	}
}

void printbuf(void *buf, int count, int local_verbose) {
	if (count) {
		char *fmtbuf = (char *)malloc(count*10);
		if (local_verbose) {
			formatverbose(buf, count, fmtbuf);
		} else {
			formatnice(buf, count, fmtbuf);
		}
		fprintf( strstr(buf, "ERROR_") != NULL
			? stderr
			: stdout,
			fmtbuf);
		free(fmtbuf);
	}
}

void show_help(void) {
	int i = 0;
	struct command_serial_t *c;

	printf("%s version %s.\n", PACKAGE, VERSION);
	printf("Copyright (C) %s by Hans Schou\n", YEAR);
	printf("Build date: %s\n", ISODATE);
	printf("\n");
	while (command_serial[i].name) {
		c = &command_serial[i];
		printf("  ");
		if (0 /* TODO */ && c->short_option) {
			printf("-%c", c->short_option);
			if (c->has_arg != non) {
				printf(" <%s>", *(char **)c->command_arg);
			}
			printf(", ");
		}
		printf("--%s", c->name);
		switch(c->has_arg) {
			case non:
				break;
			case req:
				printf("=<%s>", *(char **)c->command_arg);
				break;
			case opt:
				printf("[=<%s>]", *(char **)c->command_arg);
				break;
		}
		printf("\n");
		if (c->command_data && verbose) {
			printf("\tSpar-o-meter internal command: ");
			printbuf((char *)*c->command_data, strlen(*c->command_data), 1);
		}
		if (c->mode_baud != none) {
			printf("\tMode: ");
			switch (c->mode_baud) {
				case p300:
				case p9600:
					printf("programming");
					break;
				case m9600:
					printf("manufactor");
					break;
				case d300:
					printf("data-readout");
					break;
				default:
					printf("error no mode");
			}
			switch (c->mode_baud) {
				case p300:
				case d300:
					printf(" 300");
					break;
				case p9600:
				case m9600:
					printf(" 9600");
					break;
				default:
					fprintf(stderr, "Programmer error line %d: missing baud '%d'\n", __LINE__, c->mode_baud);
			}
			printf(" baud\n");
		}
		printf("\t%s\n", c->helptext);
		printf("\n");
		i++;
	}
	printf("ENVIRONMENT\nSet environment variable to '" PACKAGE_NAME "=%s' for default serial port instead of the one specified during './configure --enable-serial-device=/dev/ttyS0'.\n\n", o_device);
	printf("Send bug reports to: <" PACKAGE_BUGREPORT ">\n");
}

static struct timeval delay_start = {0,0};

void sleep_timeval(struct timeval d) {
	struct timeval timenow, dif;

	timeradd(&d, &delay_start, &delay_start);
	gettimeofday(&timenow, NULL);
	timersub(&delay_start, &timenow, &dif);
	if (debug) printf("Delay: %ld.%06ld\n", dif.tv_sec, dif.tv_usec );
	if (dif.tv_sec >= 0 && dif.tv_usec > 0) {
		select(0, NULL, NULL, NULL, &dif);
	} else {
		if (debug && d.tv_sec+d.tv_usec>0) fprintf(stderr, "Requested delay too short: %ld.%06ld\n", d.tv_sec, d.tv_usec);
	}
}

void printcurtime(void) {
	struct timeval tv;
	static char buffer[30];
	time_t curtime;

	gettimeofday(&tv, NULL); 
	curtime=tv.tv_sec;

	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&curtime));
	printf("%s.%06ld ", buffer, tv.tv_usec);
}

int waitdatawritten(int fd, struct timeval tv) {
	fd_set wfds;
	int retval;

	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);
	retval = select(fd + 1, NULL, &wfds, NULL, &tv);
	if (debug > 1) {
		printf("retval: %d, tv:%ld.%ld, FD_ISSET(fd): %d\n",
			retval,
			tv.tv_sec,
			tv.tv_usec,
			FD_ISSET(fd, &wfds));
	}

	return retval;
}

int writecom(int fd, const void *buf) {
	return write(fd, buf, strlen(buf) );
}

void e(int retcode, char *msg) {
	if (retcode != 0) {
		fprintf(stderr, "Error rc=%d on '%s' %d %s\n", retcode, msg, errno, strerror(errno));
		exit(1);
	}
}

/*
 * Terminating characters from Sparometer.
 * Some of these chars is probably not send by the meter
 * but it has been found that they are usefull for terminating a block.
 */
int istermchar(char c) {
	int found = 0;
	switch (c) {
		case '~': /* After change baud */
		case '^': /* After change baud */
		case 0x00: /* After change baud */
		case 0x03: /* Most get-commands */
		case 0x06: /* After writing data , e.g. set-text */
		case 0x0A: /* From get-productid and get-logdata */
			found = 1;
			break;
	}
	return found;
}

/*
 * Read from fd until terminating character or time-out
 * Returns number of bytes read or error.
 *
 */
ssize_t readwait(int fd, void *buf, size_t count) {
	char *buffer = buf;
	size_t num = 0;
	size_t idx = 0;
	fd_set rfds;
	int retval = 1;
	int keepread = 1;
	struct timeval tv;

	while (idx < count && retval > 0 && keepread) {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv = timeval_1_0;
		retval = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (retval > 0) {
			num = read(fd, &buffer[idx], count-idx);
			if (num > 0) {
				idx += num;
				if (idx > 0 && istermchar(buffer[idx-1])) {
					keepread = 0;
				}
			} else {
				keepread = 0;
			}
		}
	}

	return retval < 0
		? retval
		: ( num  < 0 
			? num 
			: idx);
}

enum error_response_t {
	error_invalid_data    = 0,
	error_invalid_command = 1,
	error_invalid_address = 2,
	error_no_mode_started = 3,
	error_no_profile      = 4,
	no_error              = 5
};

enum error_action_t {
	action_bail_out,
	action_keep_try
};

struct error_msg_t {
	enum error_response_t error_response;
	char *msg;
	enum error_action_t action;
	char *msg_action;
};

static struct error_msg_t error_msg[] = {
	{ error_invalid_data,    "ERROR_invalid_data",    action_keep_try, 0 },
	{ error_invalid_command, "ERROR_invalid_command", action_keep_try, 0 },
	{ error_invalid_address, "ERROR_invalid_address", action_bail_out, 0 },
	{ error_no_mode_started, "ERROR_No_mode_started", action_bail_out, 0 },
	{ error_no_profile,      "ERROR_no_profile",      action_bail_out, 0 },
	{ no_error,              0,                       action_bail_out, 0 }
};

enum error_response_t get_error_response(char *haystack) {
	int i = 0;

	if (strstr(haystack, "ERROR_") != NULL) {
		while  (error_msg[i].error_response != no_error) {
			if (strstr(haystack, error_msg[i].msg) != NULL) {
				fprintf(stderr, " %s ", error_msg[i].msg);
				return error_msg[i].error_response;
			}
			++i;
		}
	}

	return no_error;
}

/*
 * Write serial port and print result
 */
int wp(int fd, const void *buf) {
	char buffer[1024];
	enum error_response_t resp;
	int num = 0;
	int try = 5;

	while (try--) {
		if (writecom(fd, buf)) {
			if (verbose) {
				printf("\nWrite: ");
				printbuf((char *)buf, strlen(buf), 1);
			}
			num = readwait(fd, &buffer, sizeof(buffer));
			if (num) {
				resp = get_error_response((char *)&buffer);
				if (resp == no_error || verbose) {
					if (verbose) {
						printf("Read : ");
					}
					printbuf(&buffer, num, verbose);
				}
				if (error_msg[resp].action == action_bail_out) {
					try = 0;
				}
			}
		}
	}

	return try;
}

int wpa(int fd, const void *buf, const char *arg) {
	char buffer[1024];

	if (debug) printf("arg: '%s'\n", arg);
	snprintf((char *)&buffer, sizeof(buffer), buf, arg);

	return wp(fd, &buffer);
}

/*
 * Set baud rate
 *   setbaud(B300);
 *   setbaud(B9600);
 */
void setbaud(int fd, tcflag_t baud) {
	struct termios term;

	e(tcgetattr(fd, &term), "TCGETATTR");
	/* 300 baud, 7 data bits, 1 stop bit, even parity */
	term.c_cflag = baud | CS7 | CSTOPB | PARENB | CLOCAL | CREAD;
	term.c_cflag &= ~( PARODD | HUPCL | CRTSCTS );
	term.c_lflag &= ~( ISIG | ICANON | ECHO | ECHONL | NOFLSH | XCASE | TOSTOP | ECHOPRT );
	term.c_lflag |= ( IEXTEN | ECHOE | ECHOK | ECHOCTL | ECHOKE );
	term.c_iflag &= ~( IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP | IGNCR | ICRNL | IXOFF | IUCLC | IXANY | IMAXBEL | IUTF8 );
	term.c_iflag |= ( IXON );
	term.c_oflag &= ~( OPOST | OLCUC | ONLCR | ONOCR | ONLRET | OFILL | OFDEL );
	term.c_oflag |= ( NL0 | CR0 | TAB0 | BS0 | VT0 | FF0 );
	e(tcsetattr(fd, TCSANOW, &term), "TCSETS");
	/* TODO: /dev/ttyUSB0 need a flush buffer after tcsetattr() */ 
	readwait(fd, &term, sizeof(term));
}

mode_baud_t setmodebaud(int fd, mode_baud_t mode_baud) {
	char buffer[1024];

	switch (mode_baud) {
		case p300:
			if (verbose) printf("Command 'programming-mode300':");
			wp(fd, cs_programming_mode300);
			break;
		case p9600:
			if (verbose) printf("Command 'programming-mode9600':");
			wp(fd, cs_programming_mode9600);
			break;
		case m9600:
			if (verbose) printf("Command 'manufacturer-mode':");
			wp(fd, cs_manufacturer_mode);
			break;
		default:
			if (debug) printf("Mode unknown %d\n", mode_baud);
	}
	switch (mode_baud) {
		case p300:
			break;
		case p9600:
		case m9600:
			if (debug) now(1, "Begin set baud 9600");
			usleep(200000);
			setbaud(fd, B9600);
			readwait(fd, &buffer, sizeof(buffer));
			break;
		default:
			if (debug) printf("Baud unknown %d\n", mode_baud);
	}

	return mode_baud;
}

/*
 * Set DTR - Data Terminal Ready - to low
 * This is a really strange thing to do.
 */
void setdtrlow(int fd) {
	int status;

	e(ioctl(fd, TIOCMGET, &status), "TIOCMGET");
	status &= ~( TIOCM_DTR ); /* DTR low is stupid */
	status |= TIOCM_RTS;
	e(ioctl(fd, TIOCMSET, &status), "TIOCMSET");
	if (debug) {
		e(ioctl(fd, TIOCMGET, &status), "TIOCMGET2");
		if ((status & (TIOCM_LE | TIOCM_DTR | TIOCM_CAR | TIOCM_DSR | TIOCM_RTS | TIOCM_CTS)) == TIOCM_RTS)
			printf("Modem line status OK\n");
		else
			printf("Fail 0x%04x %04x\n", status & (TIOCM_LE | TIOCM_DTR | TIOCM_CAR | TIOCM_DSR | TIOCM_RTS | TIOCM_CTS), TIOCM_RTS);
		printf("Ctrl DSR : %d\n", (status & TIOCM_LE)  > 0 );
		printf("Ctrl DTR : %d\n", (status & TIOCM_DTR) > 0 );
		printf("Ctrl DCD : %d\n", (status & TIOCM_CAR) > 0 );
		printf("Ctrl DSR : %d\n", (status & TIOCM_DSR) > 0 );
		printf("Ctrl RTS : %d\n", (status & TIOCM_RTS) > 0 );
		printf("Ctrl CTS : %d\n", (status & TIOCM_CTS) > 0 );
	}
}

/*
 * Loop through chained list of commands and execute them.
 */
int exec_chain_list(int fd, struct command_que_t *head) {
	struct command_serial_t *cmd;
	int count = 0;

	while (head) {
		++count;
		cmd = &command_serial[head->idx];
		if (verbose) printf("Command '%s", cmd->name);
		if (cmd->has_arg != non) {
			if (verbose) printf("(%s)':", *(char **)cmd->command_arg);
			wpa(fd, *cmd->command_data, *(char **)cmd->command_arg);
		} else {
			if (verbose) printf("':");
			wp(fd, *cmd->command_data);
		}
		head = head->next;
	}

	return count;
}

int processdevice(char *device) {
	char buffer[1024];
	struct termios term_orig;
	int fd;
	int num;
	mode_baud_t current_mode = none;
	int loop_count;

	if (verbose) printf("Open device: %s\n", device);
	if ((fd = open(device, O_RDWR | O_NOCTTY )) > 0) {
		if (debug) printf("Device '%s' open with handle '%d'.\n", device, fd);

		e(tcgetattr(fd, &term_orig), "tcgetattr");
		setbaud(fd, B300);
		setdtrlow(fd);

		if (verbose) printf("Command 'get-productid':\n");
		if (writecom(fd, cs_get_productid)) {
			num = readwait(fd, &buffer, sizeof(buffer));
			printbuf(&buffer, num, verbose);
		}

		if (cq_head) {
			/* set baud to the rate first used */
			current_mode = setmodebaud(fd, command_serial[cq_head->idx].mode_baud);
			if ( (loop_count = loop) == 0) {
				loop_count = -1;
			}
			gettimeofday(&delay_start, NULL);
			while (loop_count) {
				if (time_stamp) {
					printcurtime();
				}
				exec_chain_list(fd, cq_head);
				if (time_stamp) {
					putchar('\n');
				}
				if ( (--loop_count) <= -1 ) {
					loop_count = -1;
				}
				sleep_timeval(delay);
			}
		}

		/* Flush buffer until timeout (--get-logdata) */
		num = 1;
		while (num) {
			num = readwait(fd, &buffer, sizeof(buffer));
			if (num) printbuf(&buffer, num, verbose);
		}

		if (current_mode != p300) {
			/* Set baud back to 300 to open faster next time */
			setmodebaud(fd, p300);
			usleep(100000); /* wait for data written */
		}

		ioctl(fd, TCSETS, &term_orig);
		close(fd);
	}

	return fd;
}

/*
 * Split 'source' with '\0' instead of a delimiter
 * and terminate with two '\0'.
 */
char *splitdup0(char *source, char *delim) {
	char *destination = (char *)malloc(strlen(source)+2);
	char *pd = destination;

	memcpy(destination, source, strlen(source)+1);
	while (*pd) {
		char *d = delim;
		while (*d && *d != *pd) {
			d++;
		}
		if (*d == *pd) {
			*pd = '\0';
		}
		pd++;
	}
	pd++;
	*pd = '\0';

	return destination;
}

int main(int argc, char *argv[]) {
	char *device_list;
	char *device;
	int result;

	if (argc < 2) {
		show_help();
		return 1;
	}

	pre_parse_arguments(argv[0]);
	parse_arguments(argc, argv);

	if (help) {
		show_help();
		return 0;
	}

	if (debug) printf("Open device list: '%s'\n", o_device);
	device_list = splitdup0(o_device, " :");
	device = device_list;
	result = 0;
	while (device && result >= 0) {
		result = processdevice(device);
		if (result < 0) {
			fprintf(stderr, "Error on opening '%s': %s\n", device, strerror(errno));
		} else {
			if (debug) printf("End process device: '%s'\n", device);
		}
		device += strlen(device)+1;
		if (!strlen(device)) {
			device = NULL;
		}
		result = 0;
	}
	free(device_list);

	return EXIT_SUCCESS;
}
