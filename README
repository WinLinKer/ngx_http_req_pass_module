## Description
This module can be used to limit the requests in one second or minute. It's not like the `ngx_http_limit_req_module`, it just counts the requests and reject the part of over the limited.

### Config Example

file: conf/nginx.conf

    daemon off;
    error_log logs/error.log debug;

    events {
    }

    http {

        server {
            listen   8080;

            location / {
                req_pass rate=1r/s action=/punish;
            }

            location /punish {
                echo punish;
            }
        }
    }

## Installation
```bash

$ git clone git@github.com:yzprofile/ngx_http_req_pass_module.git
$ ./configure --add-module=./ngx_http_req_pass_module

```

## Directives

Syntax: **req_pass** `rate=1r/s` `action='/location'`

Default: `none`

Context: `loc`


## Copyright & License

These codes are licenced under the BSD license.

Copyright (C) 2012-2013 by Zhuo Yuan (yzprofile) <yzprofiles@gmail.com>, Alibaba Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
