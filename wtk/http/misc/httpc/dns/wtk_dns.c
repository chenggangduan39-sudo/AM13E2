#include "wtk_dns.h" 

static wtk_string_t wtk_dns_def_host[] = {
	wtk_string("cloud.qdreamer.com"),
	wtk_string("resource.qdreamer.com"),
	wtk_string("api.qdreamer.com"),
	wtk_string("www.qdreamer.com"),
};

static wtk_string_t wtk_dns_def_ip[] = {
	wtk_string("101.37.172.108"),
	wtk_string("139.196.8.173"),
	wtk_string("121.199.26.244"),
	wtk_string("121.40.241.129"),
};

void wtk_dns_init(wtk_dns_t *dns)
{
	dns->cfg = NULL;
	dns->log = NULL;
	dns->cache = NULL;
	dns->client = NULL;

	dns->addr = NULL;
	dns->addrlen = 0;
}

wtk_dns_t *wtk_dns_new(wtk_dns_cfg_t *cfg,wtk_log_t *log)
{
	wtk_dns_t *dns;

	dns = (wtk_dns_t*)wtk_malloc(sizeof(wtk_dns_t));
	wtk_dns_init(dns);

	dns->cfg = cfg;
	dns->log = log;

	//wtk_debug("use_cache = %d  cache_path = %.*s\n",cfg->use_cache,cfg->cache_path.len,cfg->cache_path.data);
	if(cfg->use_cache) {
		dns->cache = wtk_dns_cache_new(log,cfg->cache_path.data,cfg->cache_path.len,cfg->cache_day);
	}

	if(cfg->use_dnsc) {
		dns->client = qtk_dnsc_new();
	}

	return dns;
}

void wtk_dns_delete(wtk_dns_t *dns)
{
	if(dns->cfg->use_cache) {
		wtk_dns_cache_delete(dns->cache);
	}

	if(dns->cfg->use_dnsc) {
		qtk_dnsc_delete(dns->client);
	}

	if(dns->addr) {
		wtk_free(dns->addr);
	}

	wtk_free(dns);
}

int wtk_dns_lc(wtk_dns_t *dns,char *host,int hlen,char *port,int plen)
{
	struct sockaddr_in in;
#ifdef WIN32
	ULONG addr;
#else
	in_addr_t addr;
#endif
	char *s,*e,c;
	int octet,n;
	int ret;

	addr = 0;
	octet = 0;
	n = 0;
	s = host;
	e = host + hlen;
	for(;s<e;++s) {
		c = *s;
		if(c>='0' && c<='9') {
			octet = octet*10 + (c-'0');
			if(octet > 255) {
				ret = -1;
				goto end;
			}
			continue;
		}

		if(c == '.') {
			addr = (addr<<8) + octet;
			octet = 0;
			++n;
			continue;
		}
		ret = -1;
		goto end;
	}

	if(n != 3) {
		ret = -1;
		goto end;
	}

	addr = (addr<<8) + octet;
	in.sin_addr.s_addr = htonl(addr);


	in.sin_port = htons((uint16_t)wtk_str_atoi(port,plen));
	in.sin_family = AF_INET;
	dns->addrlen = sizeof(struct sockaddr_in);
	dns->addr = (struct sockaddr*)wtk_malloc(dns->addrlen);
	memcpy(dns->addr,&in,dns->addrlen);

	ret = 0;
end:
	return ret;
}

int wtk_dns_cld_req(wtk_dns_t *dns,char *host,int hlen,char *port,int plen,int timeout)
{
	struct addrinfo hints,*result;
	struct sockaddr_in *in;
	char *sockip;
	char host_tmp[64];
	char port_tmp[16];
	int ret;

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	sprintf(host_tmp,"%.*s",hlen,host);
	sprintf(port_tmp,"%.*s",plen,port);

	ret = getaddrinfo(host_tmp,port_tmp,&hints,&result);
	if(ret != 0) {
		goto end;
	}

	dns->addrlen = result->ai_addrlen;
	dns->addr = (struct sockaddr*)wtk_malloc(dns->addrlen);
	memcpy(dns->addr,result->ai_addr,result->ai_addrlen);

	if(dns->cfg->use_cache) {
		in = (struct sockaddr_in*)dns->addr;
		sockip = inet_ntoa(in->sin_addr);
		wtk_dns_cache_save(dns->cache,host,hlen,sockip,strlen(sockip));
	}

	ret = 0;
end:
	if(ret == 0 && result) {
		freeaddrinfo(result);
	}
	return ret;
}

