/*
 *    Copyright 1995,1996,1997 (C) by Karsten Ball|der
 *                              karsten@pimajestix.physik.uni-karlsruhe.de
 *
 *
 * This software comes with ABSOLUTELY NO WARRANTY; for details see
 * the section COPYRIGHT in the manual. This is free software and you
 * are welcome to redistribute it under certain conditions.
 *
 *
 *      If you are using this program within some other software, you must
 *      state that your software is using MultiBuf, that MultiBuf is part of
 *      the KBackup package and your software must show the above copyright.
 *
 *      multibuf.c :    program for transparently handling multi-volume
 *                      archives with tar or afio
 *
 */
/*      The meaning of the three commands:

 * command:     executed at end of medium (e.g. prompting, umount)
 * command2:    executed at beginning of new medium (e.g. mt rewind, mount)
 * rcommand:    executed after command2 if reading fails or wrong sequence
 *              rcommand is not followed by another command2!
 *              e.g.: umount + prompting to change medium + mount
 *
 *      I am not sure whether this is intelligent or not, maybe I should
 *      execute a command2 after each rcommand? However, like this it works,
 *      so I will keep it like this.
 */


#define	REVISION	"Revision: 1.23 "
#define	ID		"$Id$"
#define	DATE		"$Date$"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<getopt.h>

#include	<sys/types.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>

#include	<assert.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/shm.h>


#define	DO(cmd)	{ fprintf(stderr,"PID %d, doing: \""#cmd"\"...", getpid()); cmd; fprintf(stderr,"done\n");}

#ifndef NDEBUG
#define DBGint(v)	fprintf(stderr,"PID %d, "#v"=%d\n", getpid(), v)
#define DBGmsg(m)	fprintf(sterr,"PID %d, "msg"\n",getpid());
#else
#define	DBGint(x)
#define DBGmsg(m)
#endif

#define	DATA_FORMAT_VERSION	1


#define	DEFAULT_BLOCKSIZE	1024
#define	MINIMUM_BLOCKSIZE	1024
#define	DEFAULT_BLOCKS		100
#define	MINIMUM_BLOCKS		5

#define DEFAULT_COMMAND "dialog --title \"KBackup's MultiBuf\" --yesno \"Continue with next volume?\" 5 40 </dev/tty >/dev/tty"
#define	DEFAULT_COMMAND2 "mt -f %s rewind </dev/tty >/dev/tty"
#define	DEFAULT_RCOMMAND "dialog --title \"KBackup's MultiBuf\" --yesno \"Detected wrong sequence number.\nRetry?\" 6 50 </dev/tty >/dev/tty"
#define MULTIBUF_HEADER		"MultiBuf by Karsten Ballüder"
#define MULTIBUF_HEADER_LEN	28
#define ENDMARKER		"ENDMARKER"
#define ENDMARKER_LEN		10

#define	EXIT_WRONG_ARGS		1
#define	EXIT_CANNOT_READ	2
#define	EXIT_CANNOT_WRITE	3
#define	EXIT_WRONG_SEQUENCE	4
#define	EXIT_NO_MEM		5
#define	EXIT_FATAL_ERROR	255

size_t bufsize = DEFAULT_BLOCKS * DEFAULT_BLOCKSIZE;
size_t blocks = DEFAULT_BLOCKS;
size_t blocksize = DEFAULT_BLOCKSIZE;
size_t bytes_written = 0;
char *command = DEFAULT_COMMAND;
char *command2 = DEFAULT_COMMAND2;
char *rcommand = DEFAULT_RCOMMAND;
char *device = "<NULL>";

volatile char ** _buffer  = NULL;
volatile size_t * _buflen = NULL;
int write_mode = 1;
int verbose = 0;
int command2enable = 1;
int use_header = 0;
int sequence_number = 1;
int use_endmarker = 1;
int file_endmarker = 0;
int disable_fmark_fix = 0;
int padding_disabled = 0;

int	multibuf = 0;
key_t	multisem = -1;
key_t	multishm = -1;
int	multipid1 = -1;
int	multipid2 = -1;

char *progname = "multibuf";

