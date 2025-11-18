#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/asr/img/qtk_img2.h"
#include "wtk/core/wtk_riff.h"

static int count=0;
void img2_notify_asr(void *instance, int id, float prob, int start, int end)
{
    //qtk_img2_rec_t *img2 = (qtk_img2_rec_t*)instance;
	++count;
#if 0
    printf("%f %f\n", start*0.08,end*0.08);
    switch (id)
    {
        case 1:
            printf("打开相册");
            break;
        case 2:
            printf("稍后提醒");
            break;
        case 3:
            printf("打开日程");
            break;
        case 4:
            printf("查找手机");
            break;
        case 5:
            printf("增大亮度");
            break;
        case 6:
            printf("开始录音");
            break;
        default:
            break;
    }
    printf("\n");
#endif
    //qtk_img2_rec_feed(img2,NULL,0,1);
    //qtk_img2_rec_reset2(img2);
}

float ff[]={-0.087791,-5.819783,-8.366564,-4.084229,-4.165926,-7.40133,-4.262999,-6.832147,-3.515118,-6.123557,-7.141675,
    -0.0492951,-6.143535,-8.74523,-4.82396,-5.022449,-7.398708,-4.605887,-7.308082,-4.028909,-6.588231,-7.229516,
    -0.0102437,-7.204711,-9.104306,-6.83802,-7.235632,-7.787286,-5.566443,-8.710217,-6.140767,-7.500339,-7.741779,
    -0.001755603,-9.318873,-10.27109,-8.459807,-9.168671,-9.619241,-7.139481,-10.4849,-8.416591,-9.160268,-9.211802,
    -0.001133276,-8.648906,-10.21979,-8.442684,-8.366165,-9.931849,-8.704862,-9.963159,-9.203836,-9.816358,-9.786022,
    -0.001556258,-8.59978,-9.554197,-8.062733,-8.052466,-9.419033,-8.57637,-9.717566,-8.977802,-9.239401,-9.084057,
    -0.006412406,-6.663846,-7.707799,-6.586004,-7.377751,-8.67796,-8.323779,-8.284171,-7.339914,-7.130482,-7.51145,
    -0.02498458,-5.733028,-6.657456,-4.726744,-6.409395,-6.598397,-6.556118,-6.810001,-7.50211,-5.765023,-6.176195,
    -0.02086418,-6.455145,-6.945591,-5.361383,-6.09432,-6.488383,-6.648166,-5.515426,-6.961677,-6.209004,-6.598455,
		-0.0502713,-5.208308,-5.830068,-4.709949,-5.390165,-6.184235,-6.242986,-4.425174,-5.72938,-5.63975,-5.455545,
		-0.8058621,-4.306038,-4.943312,-2.75464,-3.614755,-3.574533,-4.369318,-2.040244,-1.535207,-3.190974,-4.206678,
		-5.760464,-6.980966,-6.696501,-4.429451,-4.597473,-5.097158,-5.944213,-2.255352,-0.1642334,-4.642183,-6.974882,
		-9.189075,-8.889003,-7.69559,-5.548917,-5.458028,-7.43836,-7.643918,-3.710783,-0.0363098,-6.839581,-8.450129,
		-17.60256,-13.81571,-12.55471,-10.39893,-10.48145,-12.05637,-12.18251,-7.823004,-0.000487328,-11.44596,-13.09629,
		-23.80398,-17.51151,-15.4008,-13.12017,-13.85026,-13.90375,-14.50191,-10.62693,-2.992108e-05,-14.06678,-15.17585,
		-23.54496,-17.09925,-15.50774,-11.93287,-13.0154,-13.06752,-13.66595,-9.764506,-7.271503e-05,-13.03402,-13.99022,
		-19.42517,-15.59369,-14.39188,-10.8679,-11.82287,-13.04998,-13.67003,-7.297149,-0.0007264359,-10.93804,-14.49761,
		-15.26096,-14.1845,-13.31766,-10.18437,-11.5377,-12.11344,-11.43732,-7.553268,-0.0006431657,-9.9377,-12.47313,
		-10.68302,-11.01828,-11.15441,-6.654137,-7.712492,-8.938332,-8.333458,-6.438596,-0.004979708,-6.842772,-8.871536,
		-5.156002,-8.194893,-8.174785,-3.338577,-3.959249,-5.213395,-5.002255,-5.584054,-0.1368814,-3.58766,-3.752759};

