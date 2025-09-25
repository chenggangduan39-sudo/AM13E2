/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: *unnormalized* fft transform
 last mod: $Id: smallft.c,v 1.19 2003/10/08 05:12:37 jm Exp $

 ********************************************************************/

/* FFT implementation from OggSquish, minus cosine transforms,
 * minus all but radix 2/4 case.  In Vorbis we only need this
 * cut-down version.
 *
 * To do more than just power-of-two sized vectors, see the full
 * version I wrote for NetLib.
 *
 * Note that the packing is a little strange; rather than the FFT r/i
 * packing following R_0, I_n, R_1, I_1, R_2, I_2 ... R_n-1, I_n-1,
 * it follows R_0, R_1, I_1, R_2, I_2 ... R_n-1, I_n-1, I_n like the
 * FORTRAN version
 */

#include <math.h>
#include <stdlib.h>
#include "wtk_drft.h"
#include "qtk/math/qtk_vector.h"

static void drfti1(int n, float *wa, int *ifac){
	static int ntryh[4] = { 4,2,3,5 };
	static float tpi = 6.28318530717958648f;
	float arg,argh,argld,fi;
	int ntry=0,i,j=-1;
	int k1, l1, l2, ib;
	int ld, ii, ip, is, nq, nr;
	int ido, ipm, nfm1;
	int nl=n;
	int nf=0;

 L101:
	j++;
	if (j < 4)
		ntry=ntryh[j];
	else
		ntry+=2;

 L104:
	nq=nl/ntry;
	nr=nl-ntry*nq;
	if (nr!=0) goto L101;

	nf++;
	ifac[nf+1]=ntry;
	nl=nq;
	if(ntry!=2)goto L107;
	if(nf==1)goto L107;

	for (i=1;i<nf;i++){
		ib=nf-i+1;
		ifac[ib+1]=ifac[ib];
	}
	ifac[2] = 2;

 L107:
	if(nl!=1)goto L104;
	ifac[0]=n;
	ifac[1]=nf;
	argh=tpi/n;
	is=0;
	nfm1=nf-1;
	l1=1;

	if(nfm1==0)return;

	for (k1=0;k1<nfm1;k1++){
		ip=ifac[k1+2];
		ld=0;
		l2=l1*ip;
		ido=n/l2;
		ipm=ip-1;

		for (j=0;j<ipm;j++){
			ld+=l1;
			i=is;
			argld=(float)ld*argh;
			fi=0.f;
			for (ii=2;ii<ido;ii+=2){
				fi+=1.f;
				arg=fi*argld;
				wa[i++]=cos(arg);
				wa[i++]=sin(arg);
			}
			is+=ido;
		}
		l1=l2;
	}
}

static void fdrffti(int n, float *wsave, int *ifac){

	if (n == 1) return;
	drfti1(n, wsave+n, ifac);
}

static void dradf2(int ido,int l1,float *cc,float *ch,float *wa1){
	int i,k;
	float ti2,tr2;
	int t0,t1,t2,t3,t4,t5,t6;

	t1=0;
	t0=(t2=l1*ido);
	t3=ido<<1;
	for(k=0;k<l1;k++){
		ch[t1<<1]=cc[t1]+cc[t2];
		ch[(t1<<1)+t3-1]=cc[t1]-cc[t2];
		t1+=ido;
		t2+=ido;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	t2=t0;
	for(k=0;k<l1;k++){
		t3=t2;
		t4=(t1<<1)+(ido<<1);
		t5=t1;
		t6=t1+t1;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4-=2;
			t5+=2;
			t6+=2;
			tr2=wa1[i-2]*cc[t3-1]+wa1[i-1]*cc[t3];
			ti2=wa1[i-2]*cc[t3]-wa1[i-1]*cc[t3-1];
			ch[t6]=cc[t5]+ti2;
			ch[t4]=ti2-cc[t5];
			ch[t6-1]=cc[t5-1]+tr2;
			ch[t4-1]=cc[t5-1]-tr2;
		}
		t1+=ido;
		t2+=ido;
	}

	if(ido%2==1)return;

 L105:
	t3=(t2=(t1=ido)-1);
	t2+=t0;
	for(k=0;k<l1;k++){
		ch[t1]=-cc[t2];
		ch[t1-1]=cc[t3];
		t1+=ido<<1;
		t2+=ido;
		t3+=ido;
	}
}

static void dradf4(int ido,int l1,float *cc,float *ch,float *wa1,
	    float *wa2,float *wa3){
	static float hsqt2 = .70710678118654752f;
	int i,k,t0,t1,t2,t3,t4,t5,t6;
	float ci2,ci3,ci4,cr2,cr3,cr4,ti1,ti2,ti3,ti4,tr1,tr2,tr3,tr4;
	t0=l1*ido;

	t1=t0;
	t4=t1<<1;
	t2=t1+(t1<<1);
	t3=0;

	for(k=0;k<l1;k++){
		tr1=cc[t1]+cc[t2];
		tr2=cc[t3]+cc[t4];

		ch[t5=t3<<2]=tr1+tr2;
		ch[(ido<<2)+t5-1]=tr2-tr1;
		ch[(t5+=(ido<<1))-1]=cc[t3]-cc[t4];
		ch[t5]=cc[t2]-cc[t1];

		t1+=ido;
		t2+=ido;
		t3+=ido;
		t4+=ido;
	}

	if(ido<2)return;
	if(ido==2)goto L105;


	t1=0;
	for(k=0;k<l1;k++){
		t2=t1;
		t4=t1<<2;
		t5=(t6=ido<<1)+t4;
		for(i=2;i<ido;i+=2){
			t3=(t2+=2);
			t4+=2;
			t5-=2;

			t3+=t0;
			cr2=wa1[i-2]*cc[t3-1]+wa1[i-1]*cc[t3];
			ci2=wa1[i-2]*cc[t3]-wa1[i-1]*cc[t3-1];
			t3+=t0;
			cr3=wa2[i-2]*cc[t3-1]+wa2[i-1]*cc[t3];
			ci3=wa2[i-2]*cc[t3]-wa2[i-1]*cc[t3-1];
			t3+=t0;
			cr4=wa3[i-2]*cc[t3-1]+wa3[i-1]*cc[t3];
			ci4=wa3[i-2]*cc[t3]-wa3[i-1]*cc[t3-1];

			tr1=cr2+cr4;
			tr4=cr4-cr2;
			ti1=ci2+ci4;
			ti4=ci2-ci4;

			ti2=cc[t2]+ci3;
			ti3=cc[t2]-ci3;
			tr2=cc[t2-1]+cr3;
			tr3=cc[t2-1]-cr3;

			ch[t4-1]=tr1+tr2;
			ch[t4]=ti1+ti2;

			ch[t5-1]=tr3-ti4;
			ch[t5]=tr4-ti3;

			ch[t4+t6-1]=ti4+tr3;
			ch[t4+t6]=tr4+ti3;

			ch[t5+t6-1]=tr2-tr1;
			ch[t5+t6]=ti1-ti2;
		}
		t1+=ido;
	}
	if(ido&1)return;

 L105:

	t2=(t1=t0+ido-1)+(t0<<1);
	t3=ido<<2;
	t4=ido;
	t5=ido<<1;
	t6=ido;

	for(k=0;k<l1;k++){
		ti1=-hsqt2*(cc[t1]+cc[t2]);
		tr1=hsqt2*(cc[t1]-cc[t2]);

		ch[t4-1]=tr1+cc[t6-1];
		ch[t4+t5-1]=cc[t6-1]-tr1;

		ch[t4]=ti1-cc[t1+t0];
		ch[t4+t5]=ti1+cc[t1+t0];

		t1+=ido;
		t2+=ido;
		t4+=t3;
		t6+=ido;
	}
}

