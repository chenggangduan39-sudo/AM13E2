#include "wtk_cosynthesis_likehood.h"
#include "wtk/core/math/wtk_math.h"

//float mgc_floor[] = {
//		-5.824605  ,  0.5462366 ,  2.1852572 , -2.4437742 ,  1.2996849 ,
//		       -2.112048  , -2.2533784 , -2.8681285 , -3.1652477 , -1.2112676 ,
//		       -3.5159976 , -0.68293804, -1.6754858
//};
//float lf0_floor[] = {-1E10};

//按照python tar_spec_mode 每行有两组
// LL += np.sum(((spec[i][j] - miu_s[i][0][j]) / sigma_s[i][1][j]) * (spec[i][j] - miu_s[i][0][j]))
float wtk_cosynthesis_likelihood_spec(float *specs, int srow, int scol ,float *tar_spec_model_mean,float *tar_spec_model_var,  int trow, int tcol)
{
    int i,j;
    float LL1, LL2;
    float *tar_spec_model1 = NULL, *tar_spec_model2 = NULL;
    float f1 = 0.0f, f2 = 0.0f, f3 = 0.0f,f4 = 0.0f;

    LL2=0.0f;
    for(i = 0; i < srow; ++i){
        LL1 = 0.0f;
        tar_spec_model1 = tar_spec_model_mean;
        tar_spec_model2 = tar_spec_model_var;
        for(j = 0; j < scol; ++j){
            f1 = specs[j];
            f2 = tar_spec_model1[j];
            f3 = tar_spec_model2[j];
            f4 = (f1 - f2)/f3;
            LL1 += f4 *(f1-f2);
        }
        LL2 += LL1;
        //printf("LL=%f\n", LL2);
        specs += scol;
        tar_spec_model_mean +=tcol*1;
        tar_spec_model_var +=tcol*1;
    }

    return LL2;
}

float wtk_cosynthesis_likelihood_pitch(float *lf0s, int lrow, int lcol, 
                                            float *tar_pitch_model_mean,float *tar_pitch_model_mean_var,float *tar_pitch_model_w, int trow, int tcol)
{
    float LL = 0.0f;
    int i = 0;
    float *lf0 = NULL, *dist0 = NULL,*dist1 = NULL, *dist2 = NULL;
    //print_float(lf0s, lrow*lcol);
    //print_float(tar_pitch_model_mean, lrow*tcol);
    //print_float(tar_pitch_model_mean_var, lrow*tcol);
    for(i = 0; i < lrow; ++i){
        lf0 = lf0s;
        dist0 = tar_pitch_model_w;
        dist1 = tar_pitch_model_mean;
        dist2 = tar_pitch_model_mean_var;
        if(lf0[0] > 1E-9){
            LL += (powf(lf0[0]-dist1[0],2)/dist2[0])-logf(dist0[0]);
        }else{
            LL += -logf(1-dist0[0]);
        }
        //printf("LL=%f\n", LL);
        lf0s += lcol;
        tar_pitch_model_w += tcol *1;
        tar_pitch_model_mean += tcol*1;
        tar_pitch_model_mean_var += tcol*1;
    }
    //printf("====LL=%f\n", LL);
    //exit(0);
    return LL;
}

float wtk_cosynthesis_likelihood_dur(float *durs, int drow, int dcol, 
                            float *tar_dur_model_mean,float *tar_dur_model_var, int trow, int tcol)
{
    float LL = 0.0f;
    int i = 0;
    float miu = 0.0f, sigma = 0.0f;
    
    for(i = 0 ; i < drow; ++i){
        miu = wtk_float_sum(tar_dur_model_mean,tcol);
        sigma = wtk_float_sum(tar_dur_model_var,tcol);
        LL += ((durs[0]-miu)/sigma) * (durs[0]-miu);
        durs += dcol;
        tar_dur_model_mean += tcol;
        tar_dur_model_var += tcol;
        //printf("LL %f\n", LL);
    }

    return LL;
}

int candi_uid_pre_in_sil(short *pre, int len)
{
    int i = 0;
    for(i = 0; i < len; ++i){
        if(pre[i] < 0) return 1;
    }
    return 0;
}

