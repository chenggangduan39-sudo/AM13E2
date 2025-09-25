#include "qtk_dns.h"

static wtk_string_t qtk_dns_def_host[] = {
	wtk_string("cloud.qdreamer.com"),
	wtk_string("resource.qdreamer.com"),
	wtk_string("api.qdreamer.com"),
	wtk_string("www.qdreamer.com"),
};

static wtk_string_t qtk_dns_def_ip[] = {
	wtk_string("101.37.172.108"),
	wtk_string("139.196.8.173"),
	wtk_string("121.199.26.244"),
	wtk_string("121.40.241.129"),
};

void qtk_dns_init(qtk_dns_t *dns)
{
	dns->cfg = NULL;
	dns->log = NULL;

	dns->dnsc  = NULL;

	dns->addr = NULL;
	dns->addrlen = 0;
}

qtk_dns_t *qtk_dns_new(qtk_dns_cfg_t *cfg,wtk_log_t *log)
{
	qtk_dns_t *dns;

	dns = (qtk_dns_t*)wtk_malloc(sizeof(qtk_dns_t));
	qtk_dns_init(dns);

	dns->cfg = cfg;
	dns->log = log;

	if(cfg->use_dnsc) {
		dns->dnsc = qtk_dnsc_new();
	}

	return dns;
}

void qtk_dns_delete(qtk_dns_t *dns)
{
	if(dns->cfg->use_dnsc) {
		qtk_dnsc_delete(dns->dnsc);
	}

	if(dns->addr) {
		wtk_free(dns->addr);
	}

	wtk_free(dns);
}

int qtk_dns_lc(qtk_dns_t *dns,char *host,int hlen,char *port,int plen)
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

	in.sin_port = plen>0 ? htons((uint16_t)wtk_str_atoi(port,plen)) : 0;
	in.sin_family = AF_INET;
	dns->addrlen = sizeof(struct sockaddr_in);
	dns->addr = (struct sockaddr*)wtk_malloc(dns->addrlen);
	memcpy(dns->addr,&in,dns->addrlen);

	ret = 0;
end:
	return ret;
}

int qtk_dns_cld_req(qtk_dns_t *dns,char *host,int hlen,char *port,int plen,int timeout)
{
	struct addrinfo hints,*result;
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
	if(plen > 0) {
		sprintf(port_tmp,"%.*s",plen,port);
		ret = getaddrinfo(host_tmp,port_tmp,&hints,&result);
	} else {
		ret = getaddrinfo(host_tmp,NULL,&hints,&result);
	}

	if(ret != 0) {
		goto end;
	}

	dns->addrlen = result->ai_addrlen;
	dns->addr = (struct sockaddr*)wtk_malloc(dns->addrlen);
	memcpy(dns->addr,result->ai_addr,result->ai_addrlen);

	ret = 0;
end:
	if(ret == 0 && result) {
		freeaddrinfo(result);
	}
	return ret;
}

int qtk_dns_cld_client(qtk_dns_t *dns,char *host,int hlen,char *port,int plen)
{
	wtk_string_t v;
	int ret;

	qtk_dnsc_reset(dns->dnsc);
	v = qtk_dnsc_process(dns->dnsc,host,hlen);
	if(v.len <= 0) {
		ret = -1;
		goto end;
	}

	ret = qtk_dns_lc(dns,v.data,v.len,port,plen);
	if(ret != 0) {
		goto end;
	}

	//wtk_debug("====> domain / host = %.*s  /  %.*s\n",hlen,host,v.len,v.data);

	ret = 0;
end:
	return ret;
}

int qtk_dns_cld_def(qtk_dns_t *dns,char *host,int hlen,char *port,int plen)
{
	int ret;
	int i,n;
	int b;

	b = 0;
	n = sizeof(qtk_dns_def_host) / sizeof(wtk_string_t);
	for(i=0;i<n;++i) {
		if(wtk_string_cmp(&(qtk_dns_def_host[i]),host,hlen) == 0) {
			b = 1;
			break;
		}
	}
	if(b) {
		ret = qtk_dns_lc(dns,qtk_dns_def_ip[i].data,qtk_dns_def_ip[i].len,port,plen);
	} else {
		ret = -1;
	}
	return ret;
}

int qtk_dns_cld(qtk_dns_t *dns,char *host,int hlen,char *port,int plen,int timeout)
{
	int ret;

	if(dns->cfg->use_dnsc) {
		ret = qtk_dns_cld_client(dns,host,hlen,port,plen);
		if(ret == 0) {
			wtk_log_log0(dns->log,"dnsc req dns");
			//wtk_debug("dnsc req dns.\n");
			goto end;
		}
	} else {
		ret = qtk_dns_cld_req(dns,host,hlen,port,plen,timeout);
		if(ret == 0) {
			wtk_log_log0(dns->log,"cld req dns");
			//wtk_debug("cld req dns.\n");
			goto end;
		}
	}

	ret = qtk_dns_cld_def(dns,host,hlen,port,plen);
	if(ret == 0) {
		wtk_log_log0(dns->log,"default dns");
		//wtk_debug("default dns.\n");
	}
end:
	return ret;
}

int qtk_dns_process(qtk_dns_t *dns,char *host,int hlen,char *port,int plen)
{
	int ret;

	if(dns->addr) {
		wtk_free(dns->addr);
		dns->addr = NULL;
	}

	wtk_log_log0(dns->log,"dns process start");

	ret = qtk_dns_lc(dns,host,hlen,port,plen);
	if(ret == 0) {
		goto end;
	}

	ret = qtk_dns_cld(dns,host,hlen,port,plen,dns->cfg->timeout);
end:
	wtk_log_log(dns->log,"dns process end ret = %d",ret);
	return ret;
}

