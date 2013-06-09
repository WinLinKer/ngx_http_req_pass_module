#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define NGX_HTTP_REQ_PASS_SHM_NAME_LEN 256


typedef struct {
    ngx_uint_t                  count;
    ngx_uint_t                  time;
} ngx_http_req_pass_shctx_t;


typedef struct {
    ngx_flag_t                  enable;
    ngx_uint_t                  count;
    ngx_uint_t                  scale;
    ngx_str_t                   action;
    ngx_slab_pool_t            *shpool;
    ngx_http_req_pass_shctx_t  *shctx;
} ngx_http_req_pass_conf_t;


static void *ngx_http_req_pass_create_conf(ngx_conf_t *cf);
static char *ngx_http_req_pass_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);
static ngx_int_t ngx_http_req_pass_init(ngx_conf_t *cf);
static char *ngx_http_req_pass_set(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static ngx_int_t ngx_http_req_pass_init_shm_zone(ngx_shm_zone_t *shm_zone,
    void *data);
static ngx_int_t ngx_http_req_pass_get_shm_name(ngx_str_t *shm_name,
    ngx_pool_t *pool, ngx_uint_t generation);
static ngx_int_t ngx_http_req_pass_over(ngx_http_request_t *r,
    ngx_str_t *action);


static ngx_command_t  ngx_http_req_pass_commands[] = {

    { ngx_string("req_pass"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
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


static ngx_uint_t ngx_http_req_pass_shm_generation = 0;


static void *
ngx_http_req_pass_create_conf(ngx_conf_t *cf)
{
    ngx_http_req_pass_conf_t  *rpcf;

    rpcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_req_pass_conf_t));
    if (rpcf == NULL) {
        return NULL;
    }

    rpcf->enable = NGX_CONF_UNSET;
    rpcf->count = NGX_CONF_UNSET_UINT;
    rpcf->scale = NGX_CONF_UNSET_UINT;
    rpcf->shpool = NGX_CONF_UNSET_PTR;
    rpcf->shctx = NGX_CONF_UNSET_PTR;

    /* set by pcalloc
       rpcf->action = { NULL }
     */

    return rpcf;
}


static char *
ngx_http_req_pass_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_req_pass_conf_t *rpcf = conf;

    u_char          *p;
    size_t           len;
    ngx_str_t        shm_name, action, s, *value;
    ngx_int_t        count, scale;
    ngx_uint_t       i;
    ngx_shm_zone_t  *shm_zone;


    value = cf->args->elts;
    ngx_str_null(&action);

    count = 0;
    scale = 0;

    for (i = 1; i < cf->args->nelts; i++) {

        if (ngx_strncmp(value[i].data, "rate=", 5) == 0) {

            len = value[i].len;
            p = value[i].data + len - 3;

            if (ngx_strncmp(p, "r/s", 3) == 0) {
                scale = 1;
                len -= 3;

            } else if (ngx_strncmp(p, "r/m", 3) == 0) {
                scale = 60;
                len -= 3;
            }

            s.len = value[i].len - 5;
            s.data = value[i].data + 5;

            count = ngx_atoi(value[i].data + 5, len - 5);
            if (count <= NGX_ERROR) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid rate \"%V\" %V %i",
                                   &value[i], &s, count);
                return NGX_CONF_ERROR;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "action=", 7) == 0) {

            s.len = value[i].len - 7;
            s.data = value[i].data + 7;

            if (s.len < 2 || (s.data[0] != '@' && s.data[0] != '/')) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid action \"%V\"", &value[i]);
                return NGX_CONF_ERROR;
            }

            action = s;

            continue;
        }
    }

    ngx_http_req_pass_shm_generation++;

    if (ngx_http_req_pass_get_shm_name(&shm_name, cf->pool,
                                       ngx_http_req_pass_shm_generation)
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    shm_zone = ngx_shared_memory_add(cf, &shm_name,
                                     ngx_pagesize * 8
                                     + sizeof(ngx_http_req_pass_shctx_t),
                                     &ngx_http_req_pass_module);
    if (shm_zone == NULL) {
        return NGX_CONF_ERROR;
    }

    shm_zone->data = rpcf;
    shm_zone->init = ngx_http_req_pass_init_shm_zone;

    if (count <=0 || scale <=0 ) {
        return "r/s | r/m is error";
    }

    rpcf->enable = 1;
    rpcf->count = count;
    rpcf->scale = scale;
    rpcf->action = action;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_req_pass_init_shm_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_slab_pool_t            *shpool;
    ngx_http_req_pass_conf_t   *rpcf;
    ngx_http_req_pass_shctx_t  *sh;

    rpcf = shm_zone->data;
    shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;

    sh = ngx_slab_alloc(shpool, sizeof(ngx_http_req_pass_shctx_t));
    if (sh == NULL) {
        return NGX_ERROR;
    }

    ngx_memzero(sh, sizeof(ngx_http_req_pass_shctx_t));

    rpcf->shctx = sh;
    rpcf->shpool = shpool;

    return NGX_OK;
}


