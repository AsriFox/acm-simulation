#include "modules.hpp"
#include <chrono>
#include <stdexcept>

params::params(int argc, char **argv) 
  : source  (new factory::Source()      ),
    codec   (new factory::Codec_LDPC()  ),
    modem   (new factory::Modem()       ),
    channel (new factory::Channel()     ),
    monitor (new factory::Monitor_BFER()),
    terminal(new factory::Terminal()    )
{
    std::vector<factory::Factory*> params_list = {
        source.get(),   codec.get(),
        modem.get(),    channel.get(),
        monitor.get(),  terminal.get(),
    };
    
    tools::Command_parser cp(argc, argv, params_list, true);
    if (cp.parsing_failed()) {
        cp.print_help();
        cp.print_warnings();
        cp.print_errors();
        std::exit(1);
    }

    std::cout << "# Simulation parameters: " << std::endl;
    tools::Header::print_parameters(params_list);
    std::cout << "#" << std::endl;
    cp.print_warnings();

    R = (float)codec->enc->K / (float)codec->enc->N_cw;
}

modules::modules(const params &p) 
  : source  (p.source->build()),
    codec   (p.codec->build()),
    modem   (p.modem->build()),
    channel (p.channel->build()),
    monitor (p.monitor->build())
{
    encoder = &codec->get_encoder();
    decoder = &codec->get_decoder_siho();
    list = { 
        source.get(),  modem.get(), channel.get(),
        monitor.get(), encoder,     decoder,
    };
    for (auto& mod : list) {
        for (auto& tsk : mod->tasks) {
            tsk->set_autoalloc(true);
            tsk->set_debug(false);
            tsk->set_debug_limit(16);
            tsk->set_stats(true);

            if (!tsk->is_debug() && !tsk->is_stats())
                tsk->set_fast(true);
        }
    }
}

utils::utils(const params& p, const module::Monitor_BFER<>& monitor) 
  : noise(new tools::Sigma<>()) 
{
    reporters.push_back(upt(tools::Reporter)(
        new tools::Reporter_noise<>(*noise)
    ));
    reporters.push_back(upt(tools::Reporter)(
        new tools::Reporter_BFER<>(monitor)
    ));
    reporters.push_back(upt(tools::Reporter)(
        new tools::Reporter_throughput<>(monitor)
    ));
    terminal.reset(p.terminal->build(reporters));
}

std::vector<float> modules::bind_sockets() {
    using namespace module;
	(*encoder)[enc::sck::encode      ::U_K ].bind((*source )[src::sck::generate   ::U_K ]);
	(*modem  )[mdm::sck::modulate    ::X_N1].bind((*encoder)[enc::sck::encode     ::X_N ]);
	(*channel)[chn::sck::add_noise   ::X_N ].bind((*modem  )[mdm::sck::modulate   ::X_N2]);
	(*modem  )[mdm::sck::demodulate  ::Y_N1].bind((*channel)[chn::sck::add_noise  ::Y_N ]);
	(*decoder)[dec::sck::decode_siho ::Y_N ].bind((*modem  )[mdm::sck::demodulate ::Y_N2]);
	(*monitor)[mnt::sck::check_errors::U   ].bind((*source )[src::sck::generate   ::U_K ]);
	(*monitor)[mnt::sck::check_errors::V   ].bind((*decoder)[dec::sck::decode_siho::V_K ]);

    std::vector<float> sigma(1);
    (*channel)[chn::sck::add_noise ::CP].bind(sigma);
    (*modem  )[mdm::sck::demodulate::CP].bind(sigma);
    return sigma;
}

void modules::run_sim_chain(const utils& u) {
    using namespace module;
    while (!monitor->fe_limit_achieved() && !u.terminal->is_interrupt()) {
        (*source )[src::tsk::generate    ].exec();
        (*encoder)[enc::tsk::encode      ].exec();
        (*modem  )[mdm::tsk::modulate    ].exec();
        (*channel)[chn::tsk::add_noise   ].exec();
        (*modem  )[mdm::tsk::demodulate  ].exec();
        (*decoder)[dec::tsk::decode_siho ].exec();
        (*monitor)[mnt::tsk::check_errors].exec();
    }
}