extern char *optarg;
extern int optind, opterr, optopt;

static struct option long_opts[] =
{
    {"help", 0, 0, '?'},
    {"read", 0, 0, 'r'},
    {"write", 0, 0, 'w'},
    {"verbose", 0, 0, 'v'},
    {"command", 1, 0, 'c'},
    {"command2", 1, 0, 'C'},
    {"retry-command", 1, 0, 'R'},
    {"begin-and-end", 1, 0, '2'},
    {"end-only", 1, 0, '1'},
    {"settings", 0, 0, 's'},
    {"nblocks", 1, 0, 'n'},
    {"blocksize", 1, 0, 'b'},
    {"multibuf",1,0,'m'},
    {"exit", 0, 0, 'x'},
    {"header", 0, 0, 'h'},
    {"version", 0, 0, 'V'},
    {"no-endmarker", 0, 0, 'E'},
    {"file-endmarker", 0, 0, 'F'},
    {"disable-filemark-fix", 0, 0, 'D'},
    {"disable-padding",0,0,'p'},
    {0, 0, 0, 0}
};

static char short_opts[] = "?rwvc:sn:b:C:12xhVR:EFDpm:";

static void
print_longhelp (void)
{
    fprintf (stderr,
	     "MultiBuf " REVISION "\t(C) 1995,1996 by Karsten Ball|der\n"
	     "                           karsten@pimajestix.physik.uni-karlsruhe.de\n"
	     "\n"
	     "USAGE: %s [options] filename\n"
	     "valid options:\n"
	     "   -r, --read		: set read mode\n"
	     "   -w, --write		: set write mode (default)\n"
	     "   -v, --verbose	: verbose progress reporting\n"
	     "   -b, --blocksize	: set size of each data block in bytes\n"
	     "   -n, --nblocks	: set number of blocks in each superblock\n"
	     "   -m, --multibuf : enable multibuffering with this many buffers\n"
	     "   -c, --command \"shell-command\"  : set command to execute at end of medium\n"
	     "   -C, --command2 \"shell-command\" : set command to execute at begin of medium\n"
	     "   -R, --retry-command \"shell-command\" : set command to execute when finding\n"
	     "   					   wrong sequence number\n"
	     "   -1, --end-only	: only execute command at end of media\n"
	     "   -2, --begin-and-end	: exeute commands at end and begin of media\n"
	     "   -s, --settings	: display settings\n"
	     "   -x, --exit		: exit after processing options (for testing)\n"
	     "   -h, --header		: add header/sequence information to data\n"
	     "   -E, --no-endmarker   : do not use an endmarker\n"
	     "   -F, --file-endmarker : write endmarker as a file on a mounted medium\n"
	     "   -p, --disable-padding : disable padding of incomplete input blocks\n"
	     "   -D, --disable-filemark-fix : for debugging purpose only\n"
	     "   -V, --version	: print version information\n"
	     "\n"
	     "   return codes: 0=success, 1=read error, 2=write error, 3=wrong sequence\n"
	     ,progname);
    exit (EXIT_SUCCESS);
}

void
display_settings (void)
{
    if (write_mode)
	fprintf (stderr, "operating in write-mode\n");
    else
	fprintf (stderr, "operating in read-mode\n");

    if (use_header)
    {
	fprintf (stderr, "use sequence header\n");
	if (use_endmarker)
	{
	    fprintf (stderr, "\tuse endmarker");
	    if (file_endmarker)
		fprintf (stderr, ", written as separate file\n");
	    else
		fprintf (stderr, ", written as tape block\n");
	}
    }
    if (command2enable)
	fprintf (stderr, "call commands 1 and 2\n");
    else
	fprintf (stderr, "call one command only\n");

    fprintf (stderr,
	     "size of one data block: %ld, number of blocks per superblock: %ld\n"
	     "\t==> total buffer size: %ld\n"
	     "first command=[%s]\n",
	     (long) blocksize, (long) blocks,
	     (long) bufsize, command);
    if (command2enable)
	fprintf (stderr, "second command=[%s]\n", command2);
    fprintf (stderr, "retry command=[%s]\n", rcommand);

}