static void dradfg(int ido,int ip,int l1,int idl1,float *cc,float *c1,
                          float *c2,float *ch,float *ch2,float *wa){

	static float tpi=6.283185307179586f;
	int idij,ipph,i,j,k,l,ic,ik,is;
	int t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10;
	float dc2,ai1,ai2,ar1,ar2,ds2;
	int nbd;
	float dcp,arg,dsp,ar1h,ar2h;
	int idp2,ipp2;

	arg=tpi/(float)ip;
	dcp=cos(arg);
	dsp=sin(arg);
	ipph=(ip+1)>>1;
	ipp2=ip;
	idp2=ido;
	nbd=(ido-1)>>1;
	t0=l1*ido;
	t10=ip*ido;

	if(ido==1)goto L119;
	for(ik=0;ik<idl1;ik++)ch2[ik]=c2[ik];

	t1=0;
	for(j=1;j<ip;j++){
		t1+=t0;
		t2=t1;
		for(k=0;k<l1;k++){
			ch[t2]=c1[t2];
			t2+=ido;
		}
	}

	is=-ido;
	t1=0;
	if(nbd>l1){
		for(j=1;j<ip;j++){
			t1+=t0;
			is+=ido;
			t2= -ido+t1;
			for(k=0;k<l1;k++){
				idij=is-1;
				t2+=ido;
				t3=t2;
				for(i=2;i<ido;i+=2){
					idij+=2;
					t3+=2;
					ch[t3-1]=wa[idij-1]*c1[t3-1]+wa[idij]*c1[t3];
					ch[t3]=wa[idij-1]*c1[t3]-wa[idij]*c1[t3-1];
				}
			}
		}
	}else{

		for(j=1;j<ip;j++){
			is+=ido;
			idij=is-1;
			t1+=t0;
			t2=t1;
			for(i=2;i<ido;i+=2){
				idij+=2;
				t2+=2;
				t3=t2;
				for(k=0;k<l1;k++){
					ch[t3-1]=wa[idij-1]*c1[t3-1]+wa[idij]*c1[t3];
					ch[t3]=wa[idij-1]*c1[t3]-wa[idij]*c1[t3-1];
					t3+=ido;
				}
			}
		}
	}

	t1=0;
	t2=ipp2*t0;
	if(nbd<l1){
		for(j=1;j<ipph;j++){
			t1+=t0;
			t2-=t0;
			t3=t1;
			t4=t2;
			for(i=2;i<ido;i+=2){
				t3+=2;
				t4+=2;
				t5=t3-ido;
				t6=t4-ido;
				for(k=0;k<l1;k++){
					t5+=ido;
					t6+=ido;
					c1[t5-1]=ch[t5-1]+ch[t6-1];
					c1[t6-1]=ch[t5]-ch[t6];
					c1[t5]=ch[t5]+ch[t6];
					c1[t6]=ch[t6-1]-ch[t5-1];
				}
			}
		}
	}else{
		for(j=1;j<ipph;j++){
			t1+=t0;
			t2-=t0;
			t3=t1;
			t4=t2;
			for(k=0;k<l1;k++){
				t5=t3;
				t6=t4;
				for(i=2;i<ido;i+=2){
					t5+=2;
					t6+=2;
					c1[t5-1]=ch[t5-1]+ch[t6-1];
					c1[t6-1]=ch[t5]-ch[t6];
					c1[t5]=ch[t5]+ch[t6];
					c1[t6]=ch[t6-1]-ch[t5-1];
				}
				t3+=ido;
				t4+=ido;
			}
		}
	}

L119:
	for(ik=0;ik<idl1;ik++)c2[ik]=ch2[ik];

	t1=0;
	t2=ipp2*idl1;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1-ido;
		t4=t2-ido;
		for(k=0;k<l1;k++){
			t3+=ido;
			t4+=ido;
			c1[t3]=ch[t3]+ch[t4];
			c1[t4]=ch[t4]-ch[t3];
		}
	}

	ar1=1.f;
	ai1=0.f;
	t1=0;
	t2=ipp2*idl1;
	t3=(ip-1)*idl1;
	for(l=1;l<ipph;l++){
		t1+=idl1;
		t2-=idl1;
		ar1h=dcp*ar1-dsp*ai1;
		ai1=dcp*ai1+dsp*ar1;
		ar1=ar1h;
		t4=t1;
		t5=t2;
		t6=t3;
		t7=idl1;

		for(ik=0;ik<idl1;ik++){
			ch2[t4++]=c2[ik]+ar1*c2[t7++];
			ch2[t5++]=ai1*c2[t6++];
		}

		dc2=ar1;
		ds2=ai1;
		ar2=ar1;
		ai2=ai1;

		t4=idl1;
		t5=(ipp2-1)*idl1;
		for(j=2;j<ipph;j++){
			t4+=idl1;
			t5-=idl1;

			ar2h=dc2*ar2-ds2*ai2;
			ai2=dc2*ai2+ds2*ar2;
			ar2=ar2h;

			t6=t1;
			t7=t2;
			t8=t4;
			t9=t5;
			for(ik=0;ik<idl1;ik++){
				ch2[t6++]+=ar2*c2[t8++];
				ch2[t7++]+=ai2*c2[t9++];
			}
		}
	}

	t1=0;
	for(j=1;j<ipph;j++){
		t1+=idl1;
		t2=t1;
		for(ik=0;ik<idl1;ik++)ch2[ik]+=c2[t2++];
	}

	if(ido<l1)goto L132;

	t1=0;
	t2=0;
	for(k=0;k<l1;k++){
		t3=t1;
		t4=t2;
		for(i=0;i<ido;i++)cc[t4++]=ch[t3++];
		t1+=ido;
		t2+=t10;
	}

	goto L135;

 L132:
	for(i=0;i<ido;i++){
		t1=i;
		t2=i;
		for(k=0;k<l1;k++){
			cc[t2]=ch[t1];
			t1+=ido;
			t2+=t10;
		}
	}

 L135:
	t1=0;
	t2=ido<<1;
	t3=0;
	t4=ipp2*t0;
	for(j=1;j<ipph;j++){

		t1+=t2;
		t3+=t0;
		t4-=t0;

		t5=t1;
		t6=t3;
		t7=t4;

		for(k=0;k<l1;k++){
			cc[t5-1]=ch[t6];
			cc[t5]=ch[t7];
			t5+=t10;
			t6+=ido;
			t7+=ido;
		}
	}

	if(ido==1)return;
	if(nbd<l1)goto L141;

	t1=-ido;
	t3=0;
	t4=0;
	t5=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t2;
		t3+=t2;
		t4+=t0;
		t5-=t0;
		t6=t1;
		t7=t3;
		t8=t4;
		t9=t5;
		for(k=0;k<l1;k++){
			for(i=2;i<ido;i+=2){
				ic=idp2-i;
				cc[i+t7-1]=ch[i+t8-1]+ch[i+t9-1];
				cc[ic+t6-1]=ch[i+t8-1]-ch[i+t9-1];
				cc[i+t7]=ch[i+t8]+ch[i+t9];
				cc[ic+t6]=ch[i+t9]-ch[i+t8];
			}
			t6+=t10;
			t7+=t10;
			t8+=ido;
			t9+=ido;
		}
	}
	return;

 L141:

	t1=-ido;
	t3=0;
	t4=0;
	t5=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t2;
		t3+=t2;
		t4+=t0;
		t5-=t0;
		for(i=2;i<ido;i+=2){
			t6=idp2+t1-i;
			t7=i+t3;
			t8=i+t4;
			t9=i+t5;
			for(k=0;k<l1;k++){
				cc[t7-1]=ch[t8-1]+ch[t9-1];
				cc[t6-1]=ch[t8-1]-ch[t9-1];
				cc[t7]=ch[t8]+ch[t9];
				cc[t6]=ch[t9]-ch[t8];
				t6+=t10;
				t7+=t10;
				t8+=ido;
				t9+=ido;
			}
		}
	}
}

