#!/usr/bin/env python3

import os,sys
import vprint as vp
import hparam
import torch
import numpy as np
from speech_embedder_net import SpeechEmbedder, GE2ELoss, get_centroids, get_cossim


def print_lstm_wei_bias(wei,bias,hwei,hbias):
    wei = wei.detach().numpy().astype(np.float128)
    n,m = wei.shape
    print('weight_ih_l'+'\t'+str(n)+'\t'+str(m))    
    for i in range(n):
        for j in range(m):
            print(wei[i,j],end=' ')
        print()

    if bias is not None:
        bias = bias.detach().numpy().astype(np.float128)
        n = bias.shape[0]    
        print('bias_ih_l'+'\t'+str(n))
        for i in range(n):
            print(bias[i],end=' ')
        print()


    hwei = hwei.detach().numpy().astype(np.float128)
    n,m = hwei.shape
    print('weight_hh_l'+'\t'+str(n)+'\t'+str(m))    
    for i in range(n):
        for j in range(m):
            print(hwei[i,j],end=' ')
        print()

    if hbias is not None:
        hbias = hbias.detach().numpy().astype(np.float128)
        n = hbias.shape[0]    
        print('bias_hh_l'+'\t'+str(n))
        for i in range(n):
            print(hbias[i],end=' ')
        print()


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


def print_net(cfg_fn,mdl_fn,wb_fn):
    hp = hparam.Hparam(cfg_fn);
    embedder_net = SpeechEmbedder(hp)
    embedder_net.load_state_dict(torch.load(mdl_fn))
    ge2e_loss = GE2ELoss('cpu')
    ge2e_loss.load_state_dict(torch.load(wb_fn))
    embedder_net_dict=embedder_net.state_dict()
    ge2e_loss_dict=ge2e_loss.state_dict()
    """
    print_lstm_wei_bias(embedder_net_dict['LSTM_stack.weight_ih_l0'],embedder_net_dict['LSTM_stack.bias_ih_l0'],
                embedder_net_dict['LSTM_stack.weight_hh_l0'],embedder_net_dict['LSTM_stack.bias_hh_l0'])
    print_lstm_wei_bias(embedder_net_dict['LSTM_stack.weight_ih_l1'],embedder_net_dict['LSTM_stack.bias_ih_l1'],
                embedder_net_dict['LSTM_stack.weight_hh_l1'],embedder_net_dict['LSTM_stack.bias_hh_l1'])
    print_lstm_wei_bias(embedder_net_dict['LSTM_stack.weight_ih_l2'],embedder_net_dict['LSTM_stack.bias_ih_l2'],
                embedder_net_dict['LSTM_stack.weight_hh_l2'],embedder_net_dict['LSTM_stack.bias_hh_l2'])

    print_wei_bias(embedder_net_dict['projection.weight'],embedder_net_dict['projection.bias'])
    """
    """
    #print(ge2e_loss_dict)
    print('weight')    
    print(ge2e_loss_dict['w'].detach().numpy().astype(np.float128))
    print('bias')
    print(ge2e_loss_dict['b'].detach().numpy().astype(np.float128))
    """

    dn='./vprint';
    nm={};
    v=os.listdir(dn)
    for vi in v:
        nm[vi]=os.path.join(dn,vi);
        en_embeding=torch.load(nm[vi]);
        en_embeding=en_embeding.detach().numpy().astype(np.float128)
        en_embeding=en_embeding.squeeze(0)
        n = en_embeding.shape[0]
        print(vi+'\t'+str(n))
        for i in range(n):
            print(en_embeding[i],end=' ')
        print()


if __name__ == '__main__':
    print_net(sys.argv[1],sys.argv[2],sys.argv[3])
