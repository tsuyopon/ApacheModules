/* 
**  mod_helloworld.c -- Apache sample helloworld module
**  [Autogenerated via ``apxs -n helloworld -g'']
**
**  To play with this sample module first compile it into a
**  DSO file and install it into Apache's modules directory 
**  by running:
**
**    $ apxs -c -i mod_helloworld.c
**
**  Then activate it in Apache's httpd.conf file for instance
**  for the URL /helloworld in as follows:
**
**    #   httpd.conf
**    LoadModule helloworld_module modules/mod_helloworld.so
**    <Location /helloworld>
**    SetHandler helloworld
**    </Location>
**
**  Then after restarting Apache via
**
**    $ apachectl restart
**
**  you immediately can request the URL /helloworld and watch for the
**  output of this module. This can be achieved for instance via:
**
**    $ lynx -mime_header http://localhost/helloworld 
**
**  The output should be similar to the following one:
**
**    HTTP/1.1 200 OK
**    Date: Tue, 31 Mar 1998 14:42:22 GMT
**    Server: Apache/1.3.4 (Unix)
**    Connection: close
**    Content-Type: text/html
**  
**    The sample page from mod_helloworld.c
*/ 

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"

/* The sample content handler */
static int helloworld_handler(request_rec *r)
{
    if (strcmp(r->handler, "helloworld")) {
        return DECLINED;
    }
    r->content_type = "text/html";      

    if (!r->header_only)
        ap_rputs("The sample page from mod_helloworld.c\n", r);
    return OK;
}

static void helloworld_register_hooks(apr_pool_t *p)
{
    ap_hook_handler(helloworld_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA helloworld_module = {
    STANDARD20_MODULE_STUFF, 
    NULL,                  /* create per-dir    config structures */
    NULL,                  /* merge  per-dir    config structures */
    NULL,                  /* create per-server config structures */
    NULL,                  /* merge  per-server config structures */
    NULL,                  /* table of config file commands       */
    helloworld_register_hooks  /* register hooks                      */
};

