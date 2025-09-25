#include "qtk_dnsc.h"
#include "wtk/os/wtk_log.h"
#include <stdint.h>

typedef struct {
#pragma pack(1)
	uint16_t      transaction_ID;
	uint16_t      flags;
	uint16_t      questions;
	uint16_t      answer_RRs;
	uint16_t      authority_RRs;
	uint16_t      additional_RRs;
#pragma pack()
}qtk_dns_hdr_t;
#define QTK_DNS_HDR_LENGTH 12

typedef struct {
#pragma pack(1)
	uint16_t type;
	uint16_t classIN;
	uint32_t ttl;
	uint16_t length;
#pragma pack()
}qtk_rr_hdr_t;
#define QTK_RR_HDR_LENGTH 10

typedef struct {
#pragma pack(1)
	uint16_t type;
	uint16_t classIN;
#pragma pack()
}qtk_query_hdr_t;
#define QTK_QUERY_HDR_LENGTH 4


enum {
	DNS_OPCODE_INQUIRY = 0,
	// Others not supported in this file
};

enum {
	DNS_RR_TYPE_A      = 1,
	DNS_RR_TYPE_AAAA   = 28,
	DNS_RR_TYPE_NS     = 2,
	DNS_RR_TYPE_CNAME  = 5,
	// Others not supported in this file
};

enum {
	DNS_CLASS_INTERNET = 1,
};


enum {
	DNS_REPLYCODE_NO_ERROR               = 0,
	DNS_REPLYCODE_FMT_ERR                = 1,
	DNS_REPLYCODE_SRV_ERR                = 2,
	DNS_REPLYCODE_NAME_NOT_EXIST         = 3,
	DNS_REPLYCODE_REJECTED               = 5,
	DNS_REPLYCODE_NAME_SHOULD_NOT_APPEAR = 6,
	DNS_REPLYCODE_RR_NOT_EXIST           = 7,
	DNS_REPLYCODE_RR_INQUIRY_FAIL        = 8,
	DNS_REPLYCODE_SRV_AUTH_FAIL          = 9,
	DNS_REPLYCODE_NAME_OUT_OF_AREA       = 10,
};


#define QTK_DNS_BITS_ANY_SET(val, bits)		(0 != ((val) & (bits)))
#define QTK_DNS_BITS_ALL_SET(val, bits)		((bits) == ((val) & (bits)))
#define QTK_DNS_BITS_SET(val, bits)			((val) |= (bits))
#define QTK_DNS_BITS_CLR(val, bits)			((val) &= ~(bits))




static wtk_string_t qtk_dns_default_svr[] = {
		//wtk_string("127.0.1.1"),      // gateway default DNS
		wtk_string("223.5.5.5"),        // alibaba DNS
		//wtk_string("223.6.6.6"),		// alibaba DNS
		wtk_string("114.114.114.114"),  // 114DNS
		//wtk_string("114.114.114.115"),  // 114DNS
		//wtk_string("112.124.47.27"),    // oneDNS
		//wtk_string("114.215.126.16"),   // oneDNS
		//wtk_string("101.226.4.6"),      // small DNS inland
		//wtk_string("208.67.222.222"),   // Open DNS
		//wtk_string("208.67.220.220"),   // Open DNS
		//wtk_string("8.8.8.8"),			// google DNS
		//wtk_string("8.8.4.4"),			// google DNS
};


qtk_dnsc_item_t* qtk_dnsc_item_new()
{
	qtk_dnsc_item_t *item;

	item = (qtk_dnsc_item_t*)wtk_malloc(sizeof(qtk_dnsc_item_t));
	item->name = 0;
	item->rlt = 0;
	item->type = -1;
	return item;
}

void qtk_dnsc_item_delete(qtk_dnsc_item_t *item)
{
	if(item->name) {
		wtk_string_delete(item->name);
	}
	if(item->rlt) {
		wtk_string_delete(item->rlt);
	}
	wtk_free(item);
}

qtk_dnsc_t* qtk_dnsc_new()
{
	qtk_dnsc_t *dns;

	dns = (qtk_dnsc_t*)wtk_malloc(sizeof(qtk_dnsc_t));
	wtk_queue_init(&(dns->rlt_q));
	dns->buf = wtk_strbuf_new(256,1);
	return dns;
}