void
print_help (void)
{
    fprintf (stderr, "\aUSAGE: %s [options] filename\n	type %s -? for help\n", progname, progname);
}

void
process_options (int argn, char **argv)
{
    int opt_index, c;

    for (;;)
    {
	c = getopt_long (argn, argv, short_opts, long_opts, &opt_index);
	if (c == -1)
	    break;
	switch (c)
	{
	case '?':
	    print_longhelp ();
	    break;
	case 'r':
	    write_mode = 0;
	    break;
	case 'w':
	    write_mode = 1;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'c':
	    command = optarg;
	    break;
	case 'C':
	    command2 = optarg;
	    break;
	case 'R':
	    rcommand = optarg;
	    break;
	case 'h':
	    use_header = 1;
	    break;
	case '1':
	    command2enable = 0;
	    break;
	case '2':
	    command2enable = 1;
	    break;
	case 'E':
	    use_endmarker = 0;
	    break;
	case 'F':
	    file_endmarker = 1;
	    break;
	case 's':
	    display_settings ();
	    break;
	case 'b':
	    blocksize = (size_t) atol (optarg);
	    if (blocksize < MINIMUM_BLOCKSIZE)
		blocksize = MINIMUM_BLOCKSIZE;
	    bufsize = blocks * blocksize;
	    break;
	case 'n':
	    blocks = (size_t) atol (optarg);
	    if (blocks < MINIMUM_BLOCKS)
		blocks = MINIMUM_BLOCKS;
	    bufsize = blocks * blocksize;
	    break;
	case 'm':
	    multibuf = atol(optarg);
	    break;
	case 'V':
	    fprintf (stderr, "MultiBuf Revision " REVISION "\n\t"
		     ID "\n\t" DATE "\n");
	    break;
	case 'x':
	    exit (EXIT_SUCCESS);
	case 'D':
	    disable_fmark_fix = 1;
	    break;
	default:
	    print_help ();
	    exit (EXIT_WRONG_ARGS);
	    ;
	}
    }
}



void mysemop(int s, int o)
{
	struct sembuf sop = {0};

	sop.sem_op = o;
	sop.sem_num = s;
	while(semop(multisem,&sop,1)<0 && (errno == EINTR || errno == EAGAIN));
}

#define swait(s,n) mysemop(s,-(n))
#define ssignal(s,n) mysemop(s,n)

void remsem(void)
{
	if(multisem != -1) {
		semctl(multisem,0,IPC_RMID,0);
		multisem = -1;
	}
}


