/*
 * uxcons.c: various interactive-prompt routines shared between the
 * Unix console PuTTY tools
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "putty.h"
#include "storage.h"
#include "ssh.h"

int console_batch_mode = FALSE;

static void *console_logctx = NULL;

static struct termios orig_termios_stderr;
static int stderr_is_a_tty;

void stderr_tty_init()
{
    /* Ensure that if stderr is a tty, we can get it back to a sane state. */
    if ((flags & FLAG_STDERR_TTY) && isatty(STDERR_FILENO)) {
	stderr_is_a_tty = TRUE;
	tcgetattr(STDERR_FILENO, &orig_termios_stderr);
    }
}

void premsg(struct termios *cf)
{
    if (stderr_is_a_tty) {
	tcgetattr(STDERR_FILENO, cf);
	tcsetattr(STDERR_FILENO, TCSADRAIN, &orig_termios_stderr);
    }
}
void postmsg(struct termios *cf)
{
    if (stderr_is_a_tty)
	tcsetattr(STDERR_FILENO, TCSADRAIN, cf);
}

/*
 * Clean up and exit.
 */
void cleanup_exit(int code)
{
    /*
     * Clean up.
     */
    sk_cleanup();
    random_save_seed();
    exit(code);
}

void set_busy_status(void *frontend, int status)
{
}

void update_specials_menu(void *frontend)
{
}

void notify_remote_exit(void *frontend)
{
}

void timer_change_notify(unsigned long next)
{
}

int verify_ssh_host_key(void *frontend, char *host, int port, char *keytype,
                        char *keystr, char *fingerprint,
                        void (*callback)(void *ctx, int result), void *ctx)
{
    int ret;

    static const char absentmsg_batch[] =
	"The server's host key is not cached. You have no guarantee\n"
	"that the server is the computer you think it is.\n"
	"The server's %s key fingerprint is:\n"
	"%s\n"
	"Connection abandoned.\n";
/*    static const char absentmsg[] =
	"The server's host key is not cached. You have no guarantee\n"
	"that the server is the computer you think it is.\n"
	"The server's %s key fingerprint is:\n"
	"%s\n"
	"If you trust this host, enter \"y\" to add the key to\n"
	"PuTTY's cache and carry on connecting.\n"
	"If you want to carry on connecting just once, without\n"
	"adding the key to the cache, enter \"n\".\n"
	"If you do not trust this host, press Return to abandon the\n"
	"connection.\n"
	"Store key in cache? (y/n) ";
*/
    static const char wrongmsg_batch[] =
	"WARNING - POTENTIAL SECURITY BREACH!\n"
	"The server's host key does not match the one PuTTY has\n"
	"cached. This means that either the server administrator\n"
	"has changed the host key, or you have actually connected\n"
	"to another computer pretending to be the server.\n"
	"The new %s key fingerprint is:\n"
	"%s\n"
	"Connection abandoned.\n";
/*    static const char wrongmsg[] =
	"WARNING - POTENTIAL SECURITY BREACH!\n"
	"The server's host key does not match the one PuTTY has\n"
	"cached. This means that either the server administrator\n"
	"has changed the host key, or you have actually connected\n"
	"to another computer pretending to be the server.\n"
	"The new %s key fingerprint is:\n"
	"%s\n"
	"If you were expecting this change and trust the new key,\n"
	"enter \"y\" to update PuTTY's cache and continue connecting.\n"
	"If you want to carry on connecting but without updating\n"
	"the cache, enter \"n\".\n"
	"If you want to abandon the connection completely, press\n"
	"Return to cancel. Pressing Return is the ONLY guaranteed\n"
	"safe choice.\n"
	"Update cached key? (y/n, Return cancels connection) ";
*/
    static const char abandoned[] = "Connection abandoned.\n";

    char line[32];
//FZ struct termios cf;

    /*
     * Verify the key.
     */
    ret = verify_host_key(host, port, keytype, keystr);

    if (ret == 0)		       /* success - key matched OK */
	return 1;

//FZ premsg(&cf);
    if (ret == 2) {		       /* key was different */
	if (console_batch_mode) {
	    fprintf(stderr, wrongmsg_batch, keytype, fingerprint);
	    return 0;
	}
	fzprintf_raw(sftpRequest, "%d%s\n%d\n%s\n", (int)sftpReqHostkeyChanged, host, port, fingerprint);
    }
    if (ret == 1) {		       /* key was absent */
	if (console_batch_mode) {
	    fprintf(stderr, absentmsg_batch, keytype, fingerprint);
	    return 0;
	}
	fzprintf_raw(sftpRequest, "%d%s\n%d\n%s\n", (int)sftpReqHostkey, host, port, fingerprint);
    }

    {
	struct termios oldmode, newmode;
	tcgetattr(0, &oldmode);
	newmode = oldmode;
	newmode.c_lflag |= ISIG | ICANON;
	tcsetattr(0, TCSANOW, &newmode);
	line[0] = '\0';
	int ret;
	do
	{
	    ret = read(0, line, sizeof(line) - 1);
	} while (ret == -1 && errno == EINTR);

	tcsetattr(0, TCSANOW, &oldmode);
    }

    if (line[0] != '\0' && line[0] != '\r' && line[0] != '\n') {
	if (line[0] == 'y' || line[0] == 'Y')
	    store_host_key(host, port, keytype, keystr);
//FZ	postmsg(&cf);
        return 1;
    } else {
	fprintf(stderr, abandoned);
//FZ	postmsg(&cf);
        return 0;
    }
}

