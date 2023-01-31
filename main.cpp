#include <iostream>
#include <numeric>
#include <vector>
#include <memory>

#include "aff3ct.hpp"
using namespace aff3ct;

struct params {
    int K = 14400;
    int N = 16200;
    int fe = 100;
    int seed = 0;
    float ebn0_min = 2.0f;
    float ebn0_max = 4.01f;
    float ebn0_step = 0.1f;
    float R;
};
void init_params(params& p) {
    p.R = (float)p.K / (float)p.N;
}

struct modules {
    std::unique_ptr<tools::dvbs2_values>                    g_mat;
	std::unique_ptr<module::Source_random<>>                source;
	std::unique_ptr<module::Encoder_LDPC_DVBS2<>>           encoder;
	std::unique_ptr<module::Modem_BPSK<>>                   modem;
	std::unique_ptr<module::Channel_AWGN_LLR<>>             channel;
	std::unique_ptr<module::Decoder_LDPC_BP_flooding_SPA<>> decoder;
	std::unique_ptr<module::Monitor_BFER<>>                 monitor;
};
void init_modules(const params& p, modules& m) {
    m.source = std::unique_ptr<module::Source_random<>>(
        new module::Source_random<>(p.K)
    );
    m.g_mat = tools::build_dvbs2(p.K, p.N);
    m.encoder = std::unique_ptr<module::Encoder_LDPC_DVBS2<>>(
        new module::Encoder_LDPC_DVBS2<>(*m.g_mat)
    );
    m.modem = std::unique_ptr<module::Modem_BPSK<>>(
        new module::Modem_BPSK<>(p.N)
    );
    m.channel = std::unique_ptr<module::Channel_AWGN_LLR<>>(
        new module::Channel_AWGN_LLR<>(p.N)
    );
    auto info_bits_pos = std::vector<uint32_t>(p.K);
    std::iota(info_bits_pos.begin(), info_bits_pos.end(), 0);
    // for (auto i = 0; i < p.K; i++) {
    //     info_bits_pos[i] = i;
    // }
    m.decoder = std::unique_ptr<module::Decoder_LDPC_BP_flooding_SPA<>>(
        new module::Decoder_LDPC_BP_flooding_SPA<>(p.K, p.N, 30, tools::build_H(*m.g_mat), info_bits_pos)
    );
    m.monitor = std::unique_ptr<module::Monitor_BFER<>>(
        new module::Monitor_BFER<>(p.K, p.fe)
    );
    m.channel->set_seed(p.seed);
}

struct buffers {
	std::vector< int > ref_bits;
	std::vector< int > enc_bits;
	std::vector<float> symbols;
	std::vector<float> sigma;
	std::vector<float> noisy_symbols;
	std::vector<float> LLRs;
	std::vector< int > dec_bits;
};
void init_buffers(const params& p, buffers& b) {
    b.ref_bits      = std::vector<int  >(p.K);
	b.enc_bits      = std::vector<int  >(p.N);
	b.symbols       = std::vector<float>(p.N);
	b.sigma         = std::vector<float>(  1);
	b.noisy_symbols = std::vector<float>(p.N);
	b.LLRs          = std::vector<float>(p.N);
	b.dec_bits      = std::vector<int  >(p.K);
}

struct utils {
    std::unique_ptr<tools::Sigma<>>                 noise;
    std::vector<std::unique_ptr<tools::Reporter>>   reporters;
    std::unique_ptr<tools::Terminal_std>            terminal;
};
void init_utils(const modules& m, utils &u) {
    // create a sigma noise type
	u.noise = std::unique_ptr<tools::Sigma<>>(new tools::Sigma<>());
	// report the noise values (Es/N0 and Eb/N0)
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise<>(*u.noise)));
	// report the bit/frame error rates
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER<>(*m.monitor)));
	// report the simulation throughputs
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*m.monitor)));
	// create a terminal that will display the collected data from the reporters
	u.terminal = std::unique_ptr<tools::Terminal_std>(new tools::Terminal_std(u.reporters));
}

int main(int, char**) {
    std::cout << "Using AFF3CT " << tools::version() << std::endl;

    params p;   init_params(p);
    modules m;  init_modules(p, m);
    buffers b;  init_buffers(p, b);
    utils u;    init_utils(m, u);

    u.terminal->legend();
    for (auto ebn0 = p.ebn0_min; ebn0 < p.ebn0_max; ebn0 += p.ebn0_step) {
        const auto esn0 = tools::ebn0_to_esn0(ebn0);
        std::fill(b.sigma.begin(), b.sigma.end(), tools::esn0_to_sigma(esn0));
        u.noise->set_values(b.sigma[0], ebn0, esn0);
        u.terminal->start_temp_report();
        while (!m.monitor->fe_limit_achieved() && !u.terminal->is_interrupt()) {
            m.source->generate(b.ref_bits);
            m.encoder->encode(b.ref_bits, b.enc_bits);
            m.modem->modulate(b.enc_bits, b.symbols);
            m.channel->add_noise(b.sigma, b.symbols, b.noisy_symbols);
            m.modem->demodulate(b.sigma, b.noisy_symbols, b.LLRs);
            m.decoder->decode_siho(b.LLRs, b.dec_bits);
            m.monitor->check_errors(b.dec_bits, b.ref_bits);
        }
        u.terminal->final_report();
        m.monitor->reset();
        u.terminal->reset();
        if (u.terminal->is_over()) break;
    }

    std::cout << "# End of the simulation" << std::endl;
    return 0;
}