void
write_loop (const char *filename, int job)
{
    int output = -1, rc;
    int nread, written, bytesread;
    int result;
    char *buffer2, strbuf[1024];
    int curbuf = -1;
    char *buffer = (char *)*_buffer;
    int	trailing = 0;

    if (verbose)
	fprintf (stderr, "write_loop %d:\n",job);

    if (job != 1)
    {
	assert (output == -1);
	output = open (filename, O_WRONLY | O_CREAT, 0666);
	if (output < 0)
	{
	    fprintf (stderr, "\nmultibuf: Cannot open output file %s!\a\n", filename);
	    exit (EXIT_CANNOT_WRITE);
	}

	if (use_header)
	{
	    strcpy (buffer, MULTIBUF_HEADER);
	    sprintf (buffer + strlen (MULTIBUF_HEADER) + 1, "%05d %03d %010u %010u", sequence_number++,
		     DATA_FORMAT_VERSION, (unsigned) blocksize, (unsigned) blocks);
	    result = write (output, buffer, blocksize);
	    if (result < blocksize)
	    {
		fprintf (stderr, "\nmultibuf: Cannot write header to output!\a\n");
		exit (EXIT_CANNOT_WRITE);
	    }
	}
	if (job)
	{
	    ssignal(1,1); /* writer doesn't need buffer anymore */
	    DO(swait(1,0)); /* make sure we don't cross wires */
	}
    } else
	  DO(swait(1,1)); /* let writer use 1st buffer for header */

    for (nread=bufsize;nread==bufsize;)
    {
	if (job < 2) {
	    if (job)
	    {
		DO(swait(0,1));
		curbuf = (curbuf+1)%multibuf;
		buffer = (char *)_buffer[curbuf];
	    }
	    bytesread = fread (buffer, 1, blocksize*blocks, stdin);
	    nread = bytesread / blocksize;
	    trailing = bytesread % blocksize;
	    if(trailing && ! padding_disabled)
	    {
		memset(buffer+bytesread,0,blocksize-trailing);
		if(verbose)
		    fprintf(stderr,"\nPadding incomplete input block with zeroes.\n");
		nread ++;
	    }
	    if (job)
	    {
		_buflen[curbuf] = nread<0?0:nread*blocksize;
		DO(ssignal(1,1));
	    }
	    if (nread < blocks)
	    {
		if (nread == 0 && trailing == 0)	/* end of input data */
		  break;
		if (verbose)
		    fprintf(stderr,
			    "\nOnly read %d blocks + %d trailing bytes.\n",
			    trailing?nread-1:nread, trailing);
	    	    }
	    nread *= blocksize;
	    nread += trailing;
	}
	if (job != 1)
	{
	    do
	    {
		if (job)
		{
		    DO(swait(1,1));
		    curbuf = (curbuf+1)%multibuf;
		    buffer = (char *)_buffer[curbuf];
		    nread = _buflen[curbuf];
		}
		written = write (output, buffer, nread);
		if (written < nread)
		{
		    DO(close (output))
		    output = -1;
		    sprintf (strbuf, command, device);
		    rc = system (strbuf);
		    if ((rc == 127 && errno != 0) || rc != 0)
		      break;
		    if (command2enable)
		    {
			sprintf (strbuf, command2, device);
			rc = system (strbuf);
			if ((rc == 127 && errno != 0) || rc != 0)
			{
			    fprintf (stderr, "\nmultibuf: Cannot execute \"%s\"!\a\n", strbuf);
			    break;
			}
		    }
		    assert (output == -1);
		    output = open (filename, O_CREAT | O_WRONLY, 0666);
		    if (output < 0)
		    {
			fprintf (stderr, "\nmultibuf: Cannot open output file %s!\a\n", filename);
			exit (EXIT_CANNOT_WRITE);
		    }
		    if (use_header)
		    {
			buffer2 = malloc (blocksize);
			sprintf (buffer2, "%05d", sequence_number++);
			result = write (output, buffer2, blocksize);
			if (result < blocksize)
			{
			    fprintf (stderr, "\nmultibuf: Cannot write sequence number!\a\n");
			    exit (EXIT_CANNOT_WRITE);
			}
			free (buffer2);
		    }
		}
	    }
	    while (written < nread);
	    if (job)
	      ssignal(0,1);
	    bytes_written += written;
	    if (verbose)
	      fprintf (stderr, "\r wrote %ld bytes", (long int) bytes_written);
	}
    }				/* for */
    if (job == 1)
      return;
    if (verbose)
	fprintf (stderr, "\nTotal bytes written: %ld\n", (long int) bytes_written);
    DO(close (output));
    output = -1;

    /*  write an END marker after last block
	just a short file, so after EOF another read returns
	*/
    if (use_header && use_endmarker)	/* write END marker */
    {
	if (file_endmarker)
	{
	    char *endfile = malloc (strlen (filename) + 5);
	    if (!endfile)
		exit (EXIT_NO_MEM);
	    strcpy (endfile, filename);
	    strcat (endfile, ".END");
	    assert (output == -1);
	    output = open (endfile, O_CREAT | O_WRONLY, 0666);
	    free (endfile);
	}
	else
	{
	    assert (output == -1);
	    output = open (filename, O_CREAT | O_WRONLY, 0666);
	}
	if (output < 0)
	{
	    fprintf (stderr, "\nmultibuf: Cannot write end marker (%s)!\a\n",
		     strerror (errno));
	    exit (EXIT_CANNOT_WRITE);
	}
	strcpy (buffer, ENDMARKER);
	write (output, buffer, blocksize);
	if (verbose)
	    fprintf (stderr, "\nwrote end marker\n");
	DO(close (output));
	output = -1;
    }
}


