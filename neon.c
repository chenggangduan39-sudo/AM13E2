#include "wtk/asr/fextra/fnn/qlas/wtk_qlasasm.h"

#ifdef USE_NEON_ASM

int wtk_char_mult_neon(char *p1,char *p2,int col)
{
	int t;
	int *p;
	int cnt;

	//printf("p1=%p/%d p2=%p/%d\n",p1,((long)p1)%16,p2,((long)p2)%16);
	p=&t;
	cnt=0;
	asm volatile(
			"veor q5,q5,q5\n"
			"ldr r0,[%[col]]\n"
			"mov r1,%[p1]\n"
			"mov r2,%[p2]\n"
			"1:\n"
			"subs r0,r0,#32\n"
			"blt 2f\n"
			"vld1.8 {q0},[r1]! \n" //16  q2
			"vld1.8 {q1},[r1]! \n" //16  q2
			"vld1.8 {q2},[r2]! \n" //16  q3
			"vld1.8 {q3},[r2]! \n"//16   q4
			"vmull.s8 q4,d0,d4\n"
			"vmlal.s8 q4,d1,d5\n"
			"vmlal.s8 q4,d2,d6\n"
			"vmlal.s8 q4,d3,d7\n"
			"vpadal.s16  q5,q4\n"
			"b 1b\n"
			"2:\n"
			"add r0,r0,#32\n"
			"3:\n"
			"subs r0,r0,#8\n"
			"blt 4f\n"
			"vld1.16 d0,[r1]!\n"
			"vld1.16 d1,[r2]!\n"
			"vmull.s8 q4,d0,d1\n"
			"vpadal.s16  q5,q4\n"
			"b 3b\n"
			"4:\n"
			"add r0,r0,#8\n"
			"vadd.i32 d4,d10,d11\n"
			"vpaddl.s32 d0,d4\n"  //前8位有效
			"mov r1,%[t]\n"
			"vst1.32 {d0[0]},[r1]\n"
			"mov %[cnt],r0\n"
			:[t]"+r"(p),
			 [cnt]"+r"(cnt)
			:[p1]"r"(p1),
			 [p2]"r"(p2),
			 [col]"r"(&col)
			:"memory","cc","r0","r1","r2","q0","q1","q2","q3","q4","q5"
			);
	//t2=p[0]+p[1];
	//printf("v[%d/%d]=%d\n",cnt,col,t);
	if(cnt>0)
	{
		cnt=col-cnt;
		switch(cnt)
		{
		case 1:
			t+=p1[cnt]*p2[cnt];
			break;
		case 2:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1];
			break;
		case 3:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2];
			break;
		case 4:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3];
			break;
		case 5:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4];
			break;
		case 6:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4]+p1[cnt+5]*p2[cnt+5];
			break;
		case 7:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4]+p1[cnt+5]*p2[cnt+5]+p1[cnt+6]*p2[cnt+6];
			break;
		default:
			for(;cnt<col;++cnt)
			{
				t+=p1[cnt]*p2[cnt];
			}
			break;
		}
	}
	return t;
}

void wtk_qlas_matb_mul_neon32_raw(wtk_vecb_t *a,wtk_matb_t *b,wtk_veci_t *c)
{
	int col=b->col;
	int row=b->row;
	int i,t,cnt;
	char *p1;
	signed char* p21;
	int *p;
	int *pi=c->p;

	p1=a->p;
	p21=b->p;
	p=c->p;
	p=&t;
	for(i=0;i<row;++i)
	{
		t=0;
		cnt=0;
		asm volatile(
				"veor q5,q5,q5\n"
				"ldr r0,[%[col]]\n"
				"mov r1,%[p1]\n"
				"mov r2,%[p2]\n"
				"1:\n"
				"subs r0,r0,#32\n"
				"blt 2f\n"
				"vld1.8 {q0},[r1]! \n" //16  q2
				"vld1.8 {q1},[r1]! \n" //16  q2
				"vld1.8 {q2},[r2]! \n" //16  q3
				"vld1.8 {q3},[r2]! \n"//16   q4
				"vmull.s8 q4,d0,d4\n"
				"vmlal.s8 q4,d1,d5\n"
				"vmlal.s8 q4,d2,d6\n"
				"vmlal.s8 q4,d3,d7\n"
				"vpadal.s16  q5,q4\n"
				"b 1b\n"
				"2:\n"
				"add r0,r0,#32\n"
				"3:\n"
				"subs r0,r0,#8\n"
				"blt 4f\n"
				"vld1.16 d0,[r1]!\n"
				"vld1.16 d1,[r2]!\n"
				"vmull.s8 q4,d0,d1\n"
				"vpadal.s16  q5,q4\n"
				"b 3b\n"
				"4:\n"
				"add r0,r0,#8\n"
				"vadd.i32 d4,d10,d11\n"
				"vpaddl.s32 d0,d4\n"  //前8位有效
				"mov r1,%[t]\n"
				"vst1.32 {d0[0]},[r1]\n"
				"mov %[cnt],r0\n"
				:[t]"+r"(p),
				 [cnt]"+r"(cnt)
				:[p1]"r"(p1),
				 [p2]"r"(p21),
				 [col]"r"(&col)
				:"memory","cc","r0","r1","r2","q0","q1","q2","q3","q4","q5"
				);
		//t2=p[0]+p[1];
		//printf("v[%d/%d]=%d\n",cnt,col,t);
		if(cnt>0)
		{
			cnt=col-cnt;
			switch(cnt)
			{
			case 1:
				t+=p1[cnt]*p21[cnt];
				break;
			case 2:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1];
				break;
			case 3:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2];
				break;
			case 4:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3];
				break;
			case 5:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4];
				break;
			case 6:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4]+p1[cnt+5]*p21[cnt+5];
				break;
			case 7:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4]+p1[cnt+5]*p21[cnt+5]+p1[cnt+6]*p21[cnt+6];
				break;
			default:
				for(;cnt<col;++cnt)
				{
					t+=p1[cnt]*p21[cnt];
				}
				break;
			}
		}
		p21+=col;
		pi[i]=t;
	}
}