/*
 * Ask whether the selected algorithm is acceptable (since it was
 * below the configured 'warn' threshold).
 */
int askalg(void *frontend, const char *algtype, const char *algname,
	   void (*callback)(void *ctx, int result), void *ctx)
{
    static const char msg[] =
	"The first %s supported by the server is\n"
	"%s, which is below the configured warning threshold.\n"
	"Continue with connection? (y/n) ";
    static const char msg_batch[] =
	"The first %s supported by the server is\n"
	"%s, which is below the configured warning threshold.\n"
	"Connection abandoned.\n";
    static const char abandoned[] = "Connection abandoned.\n";

    char line[32];
//FZ struct termios cf;

//FZ premsg(&cf);

    if (console_batch_mode) {
	fprintf(stderr, msg_batch, algtype, algname);
	return 0;
    }

    fprintf(stderr, msg, algtype, algname);
    fflush(stderr);

    {
	struct termios oldmode, newmode;
	tcgetattr(0, &oldmode);
	newmode = oldmode;
	newmode.c_lflag |= ECHO | ISIG | ICANON;
	tcsetattr(0, TCSANOW, &newmode);
	line[0] = '\0';
	int ret;
	do
	{
	    ret = read(0, line, sizeof(line) - 1);
	} while (ret == -1 && errno == EINTR);

	tcsetattr(0, TCSANOW, &oldmode);
    }

    if (line[0] == 'y' || line[0] == 'Y') {
//FZ	postmsg(&cf);
	return 1;
    } else {
	fprintf(stderr, abandoned);
//FZ	postmsg(&cf);
	return 0;
    }
}

/*
 * Ask whether to wipe a session log file before writing to it.
 * Returns 2 for wipe, 1 for append, 0 for cancel (don't log).
 */
int askappend(void *frontend, Filename *filename,
	      void (*callback)(void *ctx, int result), void *ctx)
{
    static const char msgtemplate[] =
	"The session log file \"%.*s\" already exists.\n"
	"You can overwrite it with a new session log,\n"
	"append your session log to the end of it,\n"
	"or disable session logging for this session.\n"
	"Enter \"y\" to wipe the file, \"n\" to append to it,\n"
	"or just press Return to disable logging.\n"
	"Wipe the log file? (y/n, Return cancels logging) ";

    static const char msgtemplate_batch[] =
	"The session log file \"%.*s\" already exists.\n"
	"Logging will not be enabled.\n";

    char line[32];
//FZ struct termios cf;

//FZ premsg(&cf);
    if (console_batch_mode) {
	fprintf(stderr, msgtemplate_batch, FILENAME_MAX, filename->path);
	fflush(stderr);
	return 0;
    }
    fprintf(stderr, msgtemplate, FILENAME_MAX, filename->path);
    fflush(stderr);

    {
	struct termios oldmode, newmode;
	tcgetattr(0, &oldmode);
	newmode = oldmode;
	newmode.c_lflag |= ECHO | ISIG | ICANON;
	tcsetattr(0, TCSANOW, &newmode);
	line[0] = '\0';
	int ret;
	do
	{
	    ret = read(0, line, sizeof(line) - 1);
	} while (ret == -1 && errno == EINTR);
	tcsetattr(0, TCSANOW, &oldmode);
    }

//FZ postmsg(&cf);
    if (line[0] == 'y' || line[0] == 'Y')
	return 2;
    else if (line[0] == 'n' || line[0] == 'N')
	return 1;
    else
	return 0;
}

/*
 * Warn about the obsolescent key file format.
 * 
 * Uniquely among these functions, this one does _not_ expect a
 * frontend handle. This means that if PuTTY is ported to a
 * platform which requires frontend handles, this function will be
 * an anomaly. Fortunately, the problem it addresses will not have
 * been present on that platform, so it can plausibly be
 * implemented as an empty function.
 */