static ngx_int_t
ngx_http_req_pass_get_shm_name(ngx_str_t *shm_name, ngx_pool_t *pool,
    ngx_uint_t generation)
{
    u_char  *last;

    shm_name->data = ngx_palloc(pool, NGX_HTTP_REQ_PASS_SHM_NAME_LEN);
    if (shm_name->data == NULL) {
        return NGX_ERROR;
    }

    last = ngx_snprintf(shm_name->data, NGX_HTTP_REQ_PASS_SHM_NAME_LEN,
                        "%s#%ui", "ngx_http_req_pass_module", generation);

    shm_name->len = last - shm_name->data;

    return NGX_OK;
}


static char *
ngx_http_req_pass_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_req_pass_conf_t *prev = parent;
    ngx_http_req_pass_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_uint_value(conf->count, prev->count, 0);
    ngx_conf_merge_uint_value(conf->scale, prev->scale, 0);
    ngx_conf_merge_ptr_value(conf->shpool, prev->shpool, NULL);
    ngx_conf_merge_ptr_value(conf->shctx, prev->shctx, NULL);

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_req_pass_handler(ngx_http_request_t *r)
{
    ngx_flag_t                  over;
    struct timeval              tv;
    ngx_http_req_pass_conf_t   *rpcf;
    ngx_http_req_pass_shctx_t  *ctx;

    rpcf = ngx_http_get_module_loc_conf(r, ngx_http_req_pass_module);
    if (!rpcf->enable) {
        return NGX_DECLINED;
    }

    over = 0;
    ctx = rpcf->shctx;
    ngx_gettimeofday(&tv);

    ngx_shmtx_lock(&rpcf->shpool->mutex);

    if (ctx->time == 0) {
        ctx->time = tv.tv_sec;
    }

    if (tv.tv_sec - ctx->time >= rpcf->scale) {
        ctx->count = 1;
        ctx->time = tv.tv_sec;

    } else {

        if (ctx->count >= rpcf->count) {
            over = 1;
        } else {
            ctx->count++;
        }
    }

    ngx_shmtx_unlock(&rpcf->shpool->mutex);

    if (over) {
        return ngx_http_req_pass_over(r, &rpcf->action);
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "request pass %ui", ctx->count);

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_req_pass_over(ngx_http_request_t *r, ngx_str_t *action)
{
    if (action->len == 0) {
        return NGX_HTTP_SERVICE_UNAVAILABLE;
    }

    if (action->data[0] == '@') {
        (void) ngx_http_named_location(r, action);

    } else {
        (void) ngx_http_internal_redirect(r, action, &r->args);
    }

    ngx_http_finalize_request(r, NGX_DONE);
    return NGX_DONE;
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
