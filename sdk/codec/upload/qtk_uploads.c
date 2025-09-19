#include "qtk_uploads.h" 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

qtk_uploads_t* qtk_uploads_new(qtk_uploads_cfg_t *cfg)
{
	qtk_uploads_t *ups;

	ups = (qtk_uploads_t*)wtk_malloc(sizeof(*ups));
	if(!ups) {
		return NULL;
	}
	ups->cfg = cfg;
	wtk_mkdir_p(ups->cfg->dn.data,'/',1);
	return ups;
}

void qtk_uploads_delete(qtk_uploads_t *ups)
{
	wtk_free(ups);
}

static void qtk_uploads_on_chld(int signo)
{
	pid_t pid;
	int stat;

	while((pid = waitpid(-1,&stat,WNOHANG)) > 0);
	return;
}

static void qtk_uploads_run_chld(qtk_uploads_t *ups,int fd)
{
	FILE *fp = NULL;
	char buffer[4096];
	int read;
	char path[256];
	int ret;

	snprintf(path,256,"%.*s/%lf.pcm",ups->cfg->dn.len,ups->cfg->dn.data,time_get_ms());
	wtk_debug("get uploads fn = %s\n",path);
	fp = fopen(path,"wb");
	if(!fp) {
		goto end;
	}

	while(1) {
		read = recv(fd,buffer,4096,0);
		if(read < 0) {
			if(errno == EINTR || errno == 0) {
				continue;
			}
			break;
		} else if (read == 0) {
			break;
		} else {
			ret = fwrite(buffer,1,read,fp);
			if(ret != read) {
				goto end;
			}
		}
	}

end:
	if(fd > 0) {
		close(fd);
	}
	if(fp) {
		fclose(fp);
	}
	exit(0);
}

int qtk_uploads_proc(qtk_uploads_t *ups)
{
	int listen_fd,conn_fd;
	struct sockaddr_in servaddr;
	struct sockaddr_in connaddr;
	socklen_t connlen;
	struct sigaction sa;
	pid_t pid;
	int ret;

	listen_fd = socket(AF_INET,SOCK_STREAM,0);
	if(listen_fd < 0) {
		ret = -1;
		goto end;
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons((unsigned short)atoi(ups->cfg->port.data));
	ret = bind(listen_fd,(struct sockaddr*)&servaddr,(socklen_t)sizeof(servaddr));
	if(ret != 0) {
		goto end;
	}

	ret = listen(listen_fd,20);
	if(ret != 0) {
		goto end;
	}

	/*
	 * SIGCHLD
	 */
	sa.sa_handler = qtk_uploads_on_chld;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	ret = sigaction(SIGCHLD,&sa,NULL);
	if(ret != 0) {
		goto end;
	}


	while(1) {
		conn_fd = accept(listen_fd,(struct sockaddr*)&connaddr,(socklen_t*)&connlen);
		if(conn_fd < 0) {
			if(errno == EINTR || errno == 0) {
				continue;
			}
			break;
		}

		pid = fork();
		if(pid < 0) {
			break;
		} else if( pid == 0) {
			qtk_uploads_run_chld(ups,conn_fd);
		} else {
			close(conn_fd);
		}
	}

	ret = 0;
end:
	return ret;
}
