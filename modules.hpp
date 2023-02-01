#pragma once
#include <iostream>
#include <vector>
#include <memory>

#include "aff3ct.hpp"
using namespace aff3ct;

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_thread_num() { return 0; }
inline int omp_get_num_threads() { return 1; }
#endif

struct params {
    float ebn0_min  = 2.00f;
    float ebn0_max  = 4.01f;
    float ebn0_step = 0.10f;
    float R;

    std::unique_ptr<factory::Source>       source;
    std::unique_ptr<factory::Codec_LDPC>   codec;
    std::unique_ptr<factory::Modem>        modem;
    std::unique_ptr<factory::Channel>      channel;
    std::unique_ptr<factory::Monitor_BFER> monitor;
    std::unique_ptr<factory::Terminal>     terminal;

    params(int argc, char** argv);
};

struct modules;

struct utils {
                std::unique_ptr<tools::Sigma<> >  noise;
    std::vector<std::unique_ptr<tools::Reporter>> reporters;
                std::unique_ptr<tools::Terminal>  terminal;
    std::vector<std::unique_ptr<module::Monitor_BFER<>>> monitors;
    std::unique_ptr<tools::Monitor_reduction<module::Monitor_BFER<>>> monitor_red;

    std::vector<std::vector<const module::Module*>> m_list;
    std::vector<std::vector<const module::Module*>> m_stat;
    
    utils();
    modules init_modules(const params &p);
    void init(factory::Terminal term_factory);
};

struct modules {
    std::unique_ptr<module::Source<>>       source;
    std::unique_ptr<tools::Codec_SIHO<>>    codec;
    std::unique_ptr<module::Modem<>>        modem;
    std::unique_ptr<module::Channel<>>      channel;
                    module::Monitor_BFER<>* monitor;
                    module::Encoder<>*      encoder;
                    module::Decoder_SIHO<>* decoder;
    std::vector<const module::Module*>      list;

    modules(const params &p, module::Monitor_BFER<>* monitor);
    std::vector<float> bind_sockets();
    void run_sim_chain(const utils &u);
};