static void drftf1(int n,float *c,float *ch,float *wa,int *ifac){
	int i,k1,l1,l2;
	int na,kh,nf;
	int ip,iw,ido,idl1,ix2,ix3;

	nf=ifac[1];
	na=1;
	l2=n;
	iw=n;

	for(k1=0;k1<nf;k1++){
		kh=nf-k1;
		ip=ifac[kh+1];
		l1=l2/ip;
		ido=n/l2;
		idl1=ido*l1;
		iw-=(ip-1)*ido;
		na=1-na;

		if(ip!=4)goto L102;

		ix2=iw+ido;
		ix3=ix2+ido;
		if(na!=0)
			dradf4(ido,l1,ch,c,wa+iw-1,wa+ix2-1,wa+ix3-1);
		else
			dradf4(ido,l1,c,ch,wa+iw-1,wa+ix2-1,wa+ix3-1);
		goto L110;

 L102:
		if(ip!=2)goto L104;
		if(na!=0)goto L103;

		dradf2(ido,l1,c,ch,wa+iw-1);
		goto L110;

	L103:
		dradf2(ido,l1,ch,c,wa+iw-1);
		goto L110;

	L104:
		if(ido==1)na=1-na;
		if(na!=0)goto L109;

		dradfg(ido,ip,l1,idl1,c,c,c,ch,ch,wa+iw-1);
		na=1;
		goto L110;

	L109:
		dradfg(ido,ip,l1,idl1,ch,ch,ch,c,c,wa+iw-1);
		na=0;

	L110:
		l2=l1;
	}

	if(na==1)return;

	for(i=0;i<n;i++)c[i]=ch[i];
}

static void dradb2(int ido,int l1,float *cc,float *ch,float *wa1){
	int i,k,t0,t1,t2,t3,t4,t5,t6;
	float ti2,tr2;

	t0=l1*ido;

	t1=0;
	t2=0;
	t3=(ido<<1)-1;
	for(k=0;k<l1;k++){
		ch[t1]=cc[t2]+cc[t3+t2];
		ch[t1+t0]=cc[t2]-cc[t3+t2];
		t2=(t1+=ido)<<1;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	t2=0;
	for(k=0;k<l1;k++){
		t3=t1;
		t5=(t4=t2)+(ido<<1);
		t6=t0+t1;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4+=2;
			t5-=2;
			t6+=2;
			ch[t3-1]=cc[t4-1]+cc[t5-1];
			tr2=cc[t4-1]-cc[t5-1];
			ch[t3]=cc[t4]-cc[t5];
			ti2=cc[t4]+cc[t5];
			ch[t6-1]=wa1[i-2]*tr2-wa1[i-1]*ti2;
			ch[t6]=wa1[i-2]*ti2+wa1[i-1]*tr2;
		}
		t2=(t1+=ido)<<1;
	}

	if(ido%2==1)return;

L105:
	t1=ido-1;
	t2=ido-1;
	for(k=0;k<l1;k++){
		ch[t1]=cc[t2]+cc[t2];
		ch[t1+t0]=-(cc[t2+1]+cc[t2+1]);
		t1+=ido;
		t2+=ido<<1;
	}
}