void wtk_qlas_matb_mul_neon32(wtk_vecb_t *a,wtk_matb_t *b,wtk_veci_t *c)
{
	int col=b->col;
	int row=b->row;
	int i,t,cnt,row2;
	char *p1;
	signed char* p21,*p22,*p23;
	int *p;
	int xt[3];
	int *pi=c->p;

	p1=a->p;
	p21=b->p;
	p22=p21+col;
	p23=p22+col;
	row2=(row/3)*3;
	p=xt;
	//row2=0;
	for(i=0;i<row2;i+=3)
	{
		cnt=0;
		asm volatile(
				"veor q5,q5,q5\n"
				"veor q6,q6,q6\n"
				"veor q7,q7,q7\n"
				"ldr r0,[%[col]]\n"
				"mov r1,%[p1]\n"
				"mov r2,%[p21]\n"
				"mov r3,%[p22]\n"
				"mov r4,%[p23]\n"
				"1:\n"
				"subs r0,r0,#32\n"
				"blt 2f\n"
				"vld1.8 {q0},[r1]! \n" //16  q2
				"vld1.8 {q1},[r1]! \n" //16  q2

				"vld1.8 {q2},[r2]! \n" //16  q3
				"vld1.8 {q3},[r2]! \n"//16   q4
				"vmull.s8 q4,d0,d4\n"
				"vmlal.s8 q4,d1,d5\n"
				"vmlal.s8 q4,d2,d6\n"
				"vmlal.s8 q4,d3,d7\n"
				"vpadal.s16  q5,q4\n"

				"vld1.8 {q2},[r3]! \n" //16  q3
				"vld1.8 {q3},[r3]! \n"//16   q4
				"vmull.s8 q4,d0,d4\n"
				"vmlal.s8 q4,d1,d5\n"
				"vmlal.s8 q4,d2,d6\n"
				"vmlal.s8 q4,d3,d7\n"
				"vpadal.s16  q6,q4\n"

				"vld1.8 {q2},[r4]! \n" //16  q3
				"vld1.8 {q3},[r4]! \n"//16   q4
				"vmull.s8 q4,d0,d4\n"
				"vmlal.s8 q4,d1,d5\n"
				"vmlal.s8 q4,d2,d6\n"
				"vmlal.s8 q4,d3,d7\n"
				"vpadal.s16  q7,q4\n"

				"b 1b\n"
				"2:\n"
				"add r0,r0,#32\n"
				"3:\n"
				"subs r0,r0,#8\n"
				"blt 4f\n"
				"vld1.16 d0,[r1]!\n"

				"vld1.16 d1,[r2]!\n"
				"vmull.s8 q4,d0,d1\n"
				"vpadal.s16  q5,q4\n"

				"vld1.16 d1,[r3]!\n"
				"vmull.s8 q4,d0,d1\n"
				"vpadal.s16  q6,q4\n"

				"vld1.16 d1,[r4]!\n"
				"vmull.s8 q4,d0,d1\n"
				"vpadal.s16  q7,q4\n"
				"b 3b\n"
				"4:\n"
				"add r0,r0,#8\n"
				"vadd.i32 d4,d10,d11\n"
				"vpaddl.s32 d0,d4\n"  //前8位有效
				"mov r1,%[t]\n"
				"vst1.32 {d0[0]},[r1]\n"

				"vadd.i32 d4,d12,d13\n"
				"vpaddl.s32 d0,d4\n"  //前8位有效
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"

				"vadd.i32 d4,d14,d15\n"
				"vpaddl.s32 d0,d4\n"  //前8位有效
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"

				"mov %[cnt],r0\n"
				:[t]"+r"(p),
				 [cnt]"+r"(cnt)
				:[p1]"r"(p1),
				 [p21]"r"(p21),
				 [p22]"r"(p22),
				 [p23]"r"(p23),
				 [col]"r"(&col)
				:"memory","cc","r0","r1","r2","r3","r4","q0","q1","q2","q3","q4","q5","q6","q7"
				);
		//t2=p[0]+p[1];
		//printf("v[%d/%d]=%d\n",cnt,col,t);
		//printf("v[%d]=%d/%d/%d\n",i,xt[0],xt[1],xt[2]);
		if(cnt>0)
		{
			cnt=col-cnt;
			switch(cnt)
			{
			case 1:
				xt[0]+=p1[cnt]*p21[cnt];
				xt[1]+=p1[cnt]*p22[cnt];
				xt[2]+=p1[cnt]*p23[cnt];
				break;
			case 2:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1];
				break;
			case 3:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p22[cnt+2];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1]+p1[cnt+2]*p23[cnt+2];
				break;
			case 4:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p22[cnt+2]+p1[cnt+3]*p22[cnt+3];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1]+p1[cnt+2]*p23[cnt+2]+p1[cnt+3]*p23[cnt+3];
				break;
			case 5:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p22[cnt+2]+p1[cnt+3]*p22[cnt+3]+p1[cnt+4]*p22[cnt+4];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1]+p1[cnt+2]*p23[cnt+2]+p1[cnt+3]*p23[cnt+3]+p1[cnt+4]*p23[cnt+4];
				break;
			case 6:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4]+p1[cnt+5]*p21[cnt+5];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p22[cnt+2]+p1[cnt+3]*p22[cnt+3]+p1[cnt+4]*p22[cnt+4]+p1[cnt+5]*p22[cnt+5];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1]+p1[cnt+2]*p23[cnt+2]+p1[cnt+3]*p23[cnt+3]+p1[cnt+4]*p23[cnt+4]+p1[cnt+5]*p23[cnt+5];
				break;
			case 7:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4]+p1[cnt+5]*p21[cnt+5]+p1[cnt+6]*p21[cnt+6];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p22[cnt+2]+p1[cnt+3]*p22[cnt+3]+p1[cnt+4]*p22[cnt+4]+p1[cnt+5]*p22[cnt+5]+p1[cnt+6]*p22[cnt+6];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1]+p1[cnt+2]*p23[cnt+2]+p1[cnt+3]*p23[cnt+3]+p1[cnt+4]*p23[cnt+4]+p1[cnt+5]*p23[cnt+5]+p1[cnt+6]*p23[cnt+6];
				break;
			default:
				for(;cnt<col;++cnt)
				{
					xt[0]+=p1[cnt]*p21[cnt];
					xt[1]+=p1[cnt]*p22[cnt];
					xt[2]+=p1[cnt]*p23[cnt];
				}
				break;
			}
		}
		pi[i]=xt[0];
		pi[i+1]=xt[1];
		pi[i+2]=xt[2];
		p21=p23+col;
		p22=p21+col;
		p23=p22+col;
	}
	//p21=p23;
	p21=b->p+i*col;
	p=&t;
	for(;i<row;++i)
	{
		t=0;
		cnt=0;
		asm volatile(
				"veor q5,q5,q5\n"
				"ldr r0,[%[col]]\n"
				"mov r1,%[p1]\n"
				"mov r2,%[p2]\n"
				"1:\n"
				"subs r0,r0,#32\n"
				"blt 2f\n"
				"vld1.8 {q0},[r1]! \n" //16  q2
				"vld1.8 {q1},[r1]! \n" //16  q2
				"vld1.8 {q2},[r2]! \n" //16  q3
				"vld1.8 {q3},[r2]! \n"//16   q4
				"vmull.s8 q4,d0,d4\n"
				"vmlal.s8 q4,d1,d5\n"
				"vmlal.s8 q4,d2,d6\n"
				"vmlal.s8 q4,d3,d7\n"
				"vpadal.s16  q5,q4\n"
				"b 1b\n"
				"2:\n"
				"add r0,r0,#32\n"
				"3:\n"
				"subs r0,r0,#8\n"
				"blt 4f\n"
				"vld1.16 d0,[r1]!\n"
				"vld1.16 d1,[r2]!\n"
				"vmull.s8 q4,d0,d1\n"
				"vpadal.s16  q5,q4\n"
				"b 3b\n"
				"4:\n"
				"add r0,r0,#8\n"
				"vadd.i32 d4,d10,d11\n"
				"vpaddl.s32 d0,d4\n"  //前8位有效
				"mov r1,%[t]\n"
				"vst1.32 {d0[0]},[r1]\n"
				"mov %[cnt],r0\n"
				:[t]"+r"(p),
				 [cnt]"+r"(cnt)
				:[p1]"r"(p1),
				 [p2]"r"(p21),
				 [col]"r"(&col)
				:"memory","cc","r0","r1","r2","q0","q1","q2","q3","q4","q5"
				);
		//t2=p[0]+p[1];
		//printf("v[%d/%d]=%d\n",cnt,col,t);
		if(cnt>0)
		{
			cnt=col-cnt;
			switch(cnt)
			{
			case 1:
				t+=p1[cnt]*p21[cnt];
				break;
			case 2:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1];
				break;
			case 3:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2];
				break;
			case 4:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3];
				break;
			case 5:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4];
				break;
			case 6:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4]+p1[cnt+5]*p21[cnt+5];
				break;
			case 7:
				t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2]+p1[cnt+3]*p21[cnt+3]+p1[cnt+4]*p21[cnt+4]+p1[cnt+5]*p21[cnt+5]+p1[cnt+6]*p21[cnt+6];
				break;
			default:
				for(;cnt<col;++cnt)
				{
					t+=p1[cnt]*p21[cnt];
				}
				break;
			}
		}
		p21+=col;
		pi[i]=t;
	}
}