void
read_loop (const char *filename, int job)
{
    int input = -1, nread, written, result, rc, seq_no = -1,	/* seq_no: from file */
	retry;
    char *buffer2, strbuf[1024];
    int endmarker_detected = 0;
    int curbuf = -1;
    char *buffer;

    int data_format_version = -1;
    unsigned ublocks = (unsigned) blocks, ublocksize = (unsigned) blocksize;

    if (verbose)
	fprintf (stderr, "read_loop %d:\n", job);

    buffer = (char *)*_buffer;
    retry = !disable_fmark_fix;
restart_read:
    if (job < 2)
    {
	assert (input == -1);
	input = open (filename, O_RDONLY);
	if (input < 0)
	{
	    fprintf (stderr, "multibuf: Cannot open input file %s!\a\n", filename);
	    exit (EXIT_CANNOT_READ);
	}
	if (use_header)
	{
	    result = read (input, buffer, blocksize);
	    if (result < blocksize ||
		strcmp (buffer, MULTIBUF_HEADER) != 0)
	    {
		DO(close (input))
		input = -1;
		if (result == 0 && retry)
		{
		    retry = 0;
		    goto restart_read;
		}
		fprintf (stderr, "multibuf: Cannot read header!\a\n");
		rc = system (rcommand);	/* retry */
		if ((rc == 127 && errno == 0) || rc == 0)
		  goto restart_read;
		exit (EXIT_CANNOT_READ);
	    }
	    sscanf (buffer + strlen (MULTIBUF_HEADER) + 1,
		    "%05d %03d %010u %010u", &seq_no,
		    &data_format_version, &ublocksize, &ublocks);
	    if (sequence_number != seq_no)
	    {
		DO(close (input))
		input = -1;
		rc = system (rcommand);	/* retry */
		if ((rc == 127 && errno == 0) || rc == 0)
		  goto restart_read;
		fprintf (stderr, "\nmultibuf: Wrong sequence number!\a\n"
			 "\tExpected %d, read %d\n", sequence_number, seq_no);
		exit (EXIT_WRONG_SEQUENCE);
	    }
	    if (data_format_version >= DATA_FORMAT_VERSION)
	    {
		blocks = (size_t) ublocks;
		blocksize = (size_t) ublocksize;
		if (verbose)
		  fprintf (stderr, "\nset blocksize=%u, nblocks=%u\n",
			   ublocksize, ublocks);
	    }
	    sequence_number++;
	}
    }

    for (;;)
    {
	if (job < 2)
	{
	    if (job)
	    {
		swait(0,1);
		curbuf = (curbuf+1)%multibuf;
		buffer = (char *)_buffer[curbuf];
	    }
	    do
	    {
		nread = read (input, buffer, bufsize);

		if (nread < bufsize)
		{
		    DO(close (input))
		    input = -1;
		    if (use_header && use_endmarker)	/* look for end marker */
		    {
			endmarker_detected = 0;
			if (file_endmarker)
			{
			    char *endfile = malloc (strlen (filename) + 5);
			    if (!endfile)
			      exit (EXIT_NO_MEM);
			    strcpy (endfile, filename);
			    strcat (endfile, ".END");
			    assert (input == -1);
			    input = open (endfile, O_RDONLY);
			    if (input >= 0)
			    {
				endmarker_detected = 1;
				DO(close (input))
				input = -1;
			    }
			    free (endfile);
			}
			else
			  /* look for normal endmarker: */
			{
			    retry = !disable_fmark_fix;
			    buffer2 = malloc (blocksize);
			retry_endmarker:
			    assert (input == -1);
			    input = open (filename, O_RDONLY);
			    if (input < 0)
			    {
				fprintf (stderr, "\nmultibuf: Cannot open input file %s!\a\n", filename);
				exit (EXIT_CANNOT_READ);
			    }
			    endmarker_detected = 0;
			    if (buffer2 && read (input, buffer2, blocksize))
			    {
				if (strncmp (buffer2, ENDMARKER, ENDMARKER_LEN) == 0)
				{
				    if (verbose)
				      fprintf (stderr, "\nEND marker detected.\n");
				    endmarker_detected = 1;
				}
			    }
			    else if (buffer2 && retry)	/* retry */
			    {
				DO(close (input))
				input = -1;
				retry = 0;
				if (verbose)
				  fprintf (stderr, "retrying read_endmarker\n");
				goto retry_endmarker;
			    }
			    if (buffer2)
			      free (buffer2);
			    DO(close (input))
			    input = -1;
			}
		    }
		    if (endmarker_detected ||
			(((rc = system (command)) == 127 &&
			  errno != 0) || rc != 0))
		    {
			if (job)
			{
			    _buflen[curbuf] = nread;
			    ssignal(1,1);
			} else {
			    written = fwrite (buffer, nread, 1, stdout);
			    bytes_written += written;
			    if(written < 1)
			    {	fprintf(stderr,"\nmultibuf: Only wrote %d blocks!\a\n", nread);
				exit(EXIT_CANNOT_WRITE);
			    }
			}
			if (verbose)
			  fprintf (stderr, "\r read %ld bytes", (long int) bytes_written);
			exit (EXIT_SUCCESS);
		    }
		    /* continue reading */
		    if (command2enable)
		    {
			sprintf (strbuf, command2, device);
			rc = system (strbuf);
			if ((rc == 127 && errno != 0) || rc != 0)
			{
			    fprintf (stderr, "\nmultibuf: Cannot execute \"%s\"!\a\n", strbuf);
			    exit (EXIT_CANNOT_READ);
			}
		    }
		restart_read2:
		    retry = !disable_fmark_fix;
		retry_read2:
		    assert (input == -1);
		    input = open (filename, O_RDONLY);
		    if (input < 0)
		    {
			fprintf (stderr, "\nmultibuf: Cannot open input file %s!\a\n", filename);
			exit (1);
		    }
		    if (use_header)
		    {
			buffer2 = malloc (blocksize);
			result = read (input, buffer2, blocksize);
			if (retry && result == 0)
			{
			    DO(close (input))
			    input = -1;
			    free(buffer2);
			    if (verbose)
			      fprintf (stderr, "retrying read2\n");
			    goto retry_read2;
			}
			if (result < blocksize)
			{
			    fprintf (stderr, "\nmultibuf: Cannot read sequence number!\a\n");
			    exit (EXIT_CANNOT_READ);
			}
			sscanf (buffer2, "%05d", &seq_no);
			free (buffer2);
			if (sequence_number != seq_no)
			{
			    DO(close (input))
			    input = -1;
			    if (verbose)
			      fprintf (stderr, "seq no mismatch, expected: %d, got: %d\n", sequence_number, seq_no);
			    rc = system (rcommand);	/* retry */
			    if ((rc == 127 && errno != 0) || rc != 0)
			      goto restart_read2;
			    exit (EXIT_CANNOT_READ);
			}
			sequence_number++;
		    }
		}
	    }
	    while (nread < bufsize);
	    bytes_written += nread;
	    if (verbose)
	      fprintf (stderr, "\r read %ld bytes", (long int) bytes_written);
	    if (job)
	    {
		_buflen[curbuf] = nread;
		ssignal(1,1);
	    }
	    else
	    {
		written = fwrite (buffer, 1, nread, stdout);
		if (written < nread)
		{
		    fprintf (stderr,
			     "\nmultibuf: Only wrote %ld bytes (%s),"
			     " could not write the last %ld bytes!\a\n",
			     (long int) bytes_written, strerror(errno),
			     (long int) nread-written);
		    exit (EXIT_CANNOT_WRITE);
		}
	    }
	    if (verbose)
	      fprintf (stderr, "\r read %ld bytes", (long int) bytes_written);
	}
	else if (job)
	{
	    swait(1,1);
	    curbuf = (curbuf+1)%multibuf;
	    buffer = (char *)_buffer[curbuf];
	    nread = _buflen[curbuf];
	    written = fwrite(buffer, nread,1, stdout);
	    bytes_written += written;
	    if(nread && written < 1)
	    {	fprintf(stderr,"\nmultibuf: Only wrote %d blocks!\a\n", nread);
		exit(EXIT_CANNOT_WRITE);
	    }
	    if(nread < bufsize)
	      exit(EXIT_SUCCESS);
	    ssignal(0,1);
	}
    }
    DO(close (input))
    input = -1;
}


