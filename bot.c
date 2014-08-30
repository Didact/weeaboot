#undef _GNU_SOURCE
#define _GNU_SOURCE

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <unistd.h>
#include <tgmath.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <itoa.h>
#include <search.h>

#define MAX_BUF_SIZE 512

void writetells(const char*);
int prcsmsg(char*, char*, char*, int);
void clnstr(char*);

//IMPLEMENT WIKI PREVIEWS
//IMPLEMENT LOGGING
//FIX SUBSTRING TO [X, Y) OR [X, Y]
//ABSTRACT LINKED LISTS

struct tellmsg
{
	char* src;
	char* usr;
	char* dst;
	char* msg;
	struct tellmsg* next;
	struct tellmsg* prev;
	struct tm* dte;
};

struct string {
	char* ptr;
	size_t len;
};

char* nickname;
bool _mute = false;
bool _vntls = false;
bool _url = true;
bool _image = false;
bool _quit = false;

struct tellmsg* tfirst;
struct tellmsg* tlast;

struct tm epoch = {.tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 1, .tm_mon = 0, .tm_year = 70};

struct hsearch_data* bms;

void init_string(struct string* s)
{
	s->len = 0;
	s->ptr = malloc(1);
	if(s->ptr == NULL)
	{
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

struct tm* currenttime()
{
	time_t t;
	time(&t);
	struct tm* current = malloc(sizeof(struct tm));
	gmtime_r(&t, current);
	return current;
}

char* timestamp()
{
	struct tm* current = currenttime();
	char* strtime = malloc(26);
	asctime_r(current, strtime);
	strtime[strlen(strtime) - 1] = '\0';
	free(current);
	return strtime;
}

void freearr(char** arr, int n)
{
	if(arr == NULL) return;
	size_t i;
	for(i = 0; i < n; i++)
		free(arr[i]);
	free(arr);
}

void strtrm(int n, char* str)
{
	size_t i;
	if(n == 0) return;
	if(str == NULL) return;
	for(i = n; i < strlen(str) + 1; i++)
		str[i-n] = str[i];
}

int strall(char* hay, char* ndl, char*** res)
{
	size_t n = 0, l = 0;
	char* ptr, *end, *tmp;
	ptr = hay;
	while((ptr = strcasestr(ptr, ndl)) != NULL)
	{
		*res = realloc(*res, (n + 1) * sizeof(char*));
		end = (strstr(ptr, " ") != NULL) ? strstr(ptr, " ") : ptr + strlen(ptr);
		l = end - ptr;
		tmp = malloc(l + 1);
		strncpy(tmp, ptr, l);
		tmp[l] = '\0';
		(*res)[n] = tmp;
		n++;
		ptr++;
	}
	return n;
}

bool compare(const char* a, const char* b)
{
	if(a == NULL || b == NULL) return false;
	return (strncasecmp(a, b, strlen(b)) == 0);
}

void strrpl(char* str, const char* ol, const char* ne)  //if strlen(ol) < strlen(ne), strrpl will run past the allocated memory
{
	char* ptr;
	if(strlen(ol) >= strlen(ne))
	{
		while((ptr = strcasestr(str, ol)) != NULL)
		{
			strncpy(ptr, ne, strlen(ne));
			strtrm(strlen(ol) - strlen(ne), ptr + strlen(ne));
		}
	}
	else
	{
		while((ptr = strcasestr(str, ol)) != NULL)
		{
			int n = strlen(ne) - strlen(ol);
			int i;
			for(i = strlen(ptr); i >=strlen(ol); i--)
				ptr[i+n] = ptr[i];
			strncpy(ptr, ne, strlen(ne));
		}
	}
}

char* substr(char* sptr, char* eptr)  //Makes substring of (sptr, eptr)
{
	size_t len;
	char* res;
	len = eptr - sptr - 1;
	res = malloc(len + 1);
	strncpy(res, sptr + 1, len);
	res[len] = '\0';
	return res;
}

int netconn(char* server, char* port)
{
	int status;
	int sock;
	struct addrinfo hints;
	struct addrinfo* res;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(server, port, &hints, &res)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(1);
	}

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(sock == 1)
	{
		fprintf(stderr, "socket failed: %s\n", strerror(errno));
	}
	connect(sock, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	return sock;
}

int sendcommand(char* msg, int sock)
{
	if(msg == NULL) return 0;
	char* m = malloc(strlen(msg) + 2 + 1);
	sprintf(m, "%s\r\n", msg);
	int r = send(sock, m, strlen(m), 0);
	if (r == -1)
	{
		fprintf(stderr, "Send failed: %s", m);
		return 1;
	}
	free(m);
	return 0;
}

int nick(char* n, int sock)
{
	char* buf = malloc(strlen(n) + 5 + 1);
	sprintf(buf, "NICK %s", n);
	int r = sendcommand(buf, sock);
	free(buf);
	return r;
}

int user(char* n, int sock)
{
	char* buf = malloc(2 * strlen(n) + 5 + 6 + 1);
	sprintf(buf, "USER %s 8 * :%s", n, n);
	int r = sendcommand(buf, sock);
	free(buf);
	return r;
}

int join(const char* chan, int sock)
{
	char* buf;
	if(chan[0] == '#')
	{
		buf = malloc(strlen(chan) + 5 + 1);
		strcpy(buf, "JOIN ");
	}
	else
	{
		buf = malloc(strlen(chan) + 6 + 1);
		strcpy(buf, "JOIN #");
	}
	strcat(buf, chan);
	int r = sendcommand(buf, sock);
	free(buf);
	return r;
}

int part(const char* chan, const char* reas, int sock)
{
	char* buf =  malloc(5 + strlen(chan) + 2 + strlen(reas) + 1);
	sprintf(buf, "PART %s :%s", chan, reas);
	int r = sendcommand(buf, sock);
	free(buf);
	return r;
}

int quit(const char* msg, int sock)
{
	char* buf = malloc(6 + strlen(msg) + 1);
	sprintf(buf, "QUIT :%s", msg);
	printf("QUIT: %s\n", msg);
	int r = sendcommand(buf, sock);
	free(buf);
	return r;
}

int pong(char* msg, int sock)
{
	char* buf = malloc( 5 + strlen(msg) + 1);
	sprintf(buf, "PONG %s", msg);
	int r = sendcommand(buf, sock);
	free(buf);
	return r;
}

int sendmessage(const char* msg, char* dst, int sock)
{
	if(_mute) return 3;
	if(msg == NULL) return 0;
	char* str = strdup(msg);
	clnstr(str);
	printf("%s/%s: %s\n", dst, nickname, str);
	char* buf = (char*) malloc(8 + strlen(dst) + 2 + strlen(str) + 1);
	sprintf(buf, "PRIVMSG %s :%s", dst, str);
	int r = sendcommand(buf, sock);
	free(buf);
	free(str);
	return r;
}

void trim(char* str)
{
	if(str == NULL) return;
	size_t i = 0;
	while(str[i] == ' ') i++; strtrm(i, str);
	i = strlen(str) - 1;
	while(str[i] == ' ') i--; str[i+1] = '\0';
}

void clnstr(char* str)  //I swear there's entirely too many of these
{
	if(str == NULL) return;
	strrpl(str, "\x01", "");
	strrpl(str, "\n", "");
	strrpl(str, "\r", "");
	strrpl(str, "\t", "");
	strrpl(str, "&quot;", "\"");
	strrpl(str, "&amp;", "&");
	strrpl(str, "&lt;", "<");
	strrpl(str, "&gt;", ">");
	strrpl(str, "&#38;", "&");
	strrpl(str, "&#39;", "'");
	strrpl(str, "&#039;", "'");
	strrpl(str, "&#40;", "(");
	strrpl(str, "&#41;", ")");
	strrpl(str, "&#43;", "\"");
	strrpl(str, "&#47;", "/");
	strrpl(str, "&#58;", ":");
	strrpl(str, "&#62;", ">");
	strrpl(str, "&#8212;", "—");
	strrpl(str, "&#8211;", "–");
	strrpl(str, "&$8217;", "'");
	strrpl(str, "&#8220;", "\"");
	strrpl(str, "&#8221;", "\"");
	strrpl(str, "&ndash;", "–");
	strrpl(str, "&bull;", "•");
	strrpl(str, "&raquo;", "»");
	strrpl(str, "\\u0026", "&");
	strrpl(str, "\\u003d", "=");
	trim(str);
}

char* parsexml(char* str, const char* tok)
{
	if(str == NULL || tok == NULL) return NULL;
	char* ts = malloc(1 + strlen(tok) + 1);
	strcpy(ts, "<");
	strcat(ts, tok);
	char* ptr, *sptr, *eptr;
	ptr = strcasestr(str, ts);
	free(ts);
	if(ptr == NULL) return NULL;
	sptr = strstr(ptr, ">");
	eptr = strstr(sptr, "<");
	char* res;
	res = substr(sptr, eptr);
	clnstr(res);
	return res;
}

char* parsejson(const char* str, const char* tok)
{
	if(str == NULL) return NULL;
	char* sptr, *eptr, *res;
	sptr = strcasestr(str, tok);
	if(sptr == NULL) return NULL;
	sptr = strcasestr(sptr, "\"") + 1;
	sptr = strcasestr(sptr, "\"");
	eptr = strcasestr(sptr+1, "\"");
	res = substr(sptr, eptr);
	clnstr(res);
	return res;
}

size_t write_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
	size_t s = size * nmemb;
	if(_image) return s;
	char* type = strstr((char*) buffer, "Content-Type:");
	if(type != NULL)
	{
		if(!compare(type + 14, "text")) {_image = true; return s;}
	}
	struct string* up = userp;
	up->ptr = realloc(up->ptr, up->len + s + 1);
	if(up->ptr == NULL)
	{
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(up->ptr + up->len, buffer, s);
	up->ptr[up->len + s] = '\0';
	up->len += s;
	return s;
}

char* curlhttp (const char* myurl)	//Works on Kraut Space Magic
{
	if(myurl == NULL) return NULL;
	char* result;
	char* url = strdup(myurl);
	struct string s;
	init_string(&s);
	CURL* curl;
	curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl/1,0");
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
		curl_easy_perform(curl);
		_image = false;
	}
	result = s.ptr;
	curl_easy_cleanup(curl);
	free(url);
	if(*result == '\0'){free(result); return NULL;}
	return result;
}

void newtell(char* src, char* usr, char* dst, char* msg)
{
	struct tellmsg* newt = calloc(1, sizeof(struct tellmsg));
	newt->src = malloc(strlen(src) + 1);
	newt->usr = malloc(strlen(usr) + 1);
	newt->dst = malloc(strlen(dst) + 1);
	newt->msg = malloc(strlen(dst) + 1);
	strcpy(newt->src, src);
	strcpy(newt->usr, usr);
	strcpy(newt->dst, dst);
	strcpy(newt->msg, msg);
	newt->dte = currenttime();
	struct tellmsg* p = tfirst;
	while(p->next != NULL && !compare(usr, p->usr)) p = p->next;	//Sorts by name
	while(p->next != NULL &&  compare(usr, p->usr)) p = p->next;
	if(p->next != NULL)
	{
		p->next->prev = newt;
	}
	newt->next = p->next;
	p->next = newt;
	newt->prev = p;
	writetells("tells.txt");
}

void deltell(struct tellmsg* tell)
{
	if(tell->prev == NULL)	//FIRST ELEMENT IN LIST
	{
		return;
	}
	if(tell->next == NULL)	//LAST ELEMENT IN LIST
	{
		tlast = tell->prev;
		tlast->next = NULL;
	}
	free(tell->dte);
	if(tell->next != NULL)
	{
		tell->next->prev = tell->prev;
	}
	tell->prev->next = tell->next;
	free(tell->src);
	free(tell->usr);
	free(tell->dst);
	free(tell->msg);
	free(tell);
	tell = NULL;
}


size_t iostruct(const char* type, FILE* file, size_t nmemb, void* thing, ...)
{
	va_list args;
	va_start(args, thing);
	void* toread = thing;
	size_t size = va_arg(args, size_t);
	size_t i, total = 0;
	for(i = 0; i < nmemb; i++)
	{
		total += (strcmp(type, "r") == 0) ? fread(toread, size, 1, file) : fwrite(toread, size, 1, file);
		printf("Total size: %zu\n", total);
		toread = va_arg(args, void*);
		size = va_arg(args, size_t);
	}
	return total;
}

void readtells(const char* filename)
{
	FILE* file = fopen(filename, "r");
	if(file == NULL)
	{
		fprintf(stderr, "readtells: File open error. Perhaps the file doesn't exist?\n");
		return;
	}
	struct tellmsg* prev = tfirst;
	struct tellmsg* current = NULL;

	char fields[5][512];
	size_t i = 0;
	size_t j = 0;
	char c = 'a';

	while((c = fgetc(file)) != EOF)
	{
		fields[i][j] = c;
		j++;

		if(c == '\n')
		{
			fields[i++][j-1] = '\0';
			j = 0;
		}

		if(i == 5)
		{
			i = 0;
			j = 0;
			current = calloc(1, sizeof(struct tellmsg));
			current->dte = calloc(1, sizeof(struct tm));

			current->src = strdup(fields[0]);
			current->usr = strdup(fields[1]);
			current->dst = strdup(fields[2]);
			current->msg = strdup(fields[3]);

			strptime(fields[4], "%D%R", current->dte);

			prev->next = current;
			current->prev = prev;
			prev = current;
			current = NULL;
		}

	}
	fclose(file);
	return;
}

void writetells(const char* filename)
{
	FILE* file = fopen(filename, "w");
	char temp[MAX_BUF_SIZE];
	if(file == NULL)
	{
		fprintf(stderr, "File open error");
		return;
	}

	struct tellmsg* p = NULL;
	p = tfirst->next;
	while(p != NULL)
	{
		memset(temp, 0, MAX_BUF_SIZE);
		strftime(temp, MAX_BUF_SIZE, "%D%R", p->dte);
		fprintf(file, "%s\n%s\n%s\n%s\n%s\n", p->src, p->usr, p->dst, p->msg, temp);
		p = p->next;
	}

	fclose(file);
	return;
}

int disptell(char* usr, char* dst, int sock)
{
	int r = 0;
	struct tellmsg* p = NULL;
	p = tfirst->next;
	if(p == NULL)
	{
		return 0;
	}
	struct tellmsg* t = p;
	char buf[MAX_BUF_SIZE];
	while(p != NULL)
	{
		t = p->next;
		if(compare(usr, p->usr))
		{
			memset(buf, 0, MAX_BUF_SIZE);
			sprintf(buf, "%s, message from %s: %s", p->usr, p->src, p->msg);
			char* ndst = (dst == NULL) ? p->dst : dst;
			sendmessage(buf, ndst, sock);
			struct tm* now = currenttime();
			int diff = ((int) floor(difftime(mktime(now), mktime(p->dte)))) / 60;
			char buf2[25];
			if(diff < 60) sprintf(buf2, "Sent %d minutes ago", diff);
			else sprintf(buf2, "Sent %d hours ago", diff/60);
			sendmessage(buf2, ndst, sock);
			free(now);
			deltell(p);
			r++;
		}
		p = t;
	}
	writetells("tells.txt");
	return r;
}

char* search(const char* srv, char* str)
{
	if(str == NULL) return NULL;
	char query[200] = "http://ajax.googleapis.com/ajax/services/search/web?v=1.0&q="; //replace with malloc at some point
	#include "sitelist.txt"
	strrpl(str, " ", "%20");
	strcat(query, str);

	char* http, *url, *res;
	http = curlhttp(query);
	url = parsejson(http, "unescapedUrl");
	if(url == NULL)
	{
		free(http);
		return "No result found";
	}
	if(compare(srv, "VNTLS"))
	{
		char* vhttp;
		vhttp = curlhttp(url);
		res = parsejson(vhttp, "description"); //Works because meta tags function the same as json
		free(vhttp);
	}
	else
	{
		res = parsejson(http, "titleNoFormatting");
	}
	char* result = malloc(strlen(res) + 3 + strlen(url) + 2 + 1);
	if(result != NULL)
	{
		sprintf(result, "%s [ %s ]", res, url);
	}
	free(http);
	free(url);
	free(res);
	return result;
}

void addbm(char* key, char* value)
{
	ENTRY entry = {strdup(key), strdup(value)};
	ENTRY* ret = NULL;
	hsearch_r(entry, ENTER, &ret, bms);
	return;
}

char* searchbm(char* key)
{
	ENTRY entry = {key, NULL};
	ENTRY* res = NULL;
	hsearch_r(entry, FIND, &res, bms);
	if(res == NULL) return NULL;
	return res->data;
}

void readbms()
{
	FILE* file = fopen("bms.txt", "r");
	if(file == NULL)
	{
		fprintf(stderr, "readbms: File open error. Perhaps it doesn't exist?\n");
		return;
	}

	char c = 'a';
	size_t i = 0;
	size_t j = 0;
	char fields[2][512];

	while((c = fgetc(file)) != EOF)
	{
		fields[i][j] = c;
		j++;

		if(c == '\n')
		{
			fields[i][j-1] = '\0';
			j = 0;
			i++;
		}

		if(i == 2)
		{
			printf("bookmark read: %s.\n", fields[0]);
			addbm(fields[0], fields[1]);
			i = 0;
			j = 0;
		}
	}

	fclose(file);
	return;
}

size_t recvs(char* buf, size_t size, int sock)
{
	memset(buf, 0, size);
	size_t r = recv(sock, buf, size, 0);
	if(r < 2) return 0;
	buf[r-2] = '\0';
	return r;
}

void prcsstr(char* str, int sock)
{
	if(str[0] == 'P')
	{
		pong(strstr(str, " ") + 1, sock);
		return;
	}

	char* first, *second, *third, *msg, *nick;

	first = strtok(str, " ");
	second = strtok(NULL, " ");
	third = strtok(NULL, " ");
	msg = strtok(NULL, "\0");

	nick = strtok(first, "!");
	nick++;
	msg++;

	if(second[0] == ':') second++;
	if(third[0] == ':') third++;

	if(compare(second, "PRIV"))
	{
		printf("%s/%s: %s\n", third, nick, msg);
		prcsmsg(nick, third, msg, sock);
	}

	if(compare(second, "PART") || compare(second, "QUIT"))
	{
		printf("PART %s\n", nick);
	}

	if(compare(second, "JOIN"))
	{
		printf("JOIN %s\n", nick);
		disptell(nick, third, sock);
	}

	if(compare(second, "NICK"))
	{
		printf("%s -> %s\n", nick, third);
		disptell(third, NULL, sock);
	}

	return;
}

int prcsmsg(char* usr, char* dst, char* msg, int sock)
{
	if(compare(dst, nickname)) return prcsmsg(dst, usr, msg, sock);
	if(msg[0] != '`')
	{
		char** urls = NULL;
		int n = strall(msg, "http", &urls);
		if(urls != NULL){
		char* html, *title;
		int i;
		for(i = 0; i < n; i++)
		{
			html = curlhttp(urls[i]);
			title = parsexml(html, "title");
			clnstr(title);
			sendmessage(title, dst, sock);
			free(html);
			free(title);
		}
		}
		freearr(urls,  n);
	}
	if(msg[0] == '`')
	{
		strtrm(1, msg);
		if(compare(usr, "Smithy") || compare(dst, "Smithy"))
		{
			if(compare(msg, "QUIT")) {_quit = true; return  quit("Quit", sock);}
			if(compare(msg, "JOIN ")) return join(strstr(msg, " ") + 1, sock);
			if(compare(msg, "PART ")) return part(strstr(msg, " ") + 1, "ttfm", sock);
			if(compare(msg, "MUTE "))
			{
				char* arg;
				arg = strstr(msg, " ");
				if(arg == NULL) return -1;
				if(compare(++arg, "ON")) _mute = true;
				if(compare(arg, "OFF")) _mute = false;
				return 0;
			}
			if(compare(msg, "URL "))
			{
				char* arg;
				arg = strstr(msg, " ");
				if(arg == NULL) return -1;
				if(compare(++arg, "ON")) _url = true;
				if(compare(arg, "OFF")) _url = false;
				return 0;
			}
		}
		if(compare(msg, "ADD"))
		{
			strtok(msg, " ");
			char* key = strtok(NULL, " ");
			char* value = strtok(NULL, "\0");
			if(value == NULL) return sendmessage("Not enough arguments", dst, sock);
			addbm(key, value);
			FILE* file = fopen("bms.txt", "a");
			if(file == NULL)
			{
				fprintf(stderr, "BM fail saved\n");
				return 0;
			}
			fprintf(file, "%s\n%s\n", key, value);
			fclose(file);
			char buf[600];
			sprintf(buf, "Added bookmark [%s] -> [%s]", key, value);
			return sendmessage(buf, dst, sock);
		}	
		if(compare(msg, "TIME"))
		{
			int r;
			char* strtime = timestamp();
			r = sendmessage(strtime, dst, sock);
			free(strtime);
			return r;
		}
		if(compare(msg, "THUMBSUP"))
		{
			return sendmessage("http://i.imgur.com/A9mPftK.jpg", dst, sock);
		}
		if(compare(msg, "ERECT"))
		{
			return sendmessage("http://i.imgur.com/v3Qk93Y.gif", dst, sock);
		}
		if(compare(msg, "BONER"))
		{
			return sendmessage("http://i.imgur.com/X9DLci9.gif", dst, sock);
		}
		if(compare(msg, "RESCUE"))
		{
			return sendmessage("https://www.youtube.com/watch?v=PD-_oLjmCGY", dst, sock);
		}
		if(compare(msg, "CHECKIT"))
		{
			return sendmessage("http://i.imgur.com/3O0w6IU.gif", dst, sock);
		}
		if(compare(msg, "VNTLS "))
		{
			char* res = search("vntls", strstr(msg, " "));
			int r = sendmessage(res, dst, sock);
			free(res);
			return r;
		}
		if(compare(msg, "VNDB "))
		{
			char* res = search("vndb", strstr(msg, " "));
			int r = sendmessage(res, dst, sock);
			free(res);
			return r;
		}
		if(compare(msg, "MAL "))
		{
			char* res = search("mal", strstr(msg, " "));
			int r = sendmessage(res, dst, sock);
			free(res);
			return r;
		}
		if(compare(msg, "BUM "))
		{
			char* res = search("bum", strstr(msg, " "));
			int r = sendmessage(res, dst, sock);
			free(res);
			return r;
		}
		if(compare(msg, "DS "))
		{
			char* res = search("ds", strstr(msg, " "));
			int r = sendmessage(res, dst, sock);
			free(res);
			return r;
		}
		if(compare(msg, "G ") || compare(msg, "GOOGLE "))
		{
			char* res = search("google", strstr(msg, " "));
			int r = sendmessage(res, dst, sock);
			free(res);
			return r;
		}
		if(compare(msg, "WIKI "))
		{
			char* res = search("wiki", strstr(msg, " "));
			int r = sendmessage(res, dst, sock);
			free(res);
			return r;
		}
		if(compare(msg, "TELL "))
		{
			strtok(msg, " ");
			char* s, *u, *d, *m;
			s = usr;
			u = strtok(NULL, " ");
			if(u == NULL) return -1;
			d = dst;
			m = strtok(NULL, "\0");
			if(m == NULL) return -1;
			newtell(s, u, d, m);
			return sendmessage("o7", dst, sock);
		}
		if(compare(msg, "PING"))
		{
			return sendmessage("pong", dst, sock);
		}
		char* res = searchbm(msg);
		if(res != NULL) return sendmessage(res, dst, sock);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	size_t count = 0, r = 1;
	char recvbuf[512];
	
	nickname = argv[1];

	tfirst = calloc(1, sizeof(struct tellmsg));
	tfirst->next = NULL;
	tfirst->prev = NULL;
	tfirst->usr = malloc(2);
	strcpy(tfirst->usr, "0");
	tfirst->usr[1] = '\0';

	readtells("tells.txt");

	bms = calloc(128, sizeof(struct hsearch_data));
	hcreate_r(128, bms);
	readbms();

	while(!_quit)
	{
		int irc = netconn(argv[2], argv[3]);
		while(true)
		{
			if(count == 3)
			{
				nick(nickname, irc);
				user(nickname, irc);
			}
			
			if(count == 6)
			{
				join(argv[4], irc);
			}

			r = recvs(recvbuf, 512, irc);
			if(r == 0) break;

			prcsstr(recvbuf, irc);
			count++;
		}
		puts("Disco'd");
	}

	hdestroy_r(bms);
	free(tfirst->usr);
	struct tellmsg* ptr = tfirst;
	while((ptr = tfirst->next) != NULL) deltell(ptr);

	return 0;
}