void wtk_qlas_mat_cache_doverflow(wtk_vecs_t *a1,wtk_mats_t *b,wtk_veci_t *c)
{
	int row=b->row;
	int col=b->col;
	int i,cnt;
	int *p;
	short *p1=b->p;
	short *p21=a1->p;
	short *p22=p21+col;
	short *p23=p22+col;
	//int row2;
	int xt[3];
	int *pi,*pi1,*pi2;

//	wtk_veci_t *v1;
//	wtk_veci_t *v2;
//	v1=wtk_veci_new(20);
//	v2=wtk_veci_new(20);

	p=xt;
	//row2=(row>>2)<<2;
	//row2=(row/3)*3;
	pi=c->p;
	pi1=pi+row;
	pi2=pi1+row;
	for(i=0;i<row;)
	{
		cnt=0;
		asm volatile(
				"veor q4,q4,q4\n"
				"veor q5,q5,q5\n"
				"veor q6,q6,q6\n"
				"ldr r0,[%[col]]\n"
				"mov r1,%[p1]\n"
				"mov r2,%[p21]\n"
				"mov r3,%[p22]\n"
				"mov r4,%[p23]\n"
				"1:\n"
				"subs r0,r0,#16\n"
				"blt 2f\n"
				//"vld1.16 {q0},[r1]! \n" //8  q2
				//"vld1.16 {q1},[r1]! \n" //8  q3
				"vld1.16 {d0-d3},[r1]!\n"

				//"vld1.16 {q2},[r2]! \n"//8   q4
				//"vld1.16 {q3},[r2]! \n"//8   q5
				"vld1.16 {d4-d7},[r2]!\n"
				//"vmla.i16 q4,q0,q2\n"   //16 q5
				//"vmla.i16 q4,q1,q3\n"  //17 q6
				"vmlal.s16 q4,d0,d4\n"
				"vmlal.s16 q4,d1,d5\n"
				"vmlal.s16 q4,d2,d6\n"
				"vmlal.s16 q4,d3,d7\n"

				//"vld1.16 {q2},[r3]! \n"//8   q4
				//"vld1.16 {q3},[r3]! \n"//8   q5
				"vld1.16 {d4-d7},[r3]!\n"
				//"vmla.i16 q5,q0,q2\n"   //16 q5
				//"vmla.i16 q5,q1,q3\n"  //17 q6
				"vmlal.s16 q5,d0,d4\n"
				"vmlal.s16 q5,d1,d5\n"
				"vmlal.s16 q5,d2,d6\n"
				"vmlal.s16 q5,d3,d7\n"

				//"vld1.16 {q2},[r4]! \n"//8   q4
				//"vld1.16 {q3},[r4]! \n"//8   q5
				"vld1.16 {d4-d7},[r4]!\n"
				//"vmla.i16 q6,q0,q2\n"   //16 q5
				//"vmla.i16 q6,q1,q3\n"  //17 q6
				"vmlal.s16 q6,d0,d4\n"
				"vmlal.s16 q6,d1,d5\n"
				"vmlal.s16 q6,d2,d6\n"
				"vmlal.s16 q6,d3,d7\n"

				"b 1b\n"
				"2:\n"
				"add r0,r0,#16\n"
				"3:\n"
				"subs r0,r0,#4\n"
				"blt 4f\n"
				"vld1.16 d0,[r1]!\n"

				"vld1.16 d1,[r2]!\n"
				//"vmla.i16 d8,d0,d1\n"
				"vmlal.s16 q4,d0,d1\n"

				"vld1.16 d1,[r3]!\n"
				//"vmla.i16 d10,d0,d1\n"
				"vmlal.s16 q5,d0,d1\n"

				"vld1.16 d1,[r4]!\n"
				//"vmla.i16 d12,d0,d1\n"
				"vmlal.s16 q6,d0,d1\n"

				"b 3b\n"
				"4:\n"
				"add r0,r0,#4\n"

				//"vpaddl.s16 q4,q4\n"
				"vpaddl.s32 q4,q4\n"
				"vadd.i32 d8,d8,d9\n"
				"vpaddl.s32  d0,d8\n"
				"mov r1,%[t]\n"
				"vst1.32 {d0[0]},[r1]\n"

				//"vpaddl.s16 q5,q5\n"
				"vpaddl.s32 q5,q5\n"
				"vadd.i32 d8,d10,d11\n"
				"vpaddl.s32  d0,d8\n"
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"

				//"vpaddl.s16 q6,q6\n"
				"vpaddl.s32 q6,q6\n"
				"vadd.i32 d8,d12,d13\n"
				"vpaddl.s32  d0,d8\n"
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"

				"mov %[cnt],r0\n"
				:[t]"+r"(p),
				 [cnt]"+r"(cnt)
				:[p1]"r"(p1),
				 [p21]"r"(p21),
				 [p22]"r"(p22),
				 [p23]"r"(p23),
				 [col]"r"(&col)
				:"memory","cc","r0","r1","r2","r3","r4","q0","q1","q2","q3","q4","q5","q6"
				);
		if(cnt>0)
		{
			cnt=col-cnt;
			switch(cnt)
			{
			case 1:
				xt[0]+=p1[cnt]*p21[cnt];
				xt[1]+=p1[cnt]*p22[cnt];
				xt[2]+=p1[cnt]*p23[cnt];
				break;
			case 2:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1];
				break;
			case 3:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p22[cnt+2];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1]+p1[cnt+2]*p23[cnt+2];
				break;
			default:
				for(;cnt<col;++cnt)
				{
					xt[0]+=p1[cnt]*p21[cnt];
					xt[1]+=p1[cnt]*p22[cnt];
					xt[2]+=p1[cnt]*p23[cnt];
				}
				break;
			}
		}
		p1+=col;
		//p21+=col;
		//printf("%d\n",*p21);
		//p22+=col;
		//p23+=col;
		//printf("%d\n",*p22);
		//printf("%d\n",*p23);
		pi[i]=xt[0];
		pi1[i]=xt[1];
		pi2[i]=xt[2];
		i++;
	}
	//printf("%d\n",i);
	//for(i=0;i<c->len;i++)
	//{
	//	printf("%d\n",*(c->p+i));
	//}
}