static void dradb3(int ido,int l1,float *cc,float *ch,float *wa1,
                          float *wa2){
	static float taur = -.5f;
	static float taui = .8660254037844386f;
	int i,k,t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10;
	float ci2,ci3,di2,di3,cr2,cr3,dr2,dr3,ti2,tr2;
	t0=l1*ido;

	t1=0;
	t2=t0<<1;
	t3=ido<<1;
	t4=ido+(ido<<1);
	t5=0;
	for(k=0;k<l1;k++){
		tr2=cc[t3-1]+cc[t3-1];
		cr2=cc[t5]+(taur*tr2);
		ch[t1]=cc[t5]+tr2;
		ci3=taui*(cc[t3]+cc[t3]);
		ch[t1+t0]=cr2-ci3;
		ch[t1+t2]=cr2+ci3;
		t1+=ido;
		t3+=t4;
		t5+=t4;
	}

	if(ido==1)return;

	t1=0;
	t3=ido<<1;
	for(k=0;k<l1;k++){
		t7=t1+(t1<<1);
		t6=(t5=t7+t3);
		t8=t1;
		t10=(t9=t1+t0)+t0;

		for(i=2;i<ido;i+=2){
			t5+=2;
			t6-=2;
			t7+=2;
			t8+=2;
			t9+=2;
			t10+=2;
			tr2=cc[t5-1]+cc[t6-1];
			cr2=cc[t7-1]+(taur*tr2);
			ch[t8-1]=cc[t7-1]+tr2;
			ti2=cc[t5]-cc[t6];
			ci2=cc[t7]+(taur*ti2);
			ch[t8]=cc[t7]+ti2;
			cr3=taui*(cc[t5-1]-cc[t6-1]);
			ci3=taui*(cc[t5]+cc[t6]);
			dr2=cr2-ci3;
			dr3=cr2+ci3;
			di2=ci2+cr3;
			di3=ci2-cr3;
			ch[t9-1]=wa1[i-2]*dr2-wa1[i-1]*di2;
			ch[t9]=wa1[i-2]*di2+wa1[i-1]*dr2;
			ch[t10-1]=wa2[i-2]*dr3-wa2[i-1]*di3;
			ch[t10]=wa2[i-2]*di3+wa2[i-1]*dr3;
		}
		t1+=ido;
	}
}

static void dradb4(int ido,int l1,float *cc,float *ch,float *wa1,
			  float *wa2,float *wa3){
	static float sqrt2=1.414213562373095f;
	int i,k,t0,t1,t2,t3,t4,t5,t6,t7,t8;
	float ci2,ci3,ci4,cr2,cr3,cr4,ti1,ti2,ti3,ti4,tr1,tr2,tr3,tr4;
	t0=l1*ido;

	t1=0;
	t2=ido<<2;
	t3=0;
	t6=ido<<1;
	for(k=0;k<l1;k++){
		t4=t3+t6;
		t5=t1;
		tr3=cc[t4-1]+cc[t4-1];
		tr4=cc[t4]+cc[t4];
		tr1=cc[t3]-cc[(t4+=t6)-1];
		tr2=cc[t3]+cc[t4-1];
		ch[t5]=tr2+tr3;
		ch[t5+=t0]=tr1-tr4;
		ch[t5+=t0]=tr2-tr3;
		ch[t5+=t0]=tr1+tr4;
		t1+=ido;
		t3+=t2;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	for(k=0;k<l1;k++){
		t5=(t4=(t3=(t2=t1<<2)+t6))+t6;
		t7=t1;
		for(i=2;i<ido;i+=2){
			t2+=2;
			t3+=2;
			t4-=2;
			t5-=2;
			t7+=2;
			ti1=cc[t2]+cc[t5];
			ti2=cc[t2]-cc[t5];
			ti3=cc[t3]-cc[t4];
			tr4=cc[t3]+cc[t4];
			tr1=cc[t2-1]-cc[t5-1];
			tr2=cc[t2-1]+cc[t5-1];
			ti4=cc[t3-1]-cc[t4-1];
			tr3=cc[t3-1]+cc[t4-1];
			ch[t7-1]=tr2+tr3;
			cr3=tr2-tr3;
			ch[t7]=ti2+ti3;
			ci3=ti2-ti3;
			cr2=tr1-tr4;
			cr4=tr1+tr4;
			ci2=ti1+ti4;
			ci4=ti1-ti4;

			ch[(t8=t7+t0)-1]=wa1[i-2]*cr2-wa1[i-1]*ci2;
			ch[t8]=wa1[i-2]*ci2+wa1[i-1]*cr2;
			ch[(t8+=t0)-1]=wa2[i-2]*cr3-wa2[i-1]*ci3;
			ch[t8]=wa2[i-2]*ci3+wa2[i-1]*cr3;
			ch[(t8+=t0)-1]=wa3[i-2]*cr4-wa3[i-1]*ci4;
			ch[t8]=wa3[i-2]*ci4+wa3[i-1]*cr4;
		}
		t1+=ido;
	}

	if(ido%2 == 1)return;

 L105:

	t1=ido;
	t2=ido<<2;
	t3=ido-1;
	t4=ido+(ido<<1);
	for(k=0;k<l1;k++){
		t5=t3;
		ti1=cc[t1]+cc[t4];
		ti2=cc[t4]-cc[t1];
		tr1=cc[t1-1]-cc[t4-1];
		tr2=cc[t1-1]+cc[t4-1];
		ch[t5]=tr2+tr2;
		ch[t5+=t0]=sqrt2*(tr1-ti1);
		ch[t5+=t0]=ti2+ti2;
		ch[t5+=t0]=-sqrt2*(tr1+ti1);

		t3+=ido;
		t1+=t2;
		t4+=t2;
	}
}

static void dradbg(int ido,int ip,int l1,int idl1,float *cc,float *c1,
	float *c2,float *ch,float *ch2,float *wa){
	static float tpi=6.283185307179586f;
	int idij,ipph,i,j,k,l,ik,is,t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
	float dc2,ai1,ai2,ar1,ar2,ds2;
	int nbd;
	float dcp,arg,dsp,ar1h,ar2h;
	int ipp2;

	t10=ip*ido;
	t0=l1*ido;
	arg=tpi/(float)ip;
	dcp=cos(arg);
	dsp=sin(arg);
	nbd=(ido-1)>>1;
	ipp2=ip;
	ipph=(ip+1)>>1;
	if(ido<l1)goto L103;

	t1=0;
	t2=0;
	for(k=0;k<l1;k++){
		t3=t1;
		t4=t2;
		for(i=0;i<ido;i++){
			ch[t3]=cc[t4];
			t3++;
			t4++;
		}
		t1+=ido;
		t2+=t10;
	}
	goto L106;

 L103:
	t1=0;
	for(i=0;i<ido;i++){
		t2=t1;
		t3=t1;
		for(k=0;k<l1;k++){
			ch[t2]=cc[t3];
			t2+=ido;
			t3+=t10;
		}
		t1++;
	}

 L106:
	t1=0;
	t2=ipp2*t0;
	t7=(t5=ido<<1);
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		t6=t5;
		for(k=0;k<l1;k++){
			ch[t3]=cc[t6-1]+cc[t6-1];
			ch[t4]=cc[t6]+cc[t6];
			t3+=ido;
			t4+=ido;
			t6+=t10;
		}
		t5+=t7;
	}

	if (ido == 1)goto L116;
	if(nbd<l1)goto L112;

	t1=0;
	t2=ipp2*t0;
	t7=0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;

		t7+=(ido<<1);
		t8=t7;
		for(k=0;k<l1;k++){
			t5=t3;
			t6=t4;
			t9=t8;
			t11=t8;
			for(i=2;i<ido;i+=2){
				t5+=2;
				t6+=2;
				t9+=2;
				t11-=2;
				ch[t5-1]=cc[t9-1]+cc[t11-1];
				ch[t6-1]=cc[t9-1]-cc[t11-1];
				ch[t5]=cc[t9]-cc[t11];
				ch[t6]=cc[t9]+cc[t11];
			}
			t3+=ido;
			t4+=ido;
			t8+=t10;
		}
	}
	goto L116;

 L112:
	t1=0;
	t2=ipp2*t0;
	t7=0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		t7+=(ido<<1);
		t8=t7;
		t9=t7;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4+=2;
			t8+=2;
			t9-=2;
			t5=t3;
			t6=t4;
			t11=t8;
			t12=t9;
			for(k=0;k<l1;k++){
				ch[t5-1]=cc[t11-1]+cc[t12-1];
				ch[t6-1]=cc[t11-1]-cc[t12-1];
				ch[t5]=cc[t11]-cc[t12];
				ch[t6]=cc[t11]+cc[t12];
				t5+=ido;
				t6+=ido;
				t11+=t10;
				t12+=t10;
			}
		}
	}

