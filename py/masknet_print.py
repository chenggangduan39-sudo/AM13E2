import torch as th
import numpy as np
import sys
import masknet
from utils.hparams import HParam
import mix;

def print_wei_bias(wei,bias):
    wei = wei.detach().numpy().astype(np.float128)
    n,m = wei.shape
    print('weight'+'\t'+str(n)+'\t'+str(m))    
    for i in range(n):
        for j in range(m):
            print(wei[i,j],end=' ')
        print()

    if bias is not None:
        bias = bias.detach().numpy().astype(np.float128)
        n = bias.shape[0]    
        print('bias'+'\t'+str(n))
        for i in range(n):
            print(bias[i],end=' ')
        print()

def print_lstm_wei_bias(weii,weih,biasi,biash):
    weii = weii.detach().numpy().astype(np.float128)
    n,m = weii.shape
    print('weight_ih_l'+'\t'+str(n)+'\t'+str(m))    
    for i in range(n):
        for j in range(m):
            print(weii[i,j],end=' ')
        print()

    biasi = biasi.detach().numpy().astype(np.float128)
    n = biasi.shape[0]    
    print('bias_ih_l'+'\t'+str(n))
    for i in range(n):
        print(biasi[i],end=' ')
    print()

    weih = weih.detach().numpy().astype(np.float128)
    n,m = weih.shape
    print('weight_hh_l'+'\t'+str(n)+'\t'+str(m))    
    for i in range(n):
        for j in range(m):
            print(weih[i,j],end=' ')
        print()

    biash = biash.detach().numpy().astype(np.float128)
    n = biash.shape[0]    
    print('bias_hh_l'+'\t'+str(n))
    for i in range(n):
        print(biash[i],end=' ')
    print()

        
def print_cnn_wei_bias(wei,bias):
    wei = wei.detach().numpy().astype(np.float128)
    ouc,inc,ks1,ks2 = wei.shape
    n=ouc*inc*ks1
    m=ks2
    print('weight'+'\t'+str(n)+'\t'+str(m))    
    for i in range(ouc):
        for j in range(inc):
            for k in range(ks1):
                for o in range(ks2):
                    print(wei[i,j,k,o],end=' ')
                print()

    if bias is not None:
        bias = bias.detach().numpy().astype(np.float128)
        n = bias.shape[0]    
        print('bias'+'\t'+str(n))
        for i in range(n):
            print(bias[i],end=' ')
        print()

def print_batchnorm(wei,bias,bmean,bvar):
    wei = wei.detach().numpy().astype(np.float128)
    n = wei.shape[0]
    print('batchnorm_weight'+'\t'+str(n))    
    for i in range(n):
        print(wei[i],end=' ')
    print()

    bias = bias.detach().numpy().astype(np.float128)
    n = bias.shape[0]    
    print('batchnorm_bias'+'\t'+str(n))
    for i in range(n):
        print(bias[i],end=' ')
    print()

    bmean = bmean.detach().numpy().astype(np.float128)
    n = bmean.shape[0]    
    print('running_mean'+'\t'+str(n))
    for i in range(n):
        print(bmean[i],end=' ')
    print()

    bvar = bvar.detach().numpy().astype(np.float128)
    n = bvar.shape[0]    
    print('running_var'+'\t'+str(n))
    for i in range(n):
        print(bvar[i],end=' ')
    print()