void wtk_consynthesis_float_sub(float *s1, float *s2, float *d, int len)
{
    int i = 0;
    for(i = 0; i < len; ++i){
        d[i] = s1[i] - s2[i];
    }
    return;
}

int wtk_cosynthesis_get_likehood(wtk_cosynthesis_t *cs, float *tar_spec_model_mean,float *tar_spec_model_var, int tscol,
                                float *tar_picth_model_mean,float *tar_pitch_model_var,float *tar_pitch_model_w, int tpcol,
                                float *tar_dur_model_mean, float *tar_dur_model_var, int tdcol,
                                float *conca_spec_model_mean,float *conca_spec_model_var, int csrow, int cscol,
                                float *conca_pitch_model_mean,float *conca_pitch_model_var,float *conca_pitch_model_w,int cprow,int cpcol,
                                short *candi_uid, int culen,wtk_unit_t *data, 
                                short *candi_uid_pre, int cuplen, wtk_unit_t *data_pre,int nphone,
                                float **glb_select_cost,float ***glb_conca_cost,float spec_w,float dur_w,
                                float pitch_w,float conca_spec_w,float conca_pitch_w)
{
    int i = 0, j = 0;
    float *specs = NULL, *lf0s = NULL, *dur = NULL;
    int uid = 0;
    int specs_col = 0; // specs_row = 0
    int lf0_col = 0;   // lf0_row = 0
    int dur_col = 0;   // dur_row = 0
    float lh_spec,lh_picth,lh_dur;
    float *spec_cur = NULL, *lf0_cur = NULL;
    float *select_cost = NULL;
    float *conca_cost_ = NULL;
    float *lf0_pre = NULL, *delta_spec = NULL,*delta_pitch = NULL;
    int uid_pre = 0;
    float lh_conca_spec,lh_conca_pitch;
    float **conca_cost = NULL;
    int ret=0, nphn;

    select_cost = wtk_malloc(sizeof(float)*culen);
    conca_cost = wtk_malloc(sizeof(float*)*culen);
    delta_spec = wtk_malloc(sizeof(float)*tscol);
    delta_pitch = wtk_malloc(sizeof(float));
    //printf("candi_uid_pre=%p ,nphone=%d\n", candi_uid_pre, nphone);
    //print_short(candi_uid, culen);
    for(i = 0; i < culen; ++i){
    	nphn=nphone;
        uid = candi_uid[i];
        //printf("uid=%d\n", uid);
        if(uid > 0){   //假设sil 标志小于0
            specs = data[uid].spec;
//            specs_row = data[i].nphone;
            specs_col = tscol;
            lf0s = data[uid].lf0;
//            lf0_row = data[i].nphone;
            lf0_col = 1;
            dur = data[uid].dur;
//            dur_row = data[i].nphone;
            dur_col = 1;
            // wtk_debug("=============select\n");
            if (data[uid].nphone < nphn)
            {
#ifdef CONCA_DEBUG
            		wtk_debug("warnning: diff nphone=%d candi_unit.nphone=%d\n", nphone, data[uid].nphone);
#endif
            	nphn = data[uid].nphone;
            }
        	lh_spec = wtk_cosynthesis_likelihood_spec(specs,nphn,specs_col,tar_spec_model_mean,tar_spec_model_var,nphn,tscol);
        	lh_picth = wtk_cosynthesis_likelihood_pitch(lf0s,nphn,lf0_col,tar_picth_model_mean,tar_pitch_model_var,tar_pitch_model_w,nphn,tpcol);
        	lh_dur = wtk_cosynthesis_likelihood_dur(dur,nphn,dur_col,tar_dur_model_mean,tar_dur_model_var,nphn,tdcol);
        	//wtk_debug("lh_spec:%f,lh_picth:%f,lh_dur:%f\n", lh_spec,lh_picth,lh_dur);
            spec_cur = specs;
            lf0_cur = lf0s;
        }else{
            lh_spec = 0.0f;
            lh_picth = 0.0f;
            lh_dur = 0.0f;

            //spec_cur = mgc_floor;
            //lf0_cur = lf0_floor;
            spec_cur = ((float*)cs->cfg->mgc_floor->slot);
            lf0_cur = ((float*)cs->cfg->lf0_floor->slot);
        }
        if(1==culen)
        	select_cost[i]=0;
        else
        	select_cost[i] = spec_w*lh_spec + dur_w*lh_dur + pitch_w*lh_picth; //weigtgh_spec/dur/pitch
        if(candi_uid_pre == NULL){  //第一个词的情况
            conca_cost_ = (float*)wtk_malloc(sizeof(float));
            conca_cost_[0] = 0.0f;
        }else if(candi_uid_pre_in_sil(candi_uid_pre,cuplen)){   //前个uid有sil的情况
            conca_cost_ = wtk_malloc(sizeof(float));
            //wtk_consynthesis_float_sub(spec_cur,mgc_floor,delta_spec,tscol);
            wtk_consynthesis_float_sub(spec_cur,(float*)cs->cfg->mgc_floor->slot,delta_spec,tscol);
            //delta_pitch[0] = lf0_floor[0];
            delta_pitch[0] = ((float*)cs->cfg->lf0_floor->slot)[0];
            lh_conca_spec = wtk_cosynthesis_likelihood_spec(delta_spec,1,tscol,conca_spec_model_mean,conca_spec_model_var,csrow,cscol);
            lh_conca_pitch = wtk_cosynthesis_likelihood_pitch(delta_pitch,1,1,conca_pitch_model_mean,conca_pitch_model_var,conca_pitch_model_w,cprow,cpcol);
            conca_cost_[0] = conca_spec_w*lh_conca_spec + conca_pitch_w * lh_conca_pitch;//weigtgh_conca_spec/pitch
            //wtk_debug("%f,%f,%f\n", conca_cost_[0],lh_conca_spec,lh_conca_pitch);
            //exit(0);
        }else{
            conca_cost_ =  wtk_malloc(sizeof(float)*cuplen);
            float *conca_cost_p = conca_cost_;

            for(j = 0; j < cuplen; ++j){
                uid_pre = candi_uid_pre[j];
                //wtk_debug("uid_pre=%d\n", uid_pre);
                int pre_nphone = data_pre[uid_pre].nphone;
                wtk_consynthesis_float_sub(spec_cur,data_pre[uid_pre].spec+((pre_nphone-1)*tscol),delta_spec,tscol);
                lf0_pre = data_pre[uid_pre].lf0+pre_nphone-1;
                if(lf0_pre[0] <= -1e9 || lf0_cur[0] <= -1e9){
                    delta_pitch[0] = -1e10;
                }else{
                    delta_pitch[0] = lf0_cur[0] - lf0_pre[0];
                }
                lh_conca_spec = wtk_cosynthesis_likelihood_spec(delta_spec,1,tscol,conca_spec_model_mean,conca_spec_model_var,csrow,cscol);
                lh_conca_pitch = wtk_cosynthesis_likelihood_pitch(delta_pitch,1,1,conca_pitch_model_mean,conca_pitch_model_var,conca_pitch_model_w,cprow,cpcol);
                conca_cost_p[j] = conca_spec_w*lh_conca_spec + conca_pitch_w * lh_conca_pitch;
                //printf("lh_conca_spec %f\n", lh_conca_spec);
                //printf("lh_conca_pitch %f\n", lh_conca_pitch);
                //wtk_debug("conca_cost_p[%d]=%f\n", j, conca_cost_p[j]);
                //wtk_debug("%f,%f,%f\n", conca_cost_p[j],lh_conca_spec,lh_conca_pitch);
                //exit(0);
            }
            //wtk_debug("conca_cost_\n");
            //print_float(conca_cost_,cuplen);
            //exit(0);
        }
        //printf("conca_spec_w %f\n", conca_spec_w);
        //printf("conca_pitch_w %f\n", conca_pitch_w);
        conca_cost[i] = conca_cost_;
    }
    //wtk_debug("select_cost\n");
    //print_float(select_cost, culen);
    //exit(0);
    if(glb_select_cost)
    {
        *glb_select_cost = select_cost;
    }else
    {
        wtk_free(select_cost);
    }
    *glb_conca_cost = conca_cost;

//end:
    wtk_free(delta_pitch);
    wtk_free(delta_spec);
    return ret;
}