void old_keyfile_warning(void)
{
    static const char message[] =
	"You are loading an SSH-2 private key which has an\n"
	"old version of the file format. This means your key\n"
	"file is not fully tamperproof. Future versions of\n"
	"PuTTY may stop supporting this private key format,\n"
	"so we recommend you convert your key to the new\n"
	"format.\n"
	"\n"
	"Once the key is loaded into PuTTYgen, you can perform\n"
	"this conversion simply by saving it again.\n";

//FZ struct termios cf;
//FZ premsg(&cf);
    fzprintf(sftpStatus, message);
//FZ postmsg(&cf);
}

void console_provide_logctx(void *logctx)
{
    console_logctx = logctx;
}

void logevent(void *frontend, const char *string)
{
//FZ struct termios cf;
//FZ premsg(&cf);
    if (console_logctx)
	log_eventlog(console_logctx, string);
//FZ postmsg(&cf);
}

/*
 * Special functions to read and print to the console for password
 * prompts and the like. Uses /dev/tty or stdin/stderr, in that order
 * of preference; also sanitises escape sequences out of the text, on
 * the basis that it might have been sent by a hostile SSH server
 * doing malicious keyboard-interactive.
 */

static void console_open(FILE **outfp, int *infd)
{
    int fd;

    /*if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
        *infd = fd;
        *outfp = fdopen(*infd, "w");
    } else*/ {
        *infd = 0;
        *outfp = stderr;
    }
}
static void console_close(FILE *outfp, int infd)
{
    if (outfp != stderr)
        fclose(outfp);             /* will automatically close infd too */
}
/*
static void console_prompt_text(FILE *outfp, const char *data, int len)
{
    int i;

    for (i = 0; i < len; i++)
	if ((data[i] & 0x60) || (data[i] == '\n'))
	    fputc(data[i], outfp);
    fflush(outfp);
}*/

int console_get_userpass_input(prompts_t *p, unsigned char *in, int inlen)
{
    size_t curr_prompt;
    FILE *outfp = NULL;
    int infd;

    /*
     * Zero all the results, in case we abort half-way through.
     */
    {
	int i;
	for (i = 0; i < p->n_prompts; i++)
            prompt_set_result(p->prompts[i], "");
    }

    if (p->n_prompts && console_batch_mode)
	return 0;

    console_open(&outfp, &infd);

    /*
     * Preamble.
     */
    /* We only print the `name' caption if we have to... */
    if (p->name_reqd && p->name)
	fzprintf_raw_untrusted(sftpRequestPreamble, p->name);
    else
	fzprintf_raw_untrusted(sftpRequestPreamble, "");

    /* ...but we always print any `instruction'. */
    if (p->instruction)
	fzprintf_raw_untrusted(sftpRequestInstruction, p->instruction);
    else
	fzprintf_raw_untrusted(sftpRequestInstruction, "");

    for (curr_prompt = 0; curr_prompt < p->n_prompts; curr_prompt++) {

	struct termios oldmode, newmode;
	int len;
	prompt_t *pr = p->prompts[curr_prompt];

	tcgetattr(infd, &oldmode);
	newmode = oldmode;
	newmode.c_lflag |= ISIG | ICANON;
//	if (!pr->echo)
	    newmode.c_lflag &= ~ECHO;
//	else
//	    newmode.c_lflag |= ECHO;
	tcsetattr(infd, TCSANOW, &newmode);

	fzprintf_raw_untrusted(sftpRequest, "%d%s\n", (int)sftpReqPassword, pr->prompt);

        len = 0;
        while (1) {
            int ret;

            prompt_ensure_result_size(pr, len * 5 / 4 + 512);
            ret = read(infd, pr->result + len, pr->resultsize - len - 1);
            if (ret <= 0) {
                len = -1;
                break;
            }
            len += ret;
            if (pr->result[len - 1] == '\n') {
                len--;
                break;
            }
        }

	tcsetattr(infd, TCSANOW, &oldmode);

	//if (!pr->echo)
	//    console_prompt_text(outfp, "\n", 1);

        if (len < 0) {
            console_close(outfp, infd);
            return 0;                  /* failure due to read error */
        }

	pr->result[len--] = 0;
	while (len >= 0 && (pr->result[len] == '\r' || pr->result[len] == '\n'))
	    pr->result[len--] = '\0';
    }

    console_close(outfp, infd);

    return 1; /* success */
}

void frontend_keypress(void *handle)
{
    /*
     * This is nothing but a stub, in console code.
     */
    return;
}

int is_interactive(void)
{
    return isatty(0);
}

/*
 * X11-forwarding-related things suitable for console.
 */

char *platform_get_x_display(void) {
    return dupstr(getenv("DISPLAY"));
}