L116:
	ar1=1.f;
	ai1=0.f;
	t1=0;
	t9=(t2=ipp2*idl1);
	t3=(ip-1)*idl1;
	for(l=1;l<ipph;l++){
		t1+=idl1;
		t2-=idl1;

		ar1h=dcp*ar1-dsp*ai1;
		ai1=dcp*ai1+dsp*ar1;
		ar1=ar1h;
		t4=t1;
		t5=t2;
		t6=0;
		t7=idl1;
		t8=t3;
		for(ik=0;ik<idl1;ik++){
			c2[t4++]=ch2[t6++]+ar1*ch2[t7++];
			c2[t5++]=ai1*ch2[t8++];
		}
		dc2=ar1;
		ds2=ai1;
		ar2=ar1;
		ai2=ai1;

		t6=idl1;
		t7=t9-idl1;
		for(j=2;j<ipph;j++){
			t6+=idl1;
			t7-=idl1;
			ar2h=dc2*ar2-ds2*ai2;
			ai2=dc2*ai2+ds2*ar2;
			ar2=ar2h;
			t4=t1;
			t5=t2;
			t11=t6;
			t12=t7;
			for(ik=0;ik<idl1;ik++){
				c2[t4++]+=ar2*ch2[t11++];
				c2[t5++]+=ai2*ch2[t12++];
			}
		}
	}

	t1=0;
	for(j=1;j<ipph;j++){
		t1+=idl1;
		t2=t1;
		for(ik=0;ik<idl1;ik++)ch2[ik]+=ch2[t2++];
	}

	t1=0;
	t2=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		for(k=0;k<l1;k++){
			ch[t3]=c1[t3]-c1[t4];
			ch[t4]=c1[t3]+c1[t4];
			t3+=ido;
			t4+=ido;
		}
	}

	if(ido==1)goto L132;
	if(nbd<l1)goto L128;

	t1=0;
	t2=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		for(k=0;k<l1;k++){
			t5=t3;
			t6=t4;
			for(i=2;i<ido;i+=2){
				t5+=2;
				t6+=2;
				ch[t5-1]=c1[t5-1]-c1[t6];
				ch[t6-1]=c1[t5-1]+c1[t6];
				ch[t5]=c1[t5]+c1[t6-1];
				ch[t6]=c1[t5]-c1[t6-1];
			}
			t3+=ido;
			t4+=ido;
		}
	}
	goto L132;

 L128:
	t1=0;
	t2=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4+=2;
			t5=t3;
			t6=t4;
			for(k=0;k<l1;k++){
				ch[t5-1]=c1[t5-1]-c1[t6];
				ch[t6-1]=c1[t5-1]+c1[t6];
				ch[t5]=c1[t5]+c1[t6-1];
				ch[t6]=c1[t5]-c1[t6-1];
				t5+=ido;
				t6+=ido;
			}
		}
	}

L132:
	if(ido==1)return;

	for(ik=0;ik<idl1;ik++)c2[ik]=ch2[ik];

	t1=0;
	for(j=1;j<ip;j++){
		t2=(t1+=t0);
		for(k=0;k<l1;k++){
			c1[t2]=ch[t2];
			t2+=ido;
		}
	}

	if(nbd>l1)goto L139;

	is= -ido-1;
	t1=0;
	for(j=1;j<ip;j++){
		is+=ido;
		t1+=t0;
		idij=is;
		t2=t1;
		for(i=2;i<ido;i+=2){
			t2+=2;
			idij+=2;
			t3=t2;
			for(k=0;k<l1;k++){
				c1[t3-1]=wa[idij-1]*ch[t3-1]-wa[idij]*ch[t3];
				c1[t3]=wa[idij-1]*ch[t3]+wa[idij]*ch[t3-1];
				t3+=ido;
			}
		}
	}
	return;

 L139:
	is= -ido-1;
	t1=0;
	for(j=1;j<ip;j++){
		is+=ido;
		t1+=t0;
		t2=t1;
		for(k=0;k<l1;k++){
			idij=is;
			t3=t2;
			for(i=2;i<ido;i+=2){
				idij+=2;
				t3+=2;
				c1[t3-1]=wa[idij-1]*ch[t3-1]-wa[idij]*ch[t3];
				c1[t3]=wa[idij-1]*ch[t3]+wa[idij]*ch[t3-1];
			}
			t2+=ido;
		}
	}
}

