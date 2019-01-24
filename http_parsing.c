#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/acl.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

void print_string(char *source, unsigned long pos_start, unsigned long len) {

    if (256 < len) {
        char *data = (char *)malloc(sizeof(char) * len);
        bzero(data, len);
        strncpy(data, source + pos_start, len);
        printf("[%s]\n", data);
        free(data);
    } else {
        char data[256] = {0};
        strncpy(data, source + pos_start, len);
        printf("[%s]\n", data);
    }
}

/**
 * Parse the Http Request body and return the current request end postion
 * YOU can pass the return value to the next calling to continuous parsing.
 * @param request   The HTTP request body raw string.
 * @param start_pos The start position.
 * @return The next parsing position.
 */
unsigned long parse_http_body(char *request, unsigned long start_pos) {

    unsigned char
        line_no = 0,        /* Line number */
        content_length = 0, /* In length or not */
        line_space = 0;     /* line_space. */

    long body_len = 0;      /* Body length */

    if (start_pos)          /* If set the start_pos, let the request_body to be parsed was the real string */
        request = request + start_pos;

    unsigned long
        line_start_pos = 0,     /* Line start pos. */
        pos = 0,                /* Pos */
        line_pre_pos = 0,       /* line_pre_pos */
        len = strlen(request);  /* The whole length */

    /* Using the for-loop to get the parsing job done. */
    for (; pos < len; pos++ ) {

        /* The current parsing char. */
        char ch = *(request + pos);

        /* If the current pos was \r\n\r\n, which means that we get the body_pos.
         * HTTP_BODY start from the `pos+4` location and cutted by length: `body_len`, if you want to use the
         * string, you should allocate the memory by youself and free it. */
        if ( '\r' == ch && '\n' == *(request + pos + 1)
            && '\r' == *(request + pos + 2) && '\n' == *(request + pos + 3)) {

            /* Get the Content-Length value
             * You must known that there is no way to store all the http header value & field, so re-check it */
            if ( 1 <= line_no ) {

                if (content_length) {
                    /* If content-length set, get the value from it, and turn it into long value for enough
                     * to store the length of the string. */
                    char bnum[256] = {0};
                    strncpy(bnum, request + line_pre_pos, line_pre_pos);
                    body_len = strtol(bnum, 0, 10);
                }
            }

            /* This is the http body, start from the range: [ pos + 4, body_len + pos + 4] */
            print_string(request, pos + 4, (unsigned long) body_len);

            /* Return the body pos. for the next parsing.
             * Current body position was: pos + 4 + body_len */
            if (strlen(request + pos + 4) < body_len) {
                return start_pos;
            }
            return pos + 4 + body_len + start_pos;
        }

        /* If current was in the Http header env.
         * parsing the http header, and get the `Content-Length` from the request_body
         * set content_length to 1, the next parsing will get the `Content-Length` value. */
        if ( 1 <= line_no ) {
            if ( ':' == ch ) {
                /* HTTP header start from: line_start_pos, end with: pos - line_start_pos */
                print_string(request, line_start_pos, pos - line_start_pos);

                /* To get the Content-Length from the http request body. */
                if ( 0 == strncasecmp(request + line_start_pos, "content-length", strlen("content-length"))) {
                    content_length = 1;
                } else {
                    content_length = 0;
                }

                /* Set the current line pos. for the nex HTTP_HEADER value to get the right pos. */
                line_pre_pos = pos + 1;
            }
        }

        /* If the current char. was \n, means that next parsing string was a newly line.
         * depend on the absolute env. to get the meaning. */
        if ( '\n' == ch ) {
            /* Line_space equal to 2 means to get the HTTP_VERSION */
            if ( 2 == line_space && 0 == line_no ) {

                /* HTTP_VERSION start from: line_start_pos + line_pre_pos, end_with: pos - line_pre_pos */
                print_string(request, line_start_pos + line_pre_pos, pos - line_pre_pos);

                /* If you want to reuse the line_space ,set the line_space into 0 for fulture use. */
                line_space = 0;
            }

            if ( 1 <= line_no ) {

                /* HTTP Header value start from `line_pre_pos`, end by `pos - line_pre_pos`, you should clear the
                 * beginning white-space char. */
                print_string(request, line_pre_pos, pos - line_pre_pos);
            }

            /* Increse the line_no when occur the \n char. and set the line_start_postion to the
             * current \n's next pos. line_start_pos was the first pos in each line. */
            line_no++;
            line_start_pos = pos + 1;
        }

        /* line_no equal to 0 & line_start_pos equal to zero plus the current char was white space
         * It means current was in HTTP request line, it was like: GET /index.php HTTP/1.1, but the
         * last value HTTP/1.1 which close to the \n char, so it was parsed above. */
        if ( 0 == line_no && 0 == line_start_pos && ' ' == ch ) {
            if (0 == line_space) {

                /* First line_space to get the HTTP METHOD: position was: [line_start_pos, pos] */
                print_string(request, line_start_pos, pos);
            } else if (1 == line_space) {

                /* The second line_space to get the URI: such as `/index.php`
                 * Position: [ line_start_pos + line_pre_pos, pos - line_pre_pos ]*/
                print_string(request, line_start_pos + line_pre_pos, pos - line_pre_pos);
            }

            /* After each space parsing increase line_space for next parsing.
             * line_pre_pos was the current line's previous parsing pos also the next value's start_position. */
            line_space++;
            line_pre_pos = pos + 1;
        }
    }

    return start_pos;
}



int main(int argc, char *argv[])
{
    char *http_request = "OPTIONS /index.php HTTP/1.1\nContent-Type: application/json; charset=UTF-8\nAccept: image/png, image/gif, image/jpeg\n"
                         "Set-Cookie: JSESSIONID=x83kslf38slfjwl3is8f3, max-age=258000\nContent-Length: 15\r\n\r\na=hello&b=world"
                         "GET /index.php HTTP/1.1\nContent-Type: text/plain; charset=UTF-8\nContent-Length:15\r\n\r\na=hello&b=world"
                         "POST /index.php HTTP/1.1\nContent-Type: text/plain; charset=UTF-8\nContent-Length:15\r\n\r\na=hello&b=world"
                         "DELETE /index.php?accid=x8flfskfji3fslkf HTTP/1.1\nContent-Type: application/json; charset=UTF-8\nContent-Length:23\r\n\r\n{\"name\":\"http_parsing\"}"
                         "GET index.php HTTP/1.1\nContent-Length: 12\r\n\r\nabcdefghijkl";

    unsigned long pos = 0;

    // 循环解析报文:
    while ( 1 ) {
        pos = parse_http_body(http_request, pos);
        printf("[%d]", pos);
    }
}