void wtk_qlas_mat_cache_doverflow4(wtk_vecs_t *a1,wtk_mats_t *b,wtk_veci_t *c)
{
	int row=b->row;
	int col=b->col;
	int i,cnt;
	int *p;
	short *p1=b->p;
	short *p21=a1->p;
	short *p22=p21+col;
	short *p23=p22+col;
	short *p24=p23+col;
	//int row2;
	int xt[4];
	int *pi,*pi1,*pi2,*pi3;

//	wtk_veci_t *v1;
//	wtk_veci_t *v2;
//	v1=wtk_veci_new(20);
//	v2=wtk_veci_new(20);

	p=xt;
	//row2=(row>>2)<<2;
	//row2=(row/3)*3;
	pi=c->p;
	pi1=pi+row;
	pi2=pi1+row;
	pi3=pi2+row;
	for(i=0;i<row;)
	{
		cnt=0;
		asm volatile(
				"veor q4,q4,q4\n"
				"veor q5,q5,q5\n"
				"veor q6,q6,q6\n"
				"veor q7,q7,q7\n"
				"ldr r0,[%[col]]\n"
				"mov r1,%[p1]\n"
				"mov r2,%[p21]\n"
				"mov r3,%[p22]\n"
				"mov r4,%[p23]\n"
				"mov r5,%[p24]\n"
				"1:\n"
				"subs r0,r0,#16\n"
				"blt 2f\n"
				//"vld1.16 {q0},[r1]! \n" //8  q2
				//"vld1.16 {q1},[r1]! \n" //8  q3
				"vld1.16 {d0-d3},[r1]!\n"

				//"vld1.16 {q2},[r2]! \n"//8   q4
				//"vld1.16 {q3},[r2]! \n"//8   q5
				"vld1.16 {d4-d7},[r2]!\n"
				//"vmla.i16 q4,q0,q2\n"   //16 q5
				//"vmla.i16 q4,q1,q3\n"  //17 q6
				"vmlal.s16 q4,d0,d4\n"
				"vmlal.s16 q4,d1,d5\n"
				"vmlal.s16 q4,d2,d6\n"
				"vmlal.s16 q4,d3,d7\n"

				//"vld1.16 {q2},[r3]! \n"//8   q4
				//"vld1.16 {q3},[r3]! \n"//8   q5
				"vld1.16 {d4-d7},[r3]!\n"
				//"vmla.i16 q5,q0,q2\n"   //16 q5
				//"vmla.i16 q5,q1,q3\n"  //17 q6
				"vmlal.s16 q5,d0,d4\n"
				"vmlal.s16 q5,d1,d5\n"
				"vmlal.s16 q5,d2,d6\n"
				"vmlal.s16 q5,d3,d7\n"

				//"vld1.16 {q2},[r4]! \n"//8   q4
				//"vld1.16 {q3},[r4]! \n"//8   q5
				"vld1.16 {d4-d7},[r4]!\n"
				//"vmla.i16 q6,q0,q2\n"   //16 q5
				//"vmla.i16 q6,q1,q3\n"  //17 q6
				"vmlal.s16 q6,d0,d4\n"
				"vmlal.s16 q6,d1,d5\n"
				"vmlal.s16 q6,d2,d6\n"
				"vmlal.s16 q6,d3,d7\n"
				
				//"vld1.16 {q2},[r4]! \n"//8
				//"vld1.16 {q3},[r4]! \n"//8
				"vld1.16 {d4-d7},[r5]!\n"
				//"vmla.i16 q7,q0,q2\n"   //
				//"vmla.i16 q7,q1,q3\n"  //
				"vmlal.s16 q7,d0,d4\n"
				"vmlal.s16 q7,d1,d5\n"
				"vmlal.s16 q7,d2,d6\n"
				"vmlal.s16 q7,d3,d7\n"

				"b 1b\n"
				"2:\n"
				"add r0,r0,#16\n"
				"3:\n"
				"subs r0,r0,#4\n"
				"blt 4f\n"
				"vld1.16 d0,[r1]!\n"

				"vld1.16 d1,[r2]!\n"
				//"vmla.i16 d8,d0,d1\n"
				"vmlal.s16 q4,d0,d1\n"

				"vld1.16 d1,[r3]!\n"
				//"vmla.i16 d10,d0,d1\n"
				"vmlal.s16 q5,d0,d1\n"

				"vld1.16 d1,[r4]!\n"
				//"vmla.i16 d12,d0,d1\n"
				"vmlal.s16 q6,d0,d1\n"
				
				"vld1.16 d1,[r5]!\n"
				//"vmla.i16 d12,d0,d1\n"
				"vmlal.s16 q7,d0,d1\n"

				"b 3b\n"
				"4:\n"
				"add r0,r0,#4\n"

				//"vpaddl.s16 q4,q4\n"
				"vpaddl.s32 q4,q4\n"
				"vadd.i32 d8,d8,d9\n"
				"vpaddl.s32  d0,d8\n"
				"mov r1,%[t]\n"
				"vst1.32 {d0[0]},[r1]\n"

				//"vpaddl.s16 q5,q5\n"
				"vpaddl.s32 q5,q5\n"
				"vadd.i32 d8,d10,d11\n"
				"vpaddl.s32  d0,d8\n"
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"

				//"vpaddl.s16 q6,q6\n"
				"vpaddl.s32 q6,q6\n"
				"vadd.i32 d8,d12,d13\n"
				"vpaddl.s32  d0,d8\n"
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"
				
				"vpaddl.s32 q7,q7\n"
				"vadd.i32 d8,d14,d15\n"
				"vpaddl.s32  d0,d8\n"
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"

				"mov %[cnt],r0\n"
				:[t]"+r"(p),
				 [cnt]"+r"(cnt)
				:[p1]"r"(p1),
				 [p21]"r"(p21),
				 [p22]"r"(p22),
				 [p23]"r"(p23),
				 [p24]"r"(p24),
				 [col]"r"(&col)
				:"memory","cc","r0","r1","r2","r3","r4","r5","q0","q1","q2","q3","q4","q5","q6","q7"
				);
		if(cnt>0)
		{
			cnt=col-cnt;
			switch(cnt)
			{
			case 1:
				xt[0]+=p1[cnt]*p21[cnt];
				xt[1]+=p1[cnt]*p22[cnt];
				xt[2]+=p1[cnt]*p23[cnt];
				xt[3]+=p1[cnt]*p24[cnt];
				break;
			case 2:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1];
				xt[3]+=p1[cnt]*p24[cnt]+p1[cnt+1]*p24[cnt+1];
				break;
			case 3:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p22[cnt+2];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1]+p1[cnt+2]*p23[cnt+2];
				xt[3]+=p1[cnt]*p24[cnt]+p1[cnt+1]*p24[cnt+1]+p1[cnt+2]*p24[cnt+2];
				break;
			default:
				for(;cnt<col;++cnt)
				{
					xt[0]+=p1[cnt]*p21[cnt];
					xt[1]+=p1[cnt]*p22[cnt];
					xt[2]+=p1[cnt]*p23[cnt];
					xt[3]+=p1[cnt]*p24[cnt];
				}
				break;
			}
		}
		p1+=col;
		//p21+=col;
		//printf("%d\n",*p21);
		//p22+=col;
		//p23+=col;
		//printf("%d\n",*p22);
		//printf("%d\n",*p23);
		pi[i]=xt[0];
		pi1[i]=xt[1];
		pi2[i]=xt[2];
		pi3[i]=xt[3];
		i++;
	}
	//printf("%d\n",i);
	//for(i=0;i<c->len;i++)
	//{
	//	printf("%d\n",*(c->p+i));
	//}
}