void feed_img2_feat(qtk_img2_rec_t *img2)
{
	int i;
	qtk_blas_matrix_t * b;
	for(i = 0; i < 20; i++)
	{
		b = qtk_blas_matrix_new(1, 11);
		memcpy(b->m, (ff + 11 * i), sizeof(float) * 11);
		//qtk_img2_rec_nnet3_notify(img2,b,0,0);
	}
	//qtk_img2_rec_nnet3_notify(img2,NULL,1,0);
}

void feed_img2(qtk_img2_rec_t *img2, char *wav_fn)
{
    char buf[64000];
    int ret;
    wtk_riff_t *riff = NULL;

    riff = wtk_riff_new();
    wtk_riff_open(riff, wav_fn);

    do
    {
        ret = wtk_riff_read(riff, buf, 64000);
        if (ret > 0) {
            qtk_img2_rec_feed(img2, buf, ret, 0);
        }
    } while (ret > 0);
    qtk_img2_rec_feed(img2, NULL, 0, 1);

    //printf("count=[%d]\n", count);

    wtk_riff_close(riff);
    wtk_riff_delete(riff);
    qtk_img2_rec_reset(img2);
}

void feed_img2_scp(qtk_img2_rec_t *img2,char *wav_fn)
{
    wtk_flist_t *f;
    wtk_queue_node_t *n;
    wtk_fitem_t *item;
    int i;

    f=wtk_flist_new(wav_fn);
    if(f)
    {
        for(i=0,n=f->queue.pop;n;n=n->next,++i)
        {
            item=data_offset(n,wtk_fitem_t,q_n);
            feed_img2(img2,item->str->data);
        }
        wtk_flist_delete(f);
    }
}

int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=0;
    qtk_img2_rec_t *img2=NULL;
    qtk_img2_cfg_t *cfg;
	wtk_arg_t *arg;
    qtk_img2_thresh_cfg_t normal_thresh;
    char *cfg_fn=0;
    char *scp_fn=0;
    char *wav_fn=0;
    char *bin_fn=0;

	arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"b",&bin_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
    wtk_arg_get_str_s(arg,"wav",&wav_fn);

    if(cfg_fn)
    {
        main_cfg=wtk_main_cfg_new_type(qtk_img2_cfg,cfg_fn);
        if(!main_cfg)
        {
            wtk_debug("load configure failed.\n");
            return -1;
        }
        cfg = (qtk_img2_cfg_t*)main_cfg->cfg;
        img2 = qtk_img2_rec_new(cfg);
        normal_thresh.av_prob0 = 0.0;
        normal_thresh.avx0 = -0.5;
        normal_thresh.maxx0 = -2.0;
        normal_thresh.max_prob0 = 4.0;
        normal_thresh.speech_dur0 = 1;
        normal_thresh.av_prob1 = 7.5;
        normal_thresh.avx1 = -0.5;
        normal_thresh.maxx1 = -2.0;
        normal_thresh.max_prob1 = 10.0;
        normal_thresh.speech_dur1 = 3;

		qtk_img2_thresh_set_cfg(img2, &normal_thresh, 0);
        qtk_img2_rec_set_notify(img2, (qtk_img2_rec_notify_f)img2_notify_asr, img2);
    }
    else
    {
        printf("%s -c cfg [-scp scpfn] [-wav wavfn]\n", argv[0]);
        printf("\tscpfn or wavfn must exist.\n");
        return -1;
    }

    if(wav_fn)
    {
        feed_img2(img2, wav_fn);
    }
    else if(scp_fn)
    {
        feed_img2_scp(img2, scp_fn);
    }
    else
    {
        feed_img2_feat(img2);
    }

    if(img2)
    {
    	qtk_img2_rec_delete(img2);
    }
    if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
    wtk_arg_delete(arg);

    return 0;
}
