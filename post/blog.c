#include <ctype.h>
#include <string.h>
#include <httpd.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_log.h>
#include <apr_hash.h>
#include <apr_strings.h>
 
#define MAX_SIZE 1048576
 
static void blog_hooks(apr_pool_t *pool);
static int blog_handler(request_rec *r);
 
module AP_MODULE_DECLARE_DATA blog_module = {
    STANDARD20_MODULE_STUFF,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    blog_hooks
};
 
static void blog_hooks(apr_pool_t *pool)
{
    ap_hook_handler(blog_handler, NULL, NULL, APR_HOOK_MIDDLE);
}
 
static apr_hash_t *parse_form_from_string(request_rec *r, char *args)
{
    apr_hash_t *form;
    apr_array_header_t *values;
    char *pair;
    char *eq;
    const char *delim = "&";
    char *last;
    if (args == NULL) {
        return NULL;
    }
 
    form = apr_hash_make(r->pool);
 
    for (pair = apr_strtok(args, delim, &last);
         pair != NULL;
         pair = apr_strtok(NULL, delim, &last)){
        for (eq = pair; *eq; ++eq){
            if(*eq == '+'){
                *eq = ' ';
            }
        }
        eq = strchr(pair, '=');
        if (eq) {
            *eq++ = '\0';
            ap_unescape_url(pair);
            ap_unescape_url(eq);
        }
        else {
            eq = "";
            ap_unescape_url(pair);
        }
        values = apr_hash_get(form, pair, APR_HASH_KEY_STRING);
        if (values == NULL) {
            values = apr_array_make(r->pool, 1, sizeof(char*));
            apr_hash_set(form, pair, APR_HASH_KEY_STRING, values);
        }
        *((char **)apr_array_push(values)) = apr_pstrdup(r->pool, eq);
    }
 
    return form;
}
 
static int parse_form_from_POST(request_rec *r, apr_hash_t **form)
{
    int bytes, eos;
    apr_size_t count;
    apr_status_t rv;
    apr_bucket_brigade *bb;
    apr_bucket_brigade *bbin;
    char *buf;
    apr_bucket *b;
    apr_bucket *nextb;
    const char *clen = apr_table_get(r->headers_in, "Content-Length");
 
    if (clen != NULL){
        bytes = strtol(clen, NULL, 0);
        if (bytes >= MAX_SIZE) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "Request too big (%d bytes; limit %d)",
                          bytes, MAX_SIZE);
            return HTTP_REQUEST_ENTITY_TOO_LARGE;
        }
    }
    else {
        bytes = MAX_SIZE;
    }
    bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    bbin = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    count = 0;
    eos = 0;
    do {
        rv = ap_get_brigade(r->input_filters, bbin, AP_MODE_READBYTES,
                            APR_BLOCK_READ, bytes);
 
        if (rv != APR_SUCCESS) {
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        for (b = APR_BRIGADE_FIRST(bbin);
             b != APR_BRIGADE_SENTINEL(bbin);
             b = nextb) {
            nextb = APR_BUCKET_NEXT(b);
            if (APR_BUCKET_IS_EOS(b)) {
                eos = 1;
            }
            if (!APR_BUCKET_IS_METADATA(b)) {
                if (b->length != (apr_size_t)(-1)) {
                    count += b->length;
                    if (count > MAX_SIZE) {
                        apr_bucket_delete(b);
                    }
                }
            }
            if (count <= MAX_SIZE) {
                APR_BUCKET_REMOVE(b);
                APR_BRIGADE_INSERT_TAIL(bb, b);
            }
        }
    }while (!eos);
    if (count > MAX_SIZE) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                      "Request too big (%d bytes; limit %d)",
                      (int)count, MAX_SIZE);
        return HTTP_REQUEST_ENTITY_TOO_LARGE;
    }
    buf = apr_palloc(r->pool, count+1);
    rv = apr_brigade_flatten(bb, buf, &count);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                      "Error (flatten) reading form data");
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    buf[count] = '\0';
    *form = parse_form_from_string(r, buf);
    return OK;
}
 
static int blog_handler(request_rec *r)
{
    int rv = 0;
    apr_hash_t *formdata = NULL;
 
    if (!r->handler || (strcmp(r->handler, "blog") != 0)) {
        return DECLINED ;
    }
 
    if (r->method_number != M_POST) {
        return HTTP_METHOD_NOT_ALLOWED;
    }
 
    const char * ctype = apr_table_get(r->headers_in, "Content-Type");
    if (ctype && (strcasecmp(ctype,
                             "application/x-www-form-urlencoded") == 0)){
        rv =parse_form_from_POST(r, &formdata);
        if (rv != OK){
            return rv;
        }
    }
 
    ap_set_content_type(r, "text/html;charset=utf-8");
    ap_rputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">"
             "<html><head><title>Apache Module</title></head>"
             "<body>"
             "<p>This is my Apache module!</p>",
             r);
 
    if (formdata == NULL) {
        ap_rputs("<p>No form data found.</p>", r);
    }
    else {
        apr_array_header_t *arr;
        char *key;
//        char *key, *p;
        apr_ssize_t klen;
        apr_hash_index_t *index;
        char **val_ptr;
 
        ap_rprintf(r, "<h2>Form data supplied by method %s </h2><dl>",
                   r->method);
 
        for (index = apr_hash_first(r->pool, formdata);
             index != NULL;
             index = apr_hash_next(index)){
            apr_hash_this(index,
                          (const void **)&key,
                          &klen,
                          (void **)&arr);
            ap_rprintf(r, "<dt>%s</dt>", ap_escape_html(r->pool, key));
            for (val_ptr = apr_array_pop(arr);
                 val_ptr != NULL;
                 val_ptr = apr_array_pop(arr)){
                char *val = *val_ptr;
                ap_rprintf(r, "<dd>%s</dd>",
                           ap_escape_html(r->pool, val));
            }
        }
        ap_rputs("</dl>", r);
    }
    ap_rputs("</body></html>", r);
    return OK;
}