void qtk_dnsc_delete(qtk_dnsc_t *dns)
{
	qtk_dnsc_reset(dns);
	wtk_strbuf_delete(dns->buf);
	wtk_free(dns);
}

static uint16_t qtk_dns_gen_req_flags()
{
	uint16_t ret = 0;

	QTK_DNS_BITS_SET(ret,DNS_OPCODE_INQUIRY & 0X0F << 11);
	QTK_DNS_BITS_SET(ret,(1 << 8));
	return ret;
}

static void qtk_dnsc_req_append_hdr(wtk_strbuf_t *buf)
{
	qtk_dns_hdr_t dns_hdr;

	dns_hdr.transaction_ID = (uint16_t)rand();
	dns_hdr.flags = htons(qtk_dns_gen_req_flags());
	dns_hdr.questions = htons(1);
	dns_hdr.answer_RRs = 0;
	dns_hdr.authority_RRs = 0;
	dns_hdr.additional_RRs = 0;
	wtk_strbuf_push(buf,(char*)&dns_hdr,QTK_DNS_HDR_LENGTH);
}

static void qtk_dnsc_req_append_body(wtk_strbuf_t *buf,char *domain,int len)
{
	char dname[128];
	char *s,*e,c;
	uint16_t v;
	int cpos,pos,size;

	s = domain;
	e = domain + len;
	cpos = 0;
	pos = 1;
	size = 0;
	while(s < e) {
		c = *s;
		switch(c) {
		case '.':
			dname[cpos] = size;
			cpos = pos;
			++pos;
			size = 0;
			break;
		default:
			dname[pos++] = c;
			++size;
			break;
		}
		++s;
	}
	dname[cpos] = size;
	dname[pos++] = '\0';
	wtk_strbuf_push(buf,dname,pos);

	v = htons(DNS_RR_TYPE_A);
	wtk_strbuf_push(buf,(char*)&v,2);

	v = htons(DNS_CLASS_INTERNET);
	wtk_strbuf_push(buf,(char*)&v,2);
}