int wtk_dns_cld_client(wtk_dns_t *dns,char *host,int hlen,char *port,int plen)
{
	wtk_string_t v;
	int ret;

	qtk_dnsc_reset(dns->client);
	v = qtk_dnsc_process(dns->client,host,hlen);
	if(v.len <= 0) {
		ret = -1;
		goto end;
	}

	ret = wtk_dns_lc(dns,v.data,v.len,port,plen);
	if(ret != 0) {
		goto end;
	}

	//wtk_debug("====> domain / host = %.*s  /  %.*s\n",hlen,host,v.len,v.data);

	if(dns->cfg->use_cache) {
		wtk_dns_cache_save(dns->cache,host,hlen,v.data,v.len);
	}

	ret = 0;
end:
	return ret;
}

int wtk_dns_cld_cache(wtk_dns_t *dns,char *host,int hlen,char *port,int plen)
{
	wtk_string_t v;
	int ret;

	v = wtk_dns_cache_find(dns->cache,host,hlen);
	if(v.len <= 0) {
		ret = -1;
		goto end;
	}

	ret = wtk_dns_lc(dns,v.data,v.len,port,plen);
	if(ret != 0) {
		goto end;
	}

	ret = 0;
end:
	return ret;
}

int wtk_dns_cld_def(wtk_dns_t *dns,char *host,int hlen,char *port,int plen)
{
	int ret;
	int i,n;
	int b;

	b = 0;
	n = sizeof(wtk_dns_def_host) / sizeof(wtk_string_t);
	for(i=0;i<n;++i) {
		if(wtk_string_cmp(&(wtk_dns_def_host[i]),host,hlen) == 0) {
			b = 1;
			break;
		}
	}
	if(b) {
		ret = wtk_dns_lc(dns,wtk_dns_def_ip[i].data,wtk_dns_def_ip[i].len,port,plen);
	} else {
		ret = -1;
	}
	return ret;
}

int wtk_dns_cld(wtk_dns_t *dns,char *host,int hlen,char *port,int plen,int timeout)
{
	int ret;

	if(dns->cfg->use_cache) {
		ret = wtk_dns_cld_cache(dns,host,hlen,port,plen);
		if(ret == 0) {
			//wtk_debug("cache dns.\n");
			goto end;
		}
	}

	if(dns->cfg->use_dnsc) {
		ret = wtk_dns_cld_client(dns,host,hlen,port,plen);
		if(ret == 0) {
			//wtk_debug("dnsc req dns.\n");
			goto end;
		}
	} else {
		ret = wtk_dns_cld_req(dns,host,hlen,port,plen,timeout);
		if(ret == 0) {
			//wtk_debug("cld req dns.\n");
			goto end;
		}
	}

	ret = wtk_dns_cld_def(dns,host,hlen,port,plen);
	if(ret == 0) {
		//wtk_debug("default dns.\n");
	}
end:
	return ret;
}

int wtk_dns_process(wtk_dns_t *dns,char *host,int hlen,char *port,int plen)
{
	int ret;

	if(dns->addr) {
		wtk_free(dns->addr);
		dns->addr = NULL;
	}

	ret = wtk_dns_lc(dns,host,hlen,port,plen);
	if(ret == 0) {
		goto end;
	}

	ret = wtk_dns_cld(dns,host,hlen,port,plen,dns->cfg->dns_timeout);
end:
	return ret;
}

void wtk_dns_clean_cache(wtk_dns_t *dns,char *host,int len)
{
	if(dns->cfg->use_cache) {
		wtk_dns_cache_clean(dns->cache,host,len);
	}
}