int
alloc_shm (int multibuf)
{
    int i;
    char *buffer;

    if (multibuf < 2)
      return 0;
    /* try to alloc as much multibuffering as possible */
    _buffer = malloc(multibuf*sizeof(*_buffer));
    if(!_buffer)
      return 0;
    else
    {
	_buflen = (size_t *)&_buffer[multibuf];
	for(i=1;i<multibuf;i++)
	  if(!(_buffer[i] = malloc(bufsize)))
	    break;
	if((multisem = semget(IPC_PRIVATE, 2, IPC_CREAT)) < 0)
	  return 0;
	else
	{
	    atexit(remsem);
	    for(;multibuf>=2;multibuf--)
	    {
		if((multishm = shmget(IPC_PRIVATE, multibuf*(bufsize+sizeof(size_t)),IPC_CREAT)) >= 0)
		  break;
	    }
	    if(multishm>=0)
	    {
		buffer = shmat(multishm,0,0);
		shmctl(multishm,IPC_RMID,NULL); /* deferred until detach; automatic at exit */
		if(!buffer)
		  return 0;
		for(i=0;i<multibuf;i++,buffer+=bufsize)
		  _buffer[i] = buffer;
		_buflen = (size_t *)buffer;
	    }
	}
    }
    return multibuf;
}