void wtk_qlas_mats_mul_neon32(wtk_vecs_t *a,wtk_mats_t *b,wtk_veci_t *c)
{
	int row=b->row;
	int col=b->col;
	int t,i,cnt;
	int *p;
	short *p1=a->p;
	short *p21=b->p;
	short *p22=p21+col;
	short *p23=p22+col;
	int row2;
	int xt[3];
	int *pi;

	p=xt;
	//row2=(row>>2)<<2;
	row2=(row/3)*3;
	pi=c->p;
	for(i=0;i<row2;)
	{
		cnt=0;
		asm volatile(
				"veor q4,q4,q4\n"
				"veor q5,q5,q5\n"
				"veor q6,q6,q6\n"
				"ldr r0,[%[col]]\n"
				"mov r1,%[p1]\n"
				"mov r2,%[p21]\n"
				"mov r3,%[p22]\n"
				"mov r4,%[p23]\n"
				"1:\n"
				"subs r0,r0,#16\n"
				"blt 2f\n"
				"vld1.16 {q0},[r1]! \n" //8  q2
				"vld1.16 {q1},[r1]! \n" //8  q3

				"vld1.16 {q2},[r2]! \n"//8   q4
				"vld1.16 {q3},[r2]! \n"//8   q5
				"vmla.i16 q4,q0,q2\n"   //16 q5
				"vmla.i16 q4,q1,q3\n"  //17 q6

				"vld1.16 {q2},[r3]! \n"//8   q4
				"vld1.16 {q3},[r3]! \n"//8   q5
				"vmla.i16 q5,q0,q2\n"   //16 q5
				"vmla.i16 q5,q1,q3\n"  //17 q6

				"vld1.16 {q2},[r4]! \n"//8   q4
				"vld1.16 {q3},[r4]! \n"//8   q5
				"vmla.i16 q6,q0,q2\n"   //16 q5
				"vmla.i16 q6,q1,q3\n"  //17 q6

				"b 1b\n"
				"2:\n"
				"add r0,r0,#16\n"
				"3:\n"
				"subs r0,r0,#4\n"
				"blt 4f\n"
				"vld1.16 d0,[r1]!\n"

				"vld1.16 d1,[r2]!\n"
				"vmla.i16 d8,d0,d1\n"

				"vld1.16 d1,[r3]!\n"
				"vmla.i16 d10,d0,d1\n"

				"vld1.16 d1,[r4]!\n"
				"vmla.i16 d12,d0,d1\n"

				"b 3b\n"
				"4:\n"
				"add r0,r0,#4\n"

				"vpaddl.s16 q4,q4\n"
				"vadd.i32 d8,d8,d9\n"
				"vpaddl.s32  d0,d8\n"
				"mov r1,%[t]\n"
				"vst1.32 {d0[0]},[r1]\n"

				"vpaddl.s16 q5,q5\n"
				"vadd.i32 d8,d10,d11\n"
				"vpaddl.s32  d0,d8\n"
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"

				"vpaddl.s16 q6,q6\n"
				"vadd.i32 d8,d12,d13\n"
				"vpaddl.s32  d0,d8\n"
				"add r1,#4\n"
				"vst1.32 {d0[0]},[r1]\n"

				"mov %[cnt],r0\n"
				:[t]"+r"(p),
				 [cnt]"+r"(cnt)
				:[p1]"r"(p1),
				 [p21]"r"(p21),
				 [p22]"r"(p22),
				 [p23]"r"(p23),
				 [col]"r"(&col)
				:"memory","cc","r0","r1","r2","r3","r4","q0","q1","q2","q3","q4","q5","q6"
				);
		if(cnt>0)
		{
			cnt=col-cnt;
			switch(cnt)
			{
			case 1:
				xt[0]+=p1[cnt]*p21[cnt];
				xt[1]+=p1[cnt]*p22[cnt];
				xt[2]+=p1[cnt]*p23[cnt];
				break;
			case 2:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1];
				break;
			case 3:
				xt[0]+=p1[cnt]*p21[cnt]+p1[cnt+1]*p21[cnt+1]+p1[cnt+2]*p21[cnt+2];
				xt[1]+=p1[cnt]*p22[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p22[cnt+2];
				xt[2]+=p1[cnt]*p23[cnt]+p1[cnt+1]*p23[cnt+1]+p1[cnt+2]*p23[cnt+2];
				break;
			default:
				for(;cnt<col;++cnt)
				{
					xt[0]+=p1[cnt]*p21[cnt];
					xt[1]+=p1[cnt]*p22[cnt];
					xt[2]+=p1[cnt]*p23[cnt];
				}
				break;
			}
		}
		p21+=col;
		p22+=col;
		p23+=col;
		pi[i++]=xt[0];
		pi[i++]=xt[1];
		pi[i++]=xt[2];
	}
	if(i<row)
	{
		p21=p23;
		p=&t;
		for(;i<row;++i)
		{
			cnt=0;
			asm volatile(
					"veor q4,q4,q4\n"
					"ldr r0,[%[col]]\n"
					"mov r1,%[p1]\n"
					"mov r2,%[p2]\n"
					"1:\n"
					"subs r0,r0,#16\n"
					"blt 2f\n"
					"vld1.16 {q0},[r1]! \n" //8  q2
					"vld1.16 {q1},[r1]! \n" //8  q3
					"vld1.16 {q2},[r2]! \n"//8   q4
					"vld1.16 {q3},[r2]! \n"//8   q5
					"vmla.i16 q4,q0,q2\n"   //16 q5
					"vmla.i16 q4,q1,q3\n"  //17 q6
					"b 1b\n"
					"2:\n"
					"add r0,r0,#16\n"
					"3:\n"
					"subs r0,r0,#4\n"
					"blt 4f\n"
					"vld1.16 d0,[r1]!\n"
					"vld1.16 d1,[r2]!\n"
					"vmla.i16 d8,d0,d1\n"
					"b 3b\n"
					"4:\n"
					"add r0,r0,#4\n"
					"vpaddl.s16 q4,q4\n"  //前8位有效
					"vadd.i32 d8,d8,d9\n"  //32*2
					"vpaddl.s32  d0,d8\n"
					"mov r1,%[t]\n"
					"vst1.32 {d0[0]},[r1]\n"
					"mov %[cnt],r0\n"
					:[t]"+r"(p),
					 [cnt]"+r"(cnt)
					:[p1]"r"(p1),
					 [p2]"r"(p21),
					 [col]"r"(&col)
					:"memory","cc","r0","r1","r2","q0","q1","q2","q3","q4","q5"
					);
			//t2=p[0]+p[1];
			//printf("v[%d/%d]=%d\n",cnt,col,t);
			if(cnt>0)
			{
				cnt=col-cnt;
				switch(cnt)
				{
				case 1:
					t+=p1[cnt]*p21[cnt];
					break;
				case 2:
					t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p22[cnt+1];
					break;
				case 3:
					t+=p1[cnt]*p21[cnt]+p1[cnt+1]*p22[cnt+1]+p1[cnt+2]*p23[cnt+2];
					break;
				default:
					for(;cnt<col;++cnt)
					{
						t+=p1[cnt]*p21[cnt];
					}
					break;
				}
			}
			p21+=col;
			pi[i]=t;
		}
	}
}