int qtk_dnsc_send(qtk_dnsc_t *dns,int sockfd,char *domain,int dlen,struct sockaddr_in* addr,int slen)
{
	struct sockaddr_in in;
	wtk_strbuf_t *buf = dns->buf;
	int flags = 0;
	int ret;
	int writed;
	int err;

	memcpy(&in,addr,slen);

	wtk_strbuf_reset(buf);
	qtk_dnsc_req_append_hdr(buf);
	qtk_dnsc_req_append_body(buf,domain,dlen);

#ifdef WIN32
	flags = 0;
#else 
	flags |= MSG_DONTWAIT;
#endif
	writed = 0;
	do{
		ret = sendto(sockfd,buf->data+writed,buf->pos-writed,flags,(struct sockaddr*)&in,sizeof(struct sockaddr_in));
		if(ret < 0) {
#ifdef WIN32
			err = WSAGetLastError(); 
			if(err == WSAEINTR) {
#else
			err = errno;
			if (err == EINTR) {
#endif
				continue;
			} else {
				break;
			}
		} else {
			writed += ret;
		}
	}while(writed < buf->pos);

	if(writed != buf->pos) {
		writed = -1;
	}
	return writed;
}

int qtk_dns_resolve_domain(char *domain,char *data,int bytes,char *query,int left,int domainIsEmpty)
{
	if(*query == '\0') {
		domain[0] = '\0';
		return sizeof(uint8_t);
	} else if (QTK_DNS_BITS_ANY_SET((uint8_t)(*query),0xC0)) {
		uint16_t offset;

		memcpy(&offset,query,sizeof(uint16_t));
		offset = ntohs(offset);
		QTK_DNS_BITS_CLR(offset,0xC000);
		qtk_dns_resolve_domain(domain,data,bytes,data+offset,left,domainIsEmpty);
		return sizeof(uint16_t);
	} else {
		uint8_t len = *query;
		int ret = len;

		if(domainIsEmpty) {
			++ret;
			memcpy(domain,query+1,len);
			ret += qtk_dns_resolve_domain(domain+len,data,bytes,query+len+1,left-len-1,0);
		} else {
			++ret;
			domain[0] = '.';
			memcpy(domain+1,query+1,len);
			ret += qtk_dns_resolve_domain(domain+len+1,data,bytes,query+len+1,left-len-1,0);
		}
		return ret;
	}
}

int qtk_dnsc_resolve_query(qtk_dnsc_t *dns,char *data,int bytes,char *query,int left)
{
	qtk_query_hdr_t query_hdr;
	char domain[QTK_DNS_DOMAIN_LEN_MAX+1];
	int len;

	len = qtk_dns_resolve_domain(domain,data,bytes,query,left,1);

	memcpy(&query_hdr,query+len,QTK_QUERY_HDR_LENGTH);
	query_hdr.type = ntohs(query_hdr.type);
	query_hdr.classIN = ntohs(query_hdr.classIN);

	len += QTK_QUERY_HDR_LENGTH;
	return len;
}

int qtk_dnsc_resolve_answers(qtk_dnsc_t *dns,char *data,int bytes,char *query,int left)
{
	qtk_dnsc_item_t *item;
	qtk_rr_hdr_t rr_hdr;
	char para[QTK_DNS_DOMAIN_LEN_MAX+1];
	char value[QTK_DNS_DOMAIN_LEN_MAX+1];
	char *p;
	size_t len;

	p = query;
	len = qtk_dns_resolve_domain(para,data,bytes,p,left,1);
	p += len;

	memcpy(&rr_hdr,p,QTK_RR_HDR_LENGTH);
	rr_hdr.type    = ntohs(rr_hdr.type);
	rr_hdr.classIN = ntohs(rr_hdr.classIN);
	rr_hdr.ttl     = ntohl(rr_hdr.ttl);
	rr_hdr.length  = ntohs(rr_hdr.length);
	len += QTK_RR_HDR_LENGTH;
	p += QTK_RR_HDR_LENGTH;

	switch(rr_hdr.type) {
	case DNS_RR_TYPE_AAAA:
		//not support
		break;
	case DNS_RR_TYPE_A:
		sprintf(value,"%u.%u.%u.%u",(uint8_t)p[0],(uint8_t)p[1],(uint8_t)p[2],(uint8_t)p[3]);
		item = qtk_dnsc_item_new();
		item->type = QTK_DNS_IPV4;
		item->ttl = (time_t)rr_hdr.ttl;
		item->name = wtk_string_dup_data(para,strlen(para));
		item->rlt = wtk_string_dup_data(value,strlen(value));
		wtk_queue_push(&(dns->rlt_q),&(item->q_n));
		break;
	case DNS_RR_TYPE_CNAME:
		//not support
		break;
	case DNS_RR_TYPE_NS:
		//not support
		break;
	default:
		break;
	}
	len += rr_hdr.length;

	return len;
}

int qtk_dnsc_resolve(qtk_dnsc_t *dns,char *data,int bytes)
{
	qtk_dns_hdr_t dns_hdr;
	uint16_t quesRRs;
	uint16_t ansRRs;
#ifdef QTK_DNS_READ_DETAILS
	uint16_t authRRs;
	uint16_t addiRRs;
#endif
	uint16_t replycode;
	char *query;
	int left,tmplen;
	int ret;

	if(bytes < QTK_DNS_HDR_LENGTH) {
		ret = -1;
		goto end;
	}

	query = data;
	left = bytes;

	memcpy(&dns_hdr,query,QTK_DNS_HDR_LENGTH);
	quesRRs = ntohs(dns_hdr.questions);
	ansRRs  = ntohs(dns_hdr.answer_RRs);
#ifdef QTK_DNS_READ_DETAILS
	authRRs = ntohs(dns_hdr.authority_RRs);
	addiRRs = ntohs(dns_hdr.additional_RRs);
#endif
	replycode = (uint16_t) (ntohs(dns_hdr.flags) & 0x0F);

//	wtk_debug("quesRRs = %d\n",quesRRs);
//	wtk_debug("ansRRs = %d\n",ansRRs);
//	wtk_debug("authRRs = %d\n",authRRs);
//	wtk_debug("addiRRs = %d\n",addiRRs);
//	wtk_debug("replycode = %d\n",replycode);

	if(replycode != DNS_REPLYCODE_NO_ERROR) {
		ret = -1;
		goto end;
	}
	left -= QTK_DNS_HDR_LENGTH;
	query += QTK_DNS_HDR_LENGTH;

	while(quesRRs > 0 && left > 0) {
		tmplen = qtk_dnsc_resolve_query(dns,data,bytes,query,left);
		query += tmplen;
		left -= tmplen;
		--quesRRs;
	};

	while(ansRRs > 0 && left > 0) {
		tmplen = qtk_dnsc_resolve_answers(dns,data,bytes,query,left);
		query += tmplen;
		left -= tmplen;
		--ansRRs;
	}

#ifdef QTK_DNS_READ_DETAILS
	while(authRRs > 0 && left > 0) {
		tmplen = qtk_dnsc_resolve_answers(dns,data,bytes,query,left);
		query += tmplen;
		left -= tmplen;
		-- authRRs;
	}

	while(addiRRs > 0 && left > 0) {
		tmplen = qtk_dnsc_resolve_answers(dns,data,bytes,query,left);
		query += tmplen;
		left -= tmplen;
		--addiRRs;
	}
#endif

	ret = 0;
end:
	return ret;
}

int qtk_dnsc_recv1(qtk_dnsc_t *dns,int sockfd,struct sockaddr_in *addr,int socklen)
{
	char tmp[QTK_UDP_PACKAGE_LEN_MAX];
	int ret;
	int readed;
	int err;

	readed = 0;
	do{
#ifdef WIN32
		ret = recvfrom(sockfd, tmp + readed, QTK_UDP_PACKAGE_LEN_MAX - readed, 0, (struct sockaddr*)addr, (int*)&socklen);
#else
		ret = recvfrom(sockfd,tmp+readed,QTK_UDP_PACKAGE_LEN_MAX-readed,0,(struct sockaddr*)addr,(socklen_t*)&socklen);
		//wtk_debug("recv ret = %d\n",ret);
#endif

#ifdef WIN32
		err = WSAGetLastError();
#else
		err = errno;
#endif
		if(ret == 0) {
			break;
		} else if (ret < 0) {

#ifdef WIN32
			if (err == WSAEINTR) {
#else
			if(err == EINTR) {
#endif
				continue;
			} else {
				break;
			}
		} else {
			readed += ret;
			if(ret >= QTK_UDP_PACKAGE_LEN_MAX) {
				break;
			}
		}
	}while(1);

	if(readed <= 0) {
		ret = -1;
		goto end;
	}

	ret = qtk_dnsc_resolve(dns,tmp,readed);

end:
	return ret;
}

int qtk_dnsc_recv(qtk_dnsc_t *dns,int sockfd,struct sockaddr_in *addr,int socklen)
{
	char tmp[QTK_UDP_PACKAGE_LEN_MAX];
	int ret;
	int err;

recv_again:
#ifdef WIN32
	ret = recvfrom(sockfd, tmp, QTK_UDP_PACKAGE_LEN_MAX, 0, (struct sockaddr*)addr, (int*)&socklen);
#else
	ret = recvfrom(sockfd,tmp,QTK_UDP_PACKAGE_LEN_MAX,0,(struct sockaddr*)addr,(socklen_t*)&socklen);
	//wtk_debug("recv ret = %d\n",ret);
#endif

#ifdef WIN32
	err = WSAGetLastError();
#else
	err = errno;
#endif
	if (ret < 0) {
		//wtk_debug("recv ret = %d err = %d\n",ret,err);
#ifdef WIN32
		if (err == WSAEINTR) {
#else
		if(err == EINTR) {
#endif
			goto recv_again;
		}
	} else if(ret > 0) {
		//wtk_debug("recv ret = %d\n",ret);
		ret = qtk_dnsc_resolve(dns,tmp,ret);
	} else {
		//wtk_debug("recv ret = %d\n",ret);
		ret = -1;
	}

	return ret;
}

int qtk_dnsc_process_svr(qtk_dnsc_t *dns,char *domain,int dlen,char* dnssvr,int slen)
{
	struct sockaddr_in addr;
#ifdef WIN32
	int socklen;
	int tv;
#else 
	struct timeval tv;
#endif

	int sockfd = -1;
	char dnssvr_tmp[32];
	int ret;

#ifdef WIN32
	sprintf(dnssvr_tmp, "%.*s:%d", slen, dnssvr, QTK_DNS_SVR_PORT);
	socklen = sizeof(struct sockaddr_in);
	ret = WSAStringToAddress(dnssvr_tmp, AF_INET, 0, (LPSOCKADDR)&(addr), &socklen);
	//wtk_debug("err = %d\n", WSAGetLastError());
	if(ret != 0) {
		ret = -1;
		goto end;
	}
#else
	addr.sin_family = AF_INET;
	addr.sin_port = htons(QTK_DNS_SVR_PORT);
	sprintf(dnssvr_tmp,"%.*s",slen,dnssvr);
	ret = inet_pton(AF_INET,dnssvr_tmp,&(addr.sin_addr));
	if (ret != 1) {
		ret = -1;
		goto end;
	}
#endif

	sockfd = socket(AF_INET,SOCK_DGRAM,0);
#ifdef WIN32
	if (sockfd == INVALID_SOCKET) {
#else 
	if(sockfd == -1) {
#endif
		ret = -1;
		goto end;
	}

#ifdef WIN32
	tv = QTK_DNS_SND_TIMEOUT;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,(char*) &tv, sizeof(int));

	tv = QTK_DNS_RCV_TIMEOUT;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(int));
#else
	tv.tv_sec = QTK_DNS_SND_TIMEOUT / 1000;
	tv.tv_usec = (QTK_DNS_SND_TIMEOUT % 1000) * 1e3;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	tv.tv_sec = QTK_DNS_RCV_TIMEOUT / 1000;
	tv.tv_usec = (QTK_DNS_RCV_TIMEOUT % 1000) * 1e3;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif


	ret = qtk_dnsc_send(dns,sockfd,domain,dlen,&addr,sizeof(struct sockaddr_in));
	wtk_log_log(glb_log,"send ret %d",ret);
	if(ret < 0) {
		goto end;
	}

	ret = qtk_dnsc_recv(dns,sockfd,&addr,sizeof(struct sockaddr_in));
	wtk_log_log(glb_log,"recv ret %d",ret);
	if(ret != 0) {
		goto end;
	}

	ret = 0;
end:
	if(sockfd > 0) {
#ifdef WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
	}
	return ret;
}

wtk_string_t qtk_dnsc_process(qtk_dnsc_t *dns,char *domain,int len)
{
	wtk_queue_node_t *qn;
	qtk_dnsc_item_t *item;
	wtk_string_t v;
	int ret = -1;
	int i,n;

	wtk_string_set(&v,0,0);

	n = sizeof(qtk_dns_default_svr) / sizeof(wtk_string_t);
	for(i=0;i<n;++i) {
		wtk_log_log(glb_log,"dns request %.*s",qtk_dns_default_svr[i].len,qtk_dns_default_svr[i].data);
		ret = qtk_dnsc_process_svr(dns,domain,len,qtk_dns_default_svr[i].data,qtk_dns_default_svr[i].len);
		wtk_log_log(glb_log,"dns request ret %d",ret);
		if(ret == 0) {
			break;
		}
	}

	if(ret == 0) {
		for(qn=dns->rlt_q.pop;qn;qn=qn->next) {
			item = data_offset2(qn,qtk_dnsc_item_t,q_n);
			if(item->type == QTK_DNS_IPV4) {
				wtk_string_set(&v,item->rlt->data,item->rlt->len);
				break;
			}
		}
	}

	//qtk_dnsc_print(dns);

	return v;
}

void qtk_dnsc_reset(qtk_dnsc_t *dns)
{
	wtk_queue_node_t *qn;
	qtk_dnsc_item_t *item;

	while(1) {
		qn = wtk_queue_pop(&(dns->rlt_q));
		if(!qn) {
			break;
		}
		item = data_offset2(qn,qtk_dnsc_item_t,q_n);
		qtk_dnsc_item_delete(item);
	}
}

void qtk_dnsc_print(qtk_dnsc_t *dns)
{
	wtk_queue_node_t *qn;
	qtk_dnsc_item_t *item;

	wtk_debug("======================================= dns ===========\n");
	for(qn=dns->rlt_q.pop;qn;qn=qn->next) {
		item = data_offset2(qn,qtk_dnsc_item_t,q_n);
		wtk_debug("type = %d\n",item->type);
		wtk_debug("ttl = %ld\n",item->ttl);
		wtk_debug("name = %.*s\n",item->name->len,item->name->data);
		wtk_debug("rlt = %.*s\n",item->rlt->len,item->rlt->data);
		printf("\n\n");
	}
	wtk_debug("======================================= dns ===========\n");
}