int
handle_processes (void)
{
    int i,rpid,err = 0;

    if(verbose) fprintf(stderr,"Multibuffer %d\n",multibuf);
    multipid1 = rpid = fork();
    if(rpid < 0)
      multibuf = 0;
    if(rpid) {
	multipid2 = rpid = fork();
	if(rpid < 0) {
	    multibuf = 0;
	    kill(multipid1,15);
	    waitpid(multipid1,NULL,0);
	} else if(rpid) {
	    ssignal(0,1);
	    swait(0,0);
	    ssignal(0,multibuf);
	    while(multipid1 && multipid2) {
		rpid = waitpid(-1,&i,WUNTRACED);
		if(rpid > 0) {
		    if(WIFEXITED(i) || WIFSIGNALED(i) || WIFSTOPPED(i)) {
			if(rpid==multipid1)
			  multipid1 = 0;
			else
			  multipid2 = 0;
			if(!WIFEXITED(i) || WEXITSTATUS(i)) {
			    if(!err) {
				if(WIFEXITED(i))
				  err = WEXITSTATUS(i);
				else
				  err = -1;
			    }
			    if(multipid1 || multipid2)
			      kill(multipid1+multipid2,15);
			}
		    }
		}
	    }
	    exit(err);
	} else
	  i = 2;
    } else {
	swait(0,1);
	i = 1;
    }
    return i;
}

int
main (int argc, char **argv)
{
    char *buffer;
    int i;

    progname = argv[0];


    process_options (argc, argv);

    if (argc < 2)
    {
	print_help ();
	return 1;
    }

    device = argv[argc - 1];

/*   bufsize = blocks*blocksize; */ /* already done in parseargs */
    if (!(multibuf = alloc_shm(multibuf)))
    {
	buffer = malloc (bufsize = blocks * blocksize);
	_buffer = (volatile char **)&buffer;
	if (!buffer)
	{
	    fprintf (stderr, "multibuf: Cannot allocate memory!\a\n");
	    return 1;
	}
	i = 0;
    }
    else
    {
	i = handle_processes(); /* does fork() and returns multiple times */
    }

    DBGint(i);
    if (write_mode)
	write_loop (device,i);
    else
	read_loop (device,i);

    if (!i)
      free (buffer);
    if (verbose && i != 1)
	fprintf (stderr, "\nTotal bytes written: %ld\n", (long int) bytes_written);
    return 0;
}