static void drftb1(int n, float *c, float *ch, float *wa, int *ifac){
	int i,k1,l1,l2;
	int na;
	int nf,ip,iw,ix2,ix3,ido,idl1;

	nf=ifac[1];
	na=0;
	l1=1;
	iw=1;

	for(k1=0;k1<nf;k1++){
		ip=ifac[k1 + 2];
		l2=ip*l1;
		ido=n/l2;
		idl1=ido*l1;
		if(ip!=4)goto L103;
		ix2=iw+ido;
		ix3=ix2+ido;

		if(na!=0)
			dradb4(ido,l1,ch,c,wa+iw-1,wa+ix2-1,wa+ix3-1);
		else
			dradb4(ido,l1,c,ch,wa+iw-1,wa+ix2-1,wa+ix3-1);
		na=1-na;
		goto L115;

	L103:
		if(ip!=2)goto L106;

		if(na!=0)
			dradb2(ido,l1,ch,c,wa+iw-1);
		else
			dradb2(ido,l1,c,ch,wa+iw-1);
		na=1-na;
		goto L115;

	L106:
		if(ip!=3)goto L109;

		ix2=iw+ido;
		if(na!=0)
			dradb3(ido,l1,ch,c,wa+iw-1,wa+ix2-1);
		else
			dradb3(ido,l1,c,ch,wa+iw-1,wa+ix2-1);
		na=1-na;
		goto L115;

	L109:
/*		The radix five case can be translated later..... */
/*		if(ip!=5)goto L112;

		ix2=iw+ido;
		ix3=ix2+ido;
		ix4=ix3+ido;
		if(na!=0)
			dradb5(ido,l1,ch,c,wa+iw-1,wa+ix2-1,wa+ix3-1,wa+ix4-1);
		else
			dradb5(ido,l1,c,ch,wa+iw-1,wa+ix2-1,wa+ix3-1,wa+ix4-1);
		na=1-na;
		goto L115;

	L112:*/
		if(na!=0)
			dradbg(ido,ip,l1,idl1,ch,ch,ch,c,c,wa+iw-1);
		else
			dradbg(ido,ip,l1,idl1,c,c,c,ch,ch,wa+iw-1);
		if(ido==1)na=1-na;

	L115:
		l1=l2;
		iw+=(ip-1)*ido;
	}

	if(na==0)return;

	for(i=0;i<n;i++)c[i]=ch[i];
}


wtk_drft_t* wtk_drft_new(int n)
{
	wtk_drft_t *d;
	int nx;

	d=(wtk_drft_t*)malloc(sizeof(wtk_drft_t));
	d->n=n;
	d->use_fft=0;
	d->trigcache=(float*)calloc(3*n,sizeof(float));
	d->splitcache=(int*)calloc(32,sizeof(int));
	fdrffti(n, d->trigcache, d->splitcache);

	d->xtmp=(float*)calloc(n,sizeof(float));

	d->rfft=NULL;
	nx=wtk_rfft_next_pow(n);
	if(n==pow(2,nx))
	{
		d->use_fft=1;
		d->rfft=wtk_rfft_new(n/2);
	}

	return d;
}

void wtk_drft_delete(wtk_drft_t *d)
{
	free(d->trigcache);
	free(d->splitcache);
	if(d->rfft)
	{
		wtk_rfft_delete(d->rfft);
	}
	free(d->xtmp);
	free(d);
}

void wtk_drft_fft(wtk_drft_t *d,float *data)
{
	if(d->n==1)return;
	drftf1(d->n,data,d->trigcache,d->trigcache+d->n,d->splitcache);
}

void wtk_drft_fft2(wtk_drft_t *d,float *input,wtk_complex_t *out)
{
	float f;
	int i,j;
	int wins=d->n;
	int fsize=wins/2;
	float *xtmp=d->xtmp;

	if(d->use_fft)
	{
		wtk_rfft_process_fft(d->rfft,xtmp,input);
		f=1.0/wins;
		out[0].a=xtmp[0]*f;
		out[0].b=0;
		out[fsize].a=xtmp[fsize]*f;
		out[fsize].b=0;
		for(i=1;i<fsize;++i)
		{
			out[i].a=xtmp[i]*f;
			out[i].b=-xtmp[i+fsize]*f;
		}
	}else
	{
		for(i=0;i<wins;++i)
		{
			xtmp[i]=input[i];
		}
		wtk_drft_fft(d,xtmp);
		f=1.0/wins;
		out[0].a=xtmp[0]*f;
		out[0].b=0;
		out[fsize].a=xtmp[wins-1]*f;
		out[fsize].b=0;
		for(i=1,j=1;i<fsize;++i,++j)
		{
			out[i].a=xtmp[j]*f;
			out[i].b=xtmp[++j]*f;
		}
	}
}

void wtk_drft_fft2_x(wtk_drft_t *d,float *input,wtk_complex_t *out)
{
  float f;
  int i,j;
  int wins=d->n;
  int fsize=wins/2;
  float *xtmp=d->xtmp;

  if(d->use_fft)
  {
    wtk_rfft_process_fft(d->rfft,xtmp,input);
    f=1.0/wins;
    out[0].a=xtmp[0]*f;
    out[0].b=0;
    out[fsize].a=xtmp[fsize]*f;
    out[fsize].b=0;
    for(i=1;i<fsize;++i)
    {
      out[i].a=xtmp[i]*f;
      out[i].b=-xtmp[i+fsize]*f;
    }
  }else
  {
    for(i=0;i<wins;++i)
    {
      xtmp[i]=input[i];
    }
    wtk_drft_fft(d,xtmp);
    f=1.0/wins;
    out[0].a=xtmp[0]*f;
    out[0].b=0;
    if(fsize%2 == 1){
      out[fsize].a=xtmp[wins-1]*f;
      out[fsize].b=0;
      for(i=1,j=1;i<fsize;++i,++j)
      {
        out[i].a=xtmp[j]*f;
        out[i].b=xtmp[++j]*f;
      }
    }else{
      for(i=1,j=1;i<fsize + 1;++i,++j)
      {
        out[i].a=xtmp[j]*f;
        out[i].b=xtmp[++j]*f;
      }
    }
  }
}

void wtk_drft_ifft(wtk_drft_t *d,float *data)
{
	if(d->n==1){return;}
	drftb1(d->n,data,d->trigcache,d->trigcache+d->n,d->splitcache);
}

void wtk_drft_ifft2(wtk_drft_t *d,wtk_complex_t *input,float *output)
{
	float *xtmp=d->xtmp;
	int i,j;
	int wins=d->n;
	int fsize=wins/2;

	if(d->use_fft)
	{
		xtmp[0]=input[0].a;
		xtmp[fsize]=input[fsize].a;
		for(i=1;i<fsize;++i)
		{
			xtmp[i]=input[i].a;
			xtmp[i+fsize]=-input[i].b;
		}
		wtk_rfft_process_ifft(d->rfft,xtmp,output);
		for(i=0;i<wins;++i)
		{
			output[i]*=wins;
		}
	}else
	{
		output[0]=input[0].a;
		output[wins-1]=input[fsize].a;
		for(i=1,j=1;i<fsize;++i,++j)
		{
			output[j]=input[i].a;
			output[++j]=input[i].b;
		}
		wtk_drft_ifft(d,output);
		// for(i=0;i<wins;++i)
		// {
		// 	output[i]*=wins;
		// }
	}
}