void wtk_qlas_mats_mul_neon32_raw(wtk_vecs_t *a,wtk_mats_t *b,wtk_veci_t *c)
{
	int row=b->row;
	int col=b->col;
	int t,i,cnt;
	int *p;
	short *p1=a->p;
	short *p2=b->p;

	p=&t;
	for(i=0;i<row;++i)
	{
		cnt=0;
		asm volatile(
				"veor q4,q4,q4\n"
				"veor q5,q5,q5\n"
				"ldr r0,[%[col]]\n"
				"mov r1,%[p1]\n"
				"mov r2,%[p2]\n"
				"1:\n"
				"subs r0,r0,#16\n"
				"blt 2f\n"
				"vld1.16 {q0},[r1]! \n" //8  q2
				"vld1.16 {q1},[r1]! \n" //8  q3
				"vld1.16 {q2},[r2]! \n"//8   q4
				"vld1.16 {q3},[r2]! \n"//8   q5
				"vmla.i16 q4,q0,q2\n"   //16 q5
				"vmla.i16 q5,q1,q3\n"  //17 q6
				"b 1b\n"
				"2:\n"
				"add r0,r0,#16\n"
				"3:\n"
				"subs r0,r0,#4\n"
				"blt 4f\n"
				"vld1.16 d0,[r1]!\n"
				"vld1.16 d1,[r2]!\n"
				"vmla.i16 d8,d0,d1\n"
				"b 3b\n"
				"4:\n"
				"add r0,r0,#4\n"
				"vpaddl.s16 q4,q4\n"  //前8位有效
				"vpaddl.s16 q5,q5\n"  //前8位有效
				"vadd.i32 q4,q4,q5\n"  //32*4
				"vadd.i32 d8,d8,d9\n"  //32*2
				"vpaddl.s32  d0,d8\n"
				"mov r1,%[t]\n"
				"vst1.32 {d0[0]},[r1]\n"
				"mov %[cnt],r0\n"
				:[t]"+r"(p),
				 [cnt]"+r"(cnt)
				:[p1]"r"(p1),
				 [p2]"r"(p2),
				 [col]"r"(&col)
				:"memory","cc","r0","r1","r2","q0","q1","q2","q3","q4","q5","q6"
				);
		//t2=p[0]+p[1];
		//printf("v[%d/%d]=%d\n",cnt,col,t);
		if(cnt>0)
		{
			cnt=col-cnt;
			switch(cnt)
			{
			case 1:
				t+=p1[cnt]*p2[cnt];
				break;
			case 2:
				t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1];
				break;
			case 3:
				t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2];
				break;
			default:
				for(;cnt<col;++cnt)
				{
					t+=p1[cnt]*p2[cnt];
				}
				break;
			}
		}
		p2+=col;
		c->p[i]=t;
	}
}

