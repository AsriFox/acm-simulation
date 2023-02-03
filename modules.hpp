#pragma once
#include <iostream>
#include <vector>
#include <memory>

#include "aff3ct.hpp"
using namespace aff3ct;

#define upt(T) std::unique_ptr<T>
#define vec(T) std::vector<T>
typedef tools::Monitor_reduction<module::Monitor_BFER<>> Monitor_red_BFER;

struct params {
    float ebn0_min  = 2.00f;
    float ebn0_max  = 4.01f;
    float ebn0_step = 0.10f;
    float R;

    upt(factory::Source      ) source;
    upt(factory::Codec_LDPC  ) codec;
    upt(factory::Modem       ) modem;
    upt(factory::Channel     ) channel;
    upt(factory::Monitor_BFER) monitor;
    upt(factory::Terminal    ) terminal;

    params(int argc, char** argv);
};

struct utils {
        upt(tools::Sigma<> )  noise;
    vec(upt(tools::Reporter)) reporters;
        upt(tools::Terminal)  terminal;

    utils(const params& p, const module::Monitor_BFER<>& monitor);
};

struct modules {
    upt(module::Source<>)       source;
    upt(tools::Codec_SIHO<>)    codec;
    upt(module::Modem<>)        modem;
    upt(module::Channel<>)      channel;
    upt(module::Monitor_BFER<>) monitor;
        module::Encoder<>*      encoder;
        module::Decoder_SIHO<>* decoder;
    vec(const module::Module*)  list;

    modules(const params &p);
    std::vector<float> bind_sockets();
    void run_sim_chain(const utils& u);
};
