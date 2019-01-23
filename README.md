# Simple HTTP Parsing Tool
Simple &amp; Fast &amp; Best Effective HTTP parsing tool written in C.

## APIs
```
 unsigned long parse_http_body(char *http_request_body, unsigned long start_pos);
```

循环调用方法，然后把方法的返回值传入第二个参数来循环解析，当报文体不全的时候，方法会等待报文完整后，重新解析。