void wtk_drft_ifft2_x(wtk_drft_t *d,wtk_complex_t *input,float *output)
{
  float *xtmp=d->xtmp;
  int i,j;
  int wins=d->n;
  int fsize=wins/2;

  if(d->use_fft)
  {
    xtmp[0]=input[0].a;
    xtmp[fsize]=input[fsize].a;
    for(i=1;i<fsize;++i)
    {
      xtmp[i]=input[i].a;
      xtmp[i+fsize]=-input[i].b;
    }
    wtk_rfft_process_ifft(d->rfft,xtmp,output);
    for(i=0;i<wins;++i)
    {
      output[i]*=wins;
    }
  }else
  {
    output[0]=input[0].a;
    output[wins-1]=input[fsize].a;
    if(fsize%2 == 1){
      for(i=1,j=1;i<fsize;++i,++j)
      {
        output[j]=input[i].a;
        output[++j]=input[i].b;
      }
    }else{
      for(i=1,j=1;i<fsize+1;++i,++j)
      {
        output[j]=input[i].a;
        output[++j]=input[i].b;
      }
    }

    wtk_drft_ifft(d,output);
    // for(i=0;i<wins;++i)
    // {
    // 	output[i]*=wins;
    // }
  }
}

void wtk_drft_fft3(wtk_drft_t *d,float *input,wtk_complex_t *out)
{
	float f;
	int i,j;
	int wins=d->n;
	int fsize=wins/2;
	float *xtmp=d->xtmp;

	if(d->use_fft)
	{
		wtk_rfft_process_fft(d->rfft,xtmp,input);
		f=1.0;
		out[0].a=xtmp[0]*f;
		out[0].b=0;
		out[fsize].a=xtmp[fsize]*f;
		out[fsize].b=0;
		for(i=1;i<fsize;++i)
		{
			out[i].a=xtmp[i]*f;
			out[i].b=-xtmp[i+fsize]*f;
		}
	}else
	{
		for(i=0;i<wins;++i)
		{
			xtmp[i]=input[i];
		}
		wtk_drft_fft(d,xtmp);
		f=1.0;
		out[0].a=xtmp[0]*f;
		out[0].b=0;
		out[fsize].a=xtmp[wins-1]*f;
		out[fsize].b=0;
		for(i=1,j=1;i<fsize;++i,++j)
		{
			out[i].a=xtmp[j]*f;
			out[i].b=xtmp[++j]*f;
		}
	}
}

void wtk_drft_ifft3(wtk_drft_t *d,wtk_complex_t *input,float *output)
{
	float *xtmp=d->xtmp;
	int i,j;
	int wins=d->n;
	int fsize=wins/2;

	if(d->use_fft)
	{
		xtmp[0]=input[0].a;
		xtmp[fsize]=input[fsize].a;
		for(i=1;i<fsize;++i)
		{
			xtmp[i]=input[i].a;
			xtmp[i+fsize]=-input[i].b;
		}
		wtk_rfft_process_ifft(d->rfft,xtmp,output);
		// for(i=0;i<wins;++i)
		// {
		// 	output[i]*=wins;
		// }
	}else
	{
		output[0]=input[0].a;
		output[wins-1]=input[fsize].a;
		for(i=1,j=1;i<fsize;++i,++j)
		{
			output[j]=input[i].a;
			output[++j]=input[i].b;
		}
		wtk_drft_ifft(d,output);
		// for(i=0;i<wins;++i)
		// {
		// 	output[i]*=1.0/wins;
		// }
	}
}





void wtk_drft_frame_analysis(wtk_drft_t* rfft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, float *in, int wins, float *window)
{
	int i;
	int fsize=wins/2;

	memmove(rfft_in, analysis_mem, fsize*sizeof(float));
	for(i=0;i<fsize;++i)
	{
		rfft_in[i+fsize]=in[i];
	}
	memcpy(analysis_mem, in, fsize*sizeof(float));
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= window[i];
	}
	wtk_drft_fft2(rfft, rfft_in, fft);
}

void wtk_drft_frame_synthesis(wtk_drft_t* rfft,  float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window)
{
	int i;
	int fsize=wins/2;

	wtk_drft_ifft2(rfft, fft, rfft_in);
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= synthesis_window[i];
	}
	for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
	memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}

void wtk_drft_frame_analysis2(wtk_drft_t* rfft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, short *in, int wins, float *window)
{
	int i;
	int fsize=wins/2;

	memmove(rfft_in, analysis_mem, fsize*sizeof(float));
	for(i=0;i<fsize;++i)
	{
		rfft_in[i+fsize]=in[i];
	}
	memcpy(analysis_mem, rfft_in+fsize, fsize*sizeof(float));
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= window[i];
	}
	wtk_drft_fft2(rfft, rfft_in, fft);
}

void wtk_drft_frame_synthesis2(wtk_drft_t* rfft,  float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, short *out, int wins, float *synthesis_window)
{
	int i;
	int fsize=wins/2;

	wtk_drft_ifft2(rfft, fft, rfft_in);
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= synthesis_window[i];
	}
	for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
	memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}


void wtk_drft_init_synthesis_window(float *synthesis_window, float *analysis_window, int wins)
{
	int frame_size=wins/2;
	int shift, nshift, i, j, n;

	memset(synthesis_window, 0, sizeof(float)*wins);
	shift = wins-frame_size;
	nshift = wins / shift;
	for(i=0;i<shift;++i)
	{
		for(j=0;j<nshift+1;++j)
		{
			n = i+j*shift;
			if(n < wins)
			{
				synthesis_window[i] += analysis_window[n]*analysis_window[n];
			}
		}
	}
	for(i=1;i<nshift;++i)
	{
		for(j=0;j<shift;++j)
		{
			synthesis_window[i*shift+j] = synthesis_window[j];
		}
	}
	for(i=0;i<wins;++i)
	{
		synthesis_window[i]=analysis_window[i]/synthesis_window[i];
	}
}
void wtk_drft_stft(wtk_drft_t *drft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, float *in, int wins, float *window)
{
	int i;
	int fsize=wins/2;
	memmove(rfft_in, analysis_mem, fsize*sizeof(float));
	for(i=0;i<fsize;++i){
		rfft_in[i+fsize]=in[i];
	}
	memcpy(analysis_mem, rfft_in+fsize, fsize*sizeof(float));
	for(i=0;i<wins;++i){
		rfft_in[i] *= window[i];
	}
	wtk_drft_fft3(drft, rfft_in, fft);
}
void wtk_drft_istft(wtk_drft_t *drft, float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window)
{
	int i;
	int fsize=wins/2;

	wtk_drft_ifft3(drft, fft, rfft_in);
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= synthesis_window[i];
	}
	for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
	memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}