int wtk_short_multi_neon(short *p1,short *p2,int col)
{
	int t;
	int *p;
	int cnt;

	p=&t;
	cnt=0;
	asm volatile(
			"veor q6,q6,q6\n"
			"veor q7,q7,q7\n"
			"ldr r0,[%[col]]\n"
			"mov r1,%[p1]\n"
			"mov r2,%[p2]\n"
			"1:\n"
			"subs r0,r0,#16\n"
			"blt 2f\n"
			"vld1.16 {q2},[r1]! \n" //8  q2
			"vld1.16 {q3},[r1]! \n" //8  q3
			"vld1.16 {q4},[r2]! \n"//8   q4
			"vld1.16 {q5},[r2]! \n"//8   q5
			"vmla.i16 q6,q2,q4\n"   //16 q5
			"vmla.i16 q7,q3,q5\n"  //17 q6
			"b 1b\n"
			"2:\n"
			"add r0,r0,#16\n"
			"3:\n"
			"subs r0,r0,#4\n"
			"blt 4f\n"
			"vld1.16 d2,[r1]!\n"
			"vld1.16 d3,[r2]!\n"
			"vmla.i16 d12,d2,d3\n"
			"b 3b\n"
			"4:\n"
			"add r0,r0,#4\n"
			"vpaddl.s16 q6,q6\n"  //前8位有效
			"vpaddl.s16 q7,q7\n"  //前8位有效
			"vadd.i32 q6,q6,q7\n"  //32*4
			"vadd.i32 d12,d12,d13\n"  //32*2
			"vpaddl.s32  d0,d12\n"
			"mov r1,%[t]\n"
			"vst1.32 {d0[0]},[r1]\n"
			"mov %[cnt],r0\n"
			:[t]"+r"(p),
			 [cnt]"+r"(cnt)
			:[p1]"r"(p1),
			 [p2]"r"(p2),
			 [col]"r"(&col)
			:"memory","cc","r0","r1","r2","q2","q3","q4","q5","q6","q7","q8"
			);
	//t2=p[0]+p[1];
	//printf("v[%d/%d]=%d\n",cnt,col,t);
	if(cnt>0)
	{
		cnt=col-cnt;
		switch(cnt)
		{
		case 1:
			t+=p1[cnt]*p2[cnt];
			break;
		case 2:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1];
			break;
		case 3:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2];
			break;
		default:
			for(;cnt<col;++cnt)
			{
				t+=p1[cnt]*p2[cnt];
			}
			break;
		}
	}
	return t;
}