def print_net(cfg,mdl_fn):

    hp = HParam(cfg);
    input=mix.get_input(hp);
    nbin = int(hp.feat.frame_win / 2) + 1;

    net = masknet.MaskNet(hp.model,input,nbin,dropout=0.0);

    #print("DCNet:\n{}".format(net));
    device='cpu';
    net.load_state_dict(th.load(mdl_fn, map_location=device))

    net_dict=net.state_dict()

    for k,v in net_dict.items():
        print(k)
        print(v.shape)

    if net.use_cnn:
        if hp.model.use_full_cnn:
            print('cnn'+'\t'+'use_bias'+'\t'+'use_relu'+'\t'+'1'+'\t'+str(hp.model.chan1)+'\t'+'1'+'\t'+str(hp.model.cnn1)+'\t'+'1'+'\t'+'1')
            print_cnn_wei_bias(net_dict['conv.1.weight'],net_dict['conv.1.bias'])
            print('batchnorm'+'\t'+'use_affine')
            print_batchnorm(net_dict['conv.2.weight'],net_dict['conv.2.bias'],net_dict['conv.2.running_mean'],net_dict['conv.2.running_var'])

            print('cnn'+'\t'+'use_bias'+'\t'+'use_relu'+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.cnn2)+'\t'+'1'+'\t'+'1'+'\t'+'1')
            print_cnn_wei_bias(net_dict['conv.5.weight'],net_dict['conv.5.bias'])
            print('batchnorm'+'\t'+'use_affine')
            print_batchnorm(net_dict['conv.6.weight'],net_dict['conv.6.bias'],net_dict['conv.6.running_mean'],net_dict['conv.6.running_var'])

            print('cnn'+'\t'+'use_bias'+'\t'+'use_relu'+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.cnn3)+'\t'+str(hp.model.cnn3)+'\t'+'1'+'\t'+'1')
            print_cnn_wei_bias(net_dict['conv.9.weight'],net_dict['conv.9.bias'])
            print('batchnorm'+'\t'+'use_affine')
            print_batchnorm(net_dict['conv.10.weight'],net_dict['conv.10.bias'],net_dict['conv.10.running_mean'],net_dict['conv.10.running_var'])

            print('cnn'+'\t'+'use_bias'+'\t'+'use_relu'+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.chan1)+'\t'+'5'+'\t'+'5'+'\t'+'2'+'\t'+'1')
            print_cnn_wei_bias(net_dict['conv.13.weight'],net_dict['conv.13.bias'])
            print('batchnorm'+'\t'+'use_affine')
            print_batchnorm(net_dict['conv.14.weight'],net_dict['conv.14.bias'],net_dict['conv.14.running_mean'],net_dict['conv.14.running_var'])

            print('cnn'+'\t'+'use_bias'+'\t'+'use_relu'+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.chan1)+'\t'+'5'+'\t'+'5'+'\t'+'4'+'\t'+'1')
            print_cnn_wei_bias(net_dict['conv.17.weight'],net_dict['conv.17.bias'])
            print('batchnorm'+'\t'+'use_affine')
            print_batchnorm(net_dict['conv.18.weight'],net_dict['conv.18.bias'],net_dict['conv.18.running_mean'],net_dict['conv.18.running_var'])

            print('cnn'+'\t'+'use_bias'+'\t'+'use_relu'+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.chan1)+'\t'+'5'+'\t'+'5'+'\t'+'8'+'\t'+'1')
            print_cnn_wei_bias(net_dict['conv.21.weight'],net_dict['conv.21.bias'])
            print('batchnorm'+'\t'+'use_affine')
            print_batchnorm(net_dict['conv.22.weight'],net_dict['conv.22.bias'],net_dict['conv.22.running_mean'],net_dict['conv.22.running_var'])

            print('cnn'+'\t'+'use_bias'+'\t'+'use_relu'+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.chan1)+'\t'+'5'+'\t'+'5'+'\t'+'16'+'\t'+'1')
            print_cnn_wei_bias(net_dict['conv.25.weight'],net_dict['conv.25.bias'])
            print('batchnorm'+'\t'+'use_affine')
            print_batchnorm(net_dict['conv.26.weight'],net_dict['conv.26.bias'],net_dict['conv.26.running_mean'],net_dict['conv.26.running_var'])

            print('cnn'+'\t'+'use_bias'+'\t'+'use_relu'+'\t'+str(hp.model.chan1)+'\t'+str(hp.model.chan2)+'\t'+'1'+'\t'+'1'+'\t'+'1'+'\t'+'1')
            print_cnn_wei_bias(net_dict['conv.28.weight'],net_dict['conv.28.bias'])
            print('batchnorm'+'\t'+'use_affine')
            print_batchnorm(net_dict['conv.29.weight'],net_dict['conv.29.bias'],net_dict['conv.29.running_mean'],net_dict['conv.29.running_var'])

            input_dim = input*hp.model.chan2

    if net.use_lstm:
            print('lstm'+'\t'+str(input_dim)+'\t'+str(hp.model.lstm_dim)+'\t'+'1')
            print_lstm_wei_bias(net_dict['lstm.weight_ih_l0'],net_dict['lstm.weight_hh_l0'],net_dict['lstm.bias_ih_l0'],net_dict['lstm.bias_hh_l0'])

            print('dnn'+'\t'+'use_bias'+'\t'+'use_relu')
            print_wei_bias(net_dict['fc1.weight'],net_dict['fc1.bias'])

            print('dnn'+'\t'+'use_bias'+'\t'+'use_sigmoid')
            print_wei_bias(net_dict['fc2.weight'],net_dict['fc2.bias'])


if __name__ == '__main__':
    print_net(sys.argv[1],sys.argv[2])
