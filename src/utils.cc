#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdexcept>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "utils.hh"

static void* (*mocked_read_callback)(size_t* out_size, const char* path);
static int (*mocked_write_callback)(const void* data, size_t size, const char* path);


unsigned get_number_of_cores()
{
	return 1; // FIXME!
}

bool cpu_is_little_endian()
{
	static uint16_t data = 0x1122;
	uint8_t *p = (uint8_t *)&data;

	return p[0] == 0x22;
}

static void *read_file_int(size_t *out_size, uint64_t timeout, const char *path)
{
	uint8_t *data = NULL;
	int fd;
	size_t pos = 0;
	const size_t chunk = 1024;
	fd_set rfds;
	struct timeval tv;
	int ret;
	int n;

	fd = open(path, O_RDONLY | O_NONBLOCK);
	if (fd < 0 && (errno == ENXIO || errno == EWOULDBLOCK)) {
		msleep(timeout);

		fd = open(path, O_RDONLY | O_NONBLOCK);
	}

	if (fd < 0)
		return NULL;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 10;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	do {
		ret = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (ret == -1) {
			close(fd);
			free(data);

			return NULL;
		} else if (ret == 0) { // Timeout
			close(fd);
			free(data);

			return NULL;
		}
		data = (uint8_t *)xrealloc(data, pos + chunk);

		n = read(fd, data + pos, chunk);
		if (n < 0) {
			close(fd);
			free(data);

			return NULL;
		}

		pos += n;
	} while (n == (int)chunk);

	*out_size = pos;

	close(fd);

	return data;
}

void *read_file(size_t *out_size, const char *fmt, ...)
{
	char path[2048];
	va_list ap;
	int r;

	/* Create the filename */
	va_start(ap, fmt);
	r = vsnprintf(path, 2048, fmt, ap);
	va_end(ap);

	panic_if (r >= 2048,
			"Too long string!");

	if (mocked_read_callback)
		return mocked_read_callback(out_size, path);

	return read_file_int(out_size, 0, path);
}


void *read_file_timeout(size_t *out_size, uint64_t timeout_ms, const char *fmt, ...)
{
	char path[2048];
	va_list ap;
	int r;

	/* Create the filename */
	va_start(ap, fmt);
	r = vsnprintf(path, 2048, fmt, ap);
	va_end(ap);

	panic_if (r >= 2048,
			"Too long string!");

	if (mocked_read_callback)
		return mocked_read_callback(out_size, path);

	return read_file_int(out_size, timeout_ms, path);
}


void msleep(uint64_t ms)
{
	struct timespec ts;
	uint64_t ns = ms * 1000 * 1000;

	ts.tv_sec = ns / (1000 * 1000 * 1000);
	ts.tv_nsec = ns % (1000 * 1000 * 1000);

	nanosleep(&ts, NULL);
}


static int write_file_int(const void *data, size_t len, uint64_t timeout, const char *path)
{
	int fd;
	fd_set wfds;
	struct timeval tv;
	int ret = 0;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 10;

	fd = open(path, O_WRONLY | O_CREAT | O_NONBLOCK, S_IWUSR | S_IRUSR);
	if (fd < 0 && (errno == ENXIO || errno == EWOULDBLOCK)) {
		msleep(timeout);

		fd = open(path, O_WRONLY | O_CREAT | O_NONBLOCK, S_IWUSR | S_IRUSR);
		if (fd < 0 && (errno == ENXIO || errno == EWOULDBLOCK))
			return -2;
	}
	if (fd < 0)
		return fd;

	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	ret = select(fd + 1, NULL, &wfds, NULL, &tv);
	if (ret == -1) {
		close(fd);

		return ret;
	} else if (ret == 0) { // Timeout
		close(fd);

		return -2;
	}

	write(fd, data, len);

	close(fd);

	return 0;
}

int write_file(const void *data, size_t len, const char *fmt, ...)
{
	char path[2048];
	va_list ap;

	/* Create the filename */
	va_start(ap, fmt);
	vsnprintf(path, 2048, fmt, ap);
	va_end(ap);

	if (mocked_write_callback)
		return mocked_write_callback(data, len, path);

	return write_file_int(data, len, 0, path);
}