int wtk_short_multi_neon2(short *p1,short *p2,int col)
{
	int t;
	int *p;
	int cnt;

	p=&t;
	cnt=0;
	asm volatile(
			"veor q8,q8,q8\n"
			"ldr r0,[%[col]]\n"
			"mov r1,%[p1]\n"
			"mov r2,%[p2]\n"
			"1:\n"
			"subs r0,r0,#16\n"
			"blt 2f\n"
			"vld1.16 {q2},[r1]! \n" //8  q2
			"vld1.16 {q3},[r1]! \n" //8  q3
			"vld1.16 {q4},[r2]! \n"//8   q4
			"vld1.16 {q5},[r2]! \n"//8   q5
			"vmul.i16 q6,q2,q4\n"   //16 q5
			"vmul.i16 q7,q3,q5\n"  //17 q6
			"vpadal.s16  q8,q6\n"
			"vpadal.s16  q8,q7\n"
			"b 1b\n"
			"2:\n"
			"add r0,r0,#16\n"
			"3:\n"
			"subs r0,r0,#4\n"
			"blt 4f\n"
			"vld1.16 d2,[r1]!\n"
			"vld1.16 d3,[r2]!\n"
			"vmul.i16 d4,d2,d3\n"
			"vpadal.s16  d16,d4\n"
			"b 3b\n"
			"4:\n"
			"add r0,r0,#4\n"
			"vadd.i32 d4,d16,d17\n"
			"vpaddl.s32 d0,d4\n"  //前8位有效
			"mov r1,%[t]\n"
			"vst1.32 {d0[0]},[r1]\n"
			"mov %[cnt],r0\n"
			:[t]"+r"(p),
			 [cnt]"+r"(cnt)
			:[p1]"r"(p1),
			 [p2]"r"(p2),
			 [col]"r"(&col)
			:"memory","cc","r0","r1","r2","r3","q2","q3","q4","q5","q6","q7","q8"
			);
	//t2=p[0]+p[1];
	//printf("v[%d/%d]=%d\n",cnt,col,t);
	if(cnt>0)
	{
		cnt=col-cnt;
		switch(cnt)
		{
		case 1:
			t+=p1[cnt]*p2[cnt];
			break;
		case 2:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1];
			break;
		case 3:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2];
			break;
		default:
			for(;cnt<col;++cnt)
			{
				t+=p1[cnt]*p2[cnt];
			}
			break;
		}
	}
	return t;
}
#endif
