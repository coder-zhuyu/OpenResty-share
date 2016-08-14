#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_request.h>
#include <syslog.h>

static ngx_int_t ngx_http_hello_handler(ngx_http_request_t * r);
static char *    ngx_http_hello(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static ngx_str_t ngx_http_hello_get_body(ngx_http_request_t * r);
static void      ngx_http_hello_body_handler_callback(ngx_http_request_t * r);
static ngx_int_t ngx_http_hello_body_handler(ngx_http_request_t * r);
static ngx_int_t initprocess(ngx_cycle_t * cycle);
static void      exitprocess(ngx_cycle_t * cycle);
static void *    ngx_http_hello_create_main_conf(ngx_conf_t * cf);

/* *************
 * callback flag
 * *************/
typedef struct {
    ngx_int_t flag;
}my_ctx_t;

/* ***********************************************
 * set parse method of all conf items
 * ***********************************************/
static ngx_command_t ngx_http_hello_commands[] = {
    {
        ngx_string( "hello" ),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        ngx_http_hello,
        NGX_HTTP_LOC_CONF_OFFSET,
        0 ,
        NULL
    },
    ngx_null_command
};

/* ***************************************************
 * callback function
 * ***************************************************/
static ngx_http_module_t ngx_http_hello_module_ctx = {
    NULL,                              /*preconfiguration*/
    NULL,                              /*postconfiguration*/
    NULL,                              /*create main configuration*/
    NULL,                              /*init main configuration*/
    NULL,                              /*create server configuration*/
    NULL,                              /*merge server configuration*/
    NULL,                              /*create location configuration*/
    NULL                               /*merge location configuration*/
};

/* ***********************************
 * module defination
 * ***********************************/
ngx_module_t ngx_http_hello_module = {
    NGX_MODULE_V1,
    &ngx_http_hello_module_ctx,       /*module context*/
    ngx_http_hello_commands,          /*module directives*/
    NGX_HTTP_MODULE,                   /*module type*/
    NULL,                              /*init master*/
    NULL,                              /*init module*/
    initprocess,                       /*init process*/
    NULL,                              /*init thread*/
    NULL,                              /*exit thread*/
    exitprocess,                       /*exit process*/
    NULL,                              /*exit master*/
    NGX_MODULE_V1_PADDING
};

/* ***********************************************************************
 * process conf itmes
 * ************************************************************************/
static char * ngx_http_hello(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_core_loc_conf_t * clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_hello_handler;

    return NGX_CONF_OK;
}

/* ***********************************************************
 * process request
 * ***********************************************************/
static ngx_int_t ngx_http_hello_handler(ngx_http_request_t * r)
{
    my_ctx_t * myctx;
    myctx = ngx_http_get_module_ctx(r, ngx_http_hello_module);
    if (myctx == NULL)
    {
        myctx = ngx_pcalloc(r->pool, sizeof (my_ctx_t));
        if (myctx == NULL)
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        myctx->flag = 0 ;
        ngx_http_set_ctx(r, myctx, ngx_http_hello_module);

        /*read body, then call ngx_http_hello_body_handler*/
        ngx_int_t rc = ngx_http_read_client_request_body(r,ngx_http_hello_body_handler_callback);
        if (rc >= NGX_HTTP_SPECIAL_RESPONSE)
        {
            return rc;
        }
        return NGX_DONE;
    }

    if ( 1 == myctx->flag)
    {
        return  ngx_http_hello_body_handler(r);
    }

}

/* *************************************************************
 * set flag=1, call ngx_http_hello_handler, out response
 * *************************************************************/
static void ngx_http_hello_body_handler_callback(ngx_http_request_t * r)
{
    my_ctx_t * myctx;
    myctx = ngx_http_get_module_ctx(r, ngx_http_hello_module);
    if (myctx == NULL)
    {
        return ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    }
    myctx->flag = 1 ;
    ngx_http_finalize_request(r, ngx_http_hello_handler(r));
}

/******************************************************
 * handler after finish read request body
 ******************************************************/
static ngx_int_t ngx_http_hello_body_handler(ngx_http_request_t * r)
{
    /*read body, return ngx_str_t*/
    /*ngx_str_t body = ngx_http_hello_get_body(r);*/

    ngx_str_t body = ngx_string("hello world\n");

    /*http response*/
    ngx_str_t type = ngx_string( "text/plain" );
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = body.len;
    r->headers_out.content_type = type;

    ngx_int_t rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    ngx_buf_t * b;
    b = ngx_create_temp_buf(r->pool, body.len);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_memcpy(b->pos, body.data, body.len);
    b->last = b->pos + body.len;
    b->last_buf = 1 ;

    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;


    return ngx_http_output_filter(r, & out);
    /*http response end*/
}

/**************************************************************
 * get request body
 **************************************************************/
static ngx_str_t ngx_http_hello_get_body(ngx_http_request_t * r)
{
    u_char * p;
    u_char * data;
    size_t len;
    //unsigned int len;
    ngx_buf_t * buf;
    ngx_buf_t * next;
    ngx_chain_t * cl;
    ngx_str_t body = ngx_null_string;
    if (r->request_body == NULL || r->request_body->bufs == NULL)
    {
        return body;
    }
    /*
       if(r->request_body->temp_file)
       {
       body = r->request_body->temp_file->file.name;
       return body;
       }
       */
    cl = r->request_body->bufs;
    buf = cl->buf;
    if (cl->next == NULL)
    {
        len = buf->last - buf->pos;
        p = ngx_pnalloc(r->pool, len + 1 );
        if (p == NULL)
        {
            return body;
        }
        data = p;
        ngx_memcpy(p, buf->pos, len);
        data[len] = 0 ;
    }
    else
    {
        next = cl->next->buf;
        len = (buf->last - buf->pos) + (next->last - next->pos);
        p = ngx_pnalloc(r->pool, len + 1 );
        data = p;
        if (NULL == p)
        {
            return body;
        }
        p = ngx_cpymem(p, buf->pos, buf->last - buf->pos);
        ngx_memcpy(p, next->pos, next->last - next->pos);
        data[len] = 0 ;
    }
    body.len = len;
    body.data = data;
    return body;
}

/* *******************************************************
 * create main conf
 * ********************************************************/
static void * ngx_http_hello_create_main_conf(ngx_conf_t * cf)
{
    return NULL;
}


/* *************************************
 * init process
 * *************************************/
static ngx_int_t initprocess(ngx_cycle_t * cycle)
{
    return NGX_OK;
}

/* *********************************
 * exit process
 * ********************************/
static void exitprocess(ngx_cycle_t * cycle)
{

}