int write_file_timeout(const void *data, size_t len, uint64_t timeout, const char *fmt, ...)
{
	char path[2048];
	va_list ap;

	/* Create the filename */
	va_start(ap, fmt);
	vsnprintf(path, 2048, fmt, ap);
	va_end(ap);

	if (mocked_write_callback)
		return mocked_write_callback(data, len, path);

	return write_file_int(data, len, timeout, path);
}

std::string fmt(const char *fmt, ...)
{
	char buf[4096];
	va_list ap;
	int res;

	va_start(ap, fmt);
	res = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	panic_if(res >= (int)sizeof(buf),
			"Buffer overflow");

	return std::string(buf);
}

char *escapeHelper(char *dst, const char *what)
{
	int len = strlen(what);

	strcpy(dst, what);

	return dst + len;
}

std::string escapeHtml(std::string &str)
{
	const char *s = str.c_str();
	char buf[4096];
	char *dst = buf;
	size_t len = strlen(s);
	size_t i;

	memset(buf, 0, sizeof(buf));
	for (i = 0; i < len; i++) {
		char c = s[i];

		switch (c) {
		case '<':
			dst = escapeHelper(dst, "&lt;");
			break;
		case '>':
			dst = escapeHelper(dst, "&gt;");
			break;
		case '&':
			dst = escapeHelper(dst, "&amp;");
			break;
		case '\"':
			dst = escapeHelper(dst, "&quot;");
			break;
		case '\'':
			dst = escapeHelper(dst, "&#039;");
			break;
		case '/':
			dst = escapeHelper(dst, "&#047;");
			break;
		case '\\':
			dst = escapeHelper(dst, "&#092;");
			break;
		case '\n': case '\r':
			dst = escapeHelper(dst, " ");
			break;
		default:
			*dst = c;
			dst++;
			break;
		}
	}

	return std::string(buf);
}

std::string escapeHtml(const char *str)
{
	std::string s = str;

	return escapeHtml(s);
}

std::string trimString(std::string &strIn)
{
	std::string str = strIn;
	size_t endpos = str.find_last_not_of(" \t");

	if( std::string::npos != endpos )
	{
		str = str.substr( 0, endpos+1 );
	}

	// trim leading spaces
	size_t startpos = str.find_first_not_of(" \t");
	if( std::string::npos != startpos )
	{
		str = str.substr( startpos );
	}

	return str;
}

std::string get_home_directory()
{
	// FIXME! This will not work in Windows, if someone would like to use that
	std::string home = getenv("HOME");

	return home;
}

bool string_is_integer(std::string str)
{
	size_t pos;

	try
	{
		stoll(str, &pos, 0);
	}
	catch(std::invalid_argument &e)
	{
		return false;
	}

	return pos == str.size();
}

int64_t string_to_integer(std::string str)
{
	size_t pos;

	return (int64_t)stoll(str, &pos, 0);
}

std::string escape_string_for_c(std::string &str)
{
	std::string out;

	for (unsigned i = 0; i < str.size(); i++) {
		char c = str[i];

		if (c == '"')
			out += '\\';
		if (c == '\n')
			out += "\\n\"\n\"";
		else
			out += c;
	}

	return out;
}

std::string escape_string_for_xml(std::string &str)
{
	std::string out;

	for (unsigned i = 0; i < str.size(); i++) {
		char c = str[i];

		if (c == '<')
			out += "\\<";
		if (c == '>')
			out += "\\>";
		else
			out += c;
	}

	return out;
}

static const uint64_t TIMESTAMP_MOCKED = 0xffffffffffffffffULL;

static int64_t server_timestamp_diff;
static uint64_t mocked_timestamp = TIMESTAMP_MOCKED;

uint64_t get_utc_timestamp()
{
	if (mocked_timestamp != TIMESTAMP_MOCKED)
		return mocked_timestamp;

	time_t raw;
	struct tm *ptm;
	struct tm tmp;

	time(&raw);
	ptm = gmtime_r(&raw, &tmp);

	if (ptm == NULL)
		return 0;

	return (uint64_t)timegm(ptm) + server_timestamp_diff;
}

void adjust_utc_timestamp(int64_t diff)
{
	server_timestamp_diff = diff;
}

void mock_utc_timestamp(uint64_t ts)
{
	mocked_timestamp = ts;
}

void mock_read_file(void* (*callback)(size_t* out_size, const char* path))
{
	mocked_read_callback = callback;
}

void mock_write_file(int (*callback)(const void* data, size_t size, const char* path))
{
	mocked_write_callback = callback;
}
