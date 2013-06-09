#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_flag_t     enable;
} ngx_http_req_pass_conf_t;


static void *ngx_http_req_pass_create_conf(ngx_conf_t *cf);
static char *ngx_http_req_pass_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);
static ngx_int_t ngx_http_req_pass_init(ngx_conf_t *cf);
static char *ngx_http_req_pass_set(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_http_req_pass_commands[] = {

    { ngx_string("req_pass"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_http_req_pass_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_req_pass_module_ctx = {
    NULL,                               /* preconfiguration */
    ngx_http_req_pass_init,             /* postconfiguration */

    NULL,                               /* create main configuration */
    NULL,                               /* init main configuration */

    NULL,                               /* create server configuration */
    NULL,                               /* merge server configuration */

    ngx_http_req_pass_create_conf,      /* create location configuration */
    ngx_http_req_pass_merge_conf        /* merge location configuration */
};


ngx_module_t  ngx_http_req_pass_module = {
    NGX_MODULE_V1,
    &ngx_http_req_pass_module_ctx, /* module context */
    ngx_http_req_pass_commands,    /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};


static void *
ngx_http_req_pass_create_conf(ngx_conf_t *cf)
{
    ngx_http_req_pass_conf_t  *rpcf;

    rpcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_req_pass_conf_t));
    if (rpcf == NULL) {
        return NULL;
    }

    rpcf->enable = NGX_CONF_UNSET;

    return rpcf;
}


static char *
ngx_http_req_pass_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_req_pass_conf_t *rpcf = conf;

    rpcf->enable = 1;

    return NGX_CONF_OK;
}


static char *
ngx_http_req_pass_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_req_pass_conf_t *prev = parent;
    ngx_http_req_pass_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_req_pass_handler(ngx_http_request_t *r)
{
    ngx_http_req_pass_conf_t  *rpcf;

    rpcf = ngx_http_get_module_loc_conf(r, ngx_http_req_pass_module);
    if (!rpcf->enable) {
        return NGX_DECLINED;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "request pass");

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_req_pass_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_req_pass_handler;

    return NGX_OK;
}