wtk_drft_t* wtk_drft_new2(int n)
{
	wtk_drft_t *d;

	d=(wtk_drft_t*)malloc(sizeof(wtk_drft_t));
	d->n=n;
	d->use_fft=0;

	d->trigcache=NULL;
	d->splitcache=NULL;
	d->rfft=NULL;
	d->pffft=pffft_new_setup(n, PFFFT_REAL);
	d->xtmp=(float *)wtk_malloc(sizeof(float)*n*2);

	return d;
}

void wtk_drft_delete2(wtk_drft_t *d)
{
	pffft_destroy_setup(d->pffft);
	free(d->xtmp);
	free(d);
}

void wtk_drft_fft22(wtk_drft_t *d,float *input,wtk_complex_t *out)
{
	float f;
	int i;
	int wins=d->n;
	int fsize=wins/2;
	float *xtmp=d->xtmp;
	float *ytmp=xtmp+wins;

	pffft_transform_ordered(d->pffft, input, xtmp, ytmp, PFFFT_FORWARD);

	f=1.0/wins;
	out[0].a=xtmp[0]*f;
	out[0].b=0;
	out[fsize].a=xtmp[1]*f;
	out[fsize].b=0;
	for(i=1;i<fsize;++i)
	{
		out[i].a=xtmp[i*2]*f;
		out[i].b=xtmp[i*2+1]*f;
	}
}

void wtk_drft_ifft22(wtk_drft_t *d,wtk_complex_t *input,float *output)
{
	int i;
	int wins=d->n;
	int fsize=wins/2;
	float *xtmp=d->xtmp;
	float *ytmp=xtmp+wins;

	xtmp[0]=input[0].a;
	xtmp[1]=input[fsize].a;
	for(i=1;i<fsize;++i)
	{
		xtmp[i*2]=input[i].a;
		xtmp[i*2+1]=input[i].b;
	}
	pffft_transform_ordered(d->pffft, xtmp, output, ytmp, PFFFT_BACKWARD);
}

void wtk_drft_frame_analysis22(wtk_drft_t* rfft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, short *in, int wins, float *window)
{
	int i;
	int fsize=wins/2;

	memmove(rfft_in, analysis_mem, fsize*sizeof(float));
	for(i=0;i<fsize;++i)
	{
		rfft_in[i+fsize]=in[i];
	}
	memcpy(analysis_mem, rfft_in+fsize, fsize*sizeof(float));
	qtk_vector_multipy_elewise(rfft_in, window, rfft_in, wins);
	wtk_drft_fft22(rfft, rfft_in, fft);
}

void wtk_drft_frame_synthesis22(wtk_drft_t* rfft,  float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window)
{
	int i;
	int fsize=wins/2;

	wtk_drft_ifft22(rfft, fft, rfft_in);
	qtk_vector_multipy_elewise(rfft_in, synthesis_window, rfft_in, wins);
	for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
	memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}

void wtk_drft_frame_synthesis3(wtk_drft_t* rfft,  float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window)
{
	int i;
	int fsize=wins/2;

	wtk_drft_ifft22(rfft, fft, rfft_in);
	for (i=0;i<wins;++i)
	{
		rfft_in[i] /= wins;
	}

	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= synthesis_window[i];
	}
	for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
	memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}

void wtk_drft_frame_synthesis4(wtk_drft_t* rfft,  float *rfft_in, wtk_complex_t *fft, float *out, int wins, float *synthesis_window)
{
	int i;
	int fsize=wins/2;

	wtk_drft_ifft22(rfft, fft, rfft_in);
	for (i=0;i<wins;++i)
	{
		rfft_in[i] /= wins;
	}

	for (i=0;i<wins;++i)
	{
		rfft_in[i] /= synthesis_window[i];
	}
	for (i=0;i<fsize;i++) out[i] = rfft_in[i];
}

void wtk_drft_frame_analysis22_float(wtk_drft_t* rfft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, float *in, int wins, float *window)
{
	int i;
	int fsize=wins/2;

	memmove(rfft_in, analysis_mem, fsize*sizeof(float));
  memcpy(rfft_in+fsize, in, fsize*sizeof(float));
	memcpy(analysis_mem, rfft_in+fsize, fsize*sizeof(float));
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= window[i];
	}
	wtk_drft_fft22(rfft, rfft_in, fft);
}

void wtk_drft_fft23(wtk_drft_t *d,float *input,wtk_complex_t *out)
{
	float f;
	int i;
	int wins=d->n;
	int fsize=wins/2;
	float *xtmp=d->xtmp;
	float *ytmp=xtmp+wins;

	pffft_transform_ordered(d->pffft, input, xtmp, ytmp, PFFFT_FORWARD);

	out[0].a=xtmp[0];
	out[0].b=0;
	out[fsize].a=xtmp[1];
	out[fsize].b=0;
	for(i=1;i<fsize;++i)
	{
		out[i].a=xtmp[i*2];
		out[i].b=xtmp[i*2+1];
	}
}

void wtk_drft_ifft23(wtk_drft_t *d,wtk_complex_t *input,float *output)
{
	int i;
	int wins=d->n;
	int fsize=wins/2;
	float *xtmp=d->xtmp;
	float *ytmp=xtmp+wins;
	float f;

	xtmp[0]=input[0].a;
	xtmp[1]=input[fsize].a;
	for(i=1;i<fsize;++i)
	{
		xtmp[i*2]=input[i].a;
		xtmp[i*2+1]=input[i].b;
	}
	pffft_transform_ordered(d->pffft, xtmp, output, ytmp, PFFFT_BACKWARD);
	f = 1.0/wins;
	qtk_vector_scale(output, output, wins, f);
}

void wtk_drft_stft2(wtk_drft_t *drft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, float *in, int wins, float *window)
{
	int fsize=wins/2;
	memmove(rfft_in, analysis_mem, fsize*sizeof(float));
	memcpy(rfft_in+fsize, in, fsize*sizeof(float));
	memcpy(analysis_mem, rfft_in+fsize, fsize*sizeof(float));
	qtk_vector_multipy_elewise(rfft_in, window, rfft_in, wins);
	wtk_drft_fft23(drft, rfft_in, fft);
}
void wtk_drft_istft2(wtk_drft_t *drft, float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window)
{
	int fsize=wins/2;

	wtk_drft_ifft23(drft, fft, rfft_in);
	qtk_vector_multipy_elewise(rfft_in, synthesis_window, rfft_in, wins);
	qtk_vector_add_elewise(rfft_in, synthesis_mem, out, fsize);
	memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}